[[vk::binding(0, 0)]]
cbuffer UniformBufferObject
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewProjInverse;
    float4x4 prevViewProj;
    float4x4 prevViewProjInverse;
    float4 viewPos;
    uint frameCount;
};

struct GPULight
{
    float3 position;
    float radius;
    float3 color;
    float intensity;
};

[[vk::binding(1, 0)]]
Sampler2D albedoTex;

[[vk::binding(2, 0)]]
Sampler2D normalTex;

[[vk::binding(3, 0)]]
Sampler2D shadowMaskTex;

[[vk::binding(4, 0)]]
RWTexture2D<float4> outImage;

[[vk::binding(5, 0)]]
Sampler2D giTex;

[[vk::binding(6, 0)]]
Sampler2D depthTex;

[[vk::binding(7, 0)]]
Sampler2D envMapRaw;

[[vk::binding(8, 0)]]
StructuredBuffer<GPULight> lights;

[[vk::push_constant]]
cbuffer ResolvePushConstants
{
    uint numLights;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

static const float3 sunDir = normalize(float3(-1.0, -0.2, -0.3));
static const float3 sunColor = float3(1.0, 1.0, 1.0);
static const float  exposure = 1.0f;

float2 DirectionToEquirectUV(float3 dir)
{
    float phi = atan2(dir.z, dir.x);
    float theta = acos(clamp(dir.y, -1.0f, 1.0f));
    return float2(phi / (2.0f * 3.14159265f) + 0.5f, theta / 3.14159265f);
}

float3 ReconstructPosition(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = mul(viewProjInverse, clipPos);
    return worldPos.xyz / worldPos.w;
}

float3 ACESFilm(float3 color, float exposure)
{
    float3 x = exp2(exposure) * color;
    float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 dispatchThreadID: SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;

    uint width, height;
    outImage.GetDimensions(width, height);

    if (coord.x >= width || coord.y >= height)
        return;

    float2 uv = (float2(coord) + 0.5) / float2(width, height);

    float depth = depthTex.SampleLevel(uv, 0).r;
    float3 gi = giTex.SampleLevel(uv, 0).rgb;

    if (depth >= 1.0f)
    {
        float3 farPos = ReconstructPosition(uv, 1.0f);
        float3 viewDir = normalize(farPos - viewPos.xyz);
        float2 envUV = DirectionToEquirectUV(viewDir);
        float3 skyHdr = envMapRaw.SampleLevel(envUV, 0).rgb;
        outImage[coord] = float4(ACESFilm(skyHdr, exposure), 1.0);
        return;
    }

    float3 albedo = albedoTex.SampleLevel(uv, 0).rgb;
    float3 N = normalize(normalTex.SampleLevel(uv, 0).xyz * 2.0 - 1.0);
    float shadow = shadowMaskTex.SampleLevel(uv, 0).r;

    float NdotL = max(dot(N, -sunDir), 0.0);
    float3 direct = albedo * sunColor * NdotL * shadow;
    float3 worldPos = ReconstructPosition(uv, depth);

    for (uint i = 0; i < numLights; ++i)
    {
        GPULight light = lights[i];

        float3 toLight = light.position - worldPos;
        float  dist    = length(toLight);
        float3 L       = toLight / max(dist, 0.0001f);

        float attenuation = 1.0f / max(dist * dist, 0.0001f);
        if (light.radius > 0.0f)
            attenuation *= saturate(1.0f - pow(dist / light.radius, 4.0f));

        float pointNdotL = max(dot(N, L), 0.0f);

        direct += albedo * light.color * light.intensity * pointNdotL * attenuation;
    }

    float3 hdrResult = direct + gi;
    float3 ldrResult = ACESFilm(hdrResult, exposure);

    outImage[coord] = float4(ldrResult, 1.0);
}