struct UniformBufferObject
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewProjInverse;
    float4x4 prevViewProj;
    float4x4 prevViewProjInverse;
    float4 viewPos;
    uint frameCount;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBufferObject> ubo;

enum ELightType
{
    Sun, 
    Point,
    Spot
};

struct Light
{
    float3 position;
    ELightType type;
    float3 direction;
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
Sampler2D emissiveTex;

struct ResolvePushConstants
{
    Light *lightBuffer;
    uint numLights;
};

[[vk::push_constant]]
ConstantBuffer<ResolvePushConstants> pc;

static const float exposure = 1.0f;
static const float PI = 3.14159265f;

float2 DirectionToEquirectUV(float3 dir)
{
    float phi = atan2(dir.z, dir.x);
    float theta = acos(clamp(dir.y, -1.0f, 1.0f));
    return float2(phi / (2.0f * PI) + 0.5f, theta / PI);
}

float3 ReconstructPosition(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = mul(ubo.viewProjInverse, clipPos);
    return worldPos.xyz / worldPos.w;
}

float3 ACESFilm(float3 color, float exposure)
{
    float3 x = exp2(exposure) * color;
    float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float D_GGX(float NdotH, float roughness)
{
    float a = max(roughness * roughness, 0.001f);
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / max(PI * d * d, 0.0001f);
}

float G_SmithSchlickGGX(float NdotV, float NdotL, float roughness)
{
    float k = (roughness * roughness) * 0.5f;
    float Gv = NdotV / max(NdotV * (1.0f - k) + k, 0.0001f);
    float Gl = NdotL / max(NdotL * (1.0f - k) + k, 0.0001f);
    return Gv * Gl;
}

float3 F_Schlick(float VdotH, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - VdotH), 5.0f);
}

float3 EvaluateLight(
    Light light,
    float3 worldPos,
    float3 N,
    float3 V,
    float3 albedo,
    float metalness,
    float roughness)
{
    float3 toLight = light.position - worldPos;
    if (light.type == ELightType.Sun)
        toLight = -light.direction;

    float dist = light.type == ELightType.Sun ? 1.0f : length(toLight);
    float3 L = toLight / max(dist, 0.0001f);

    float attenuation = 1.0f / max(dist * dist, 0.0001f);
    if (light.radius > 0.0f)
        attenuation *= saturate(1.0f - pow(dist / light.radius, 4.0f));

    float NdotL = max(dot(N, L), 0.0f);
    if (NdotL <= 0.0f)
        return 0.0f.xxx;

    float NdotV = max(dot(N, V), 0.0001f);

    float3 H = normalize(V + L);
    float NdotH = max(dot(N, H), 0.0f);
    float VdotH = max(dot(V, H), 0.0f);

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);
    float3 diffuseColor = albedo * (1.0f - metalness);

    float D = D_GGX(NdotH, roughness);
    float G = G_SmithSchlickGGX(NdotV, NdotL, roughness);
    float3 F = F_Schlick(VdotH, F0);

    float3 specular = (D * G * F) / max(4.0f * NdotV * NdotL, 0.0001f);
    float3 diffuse = diffuseColor / PI;

    return (diffuse + specular) * light.color * light.intensity * NdotL * attenuation;
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
        float3 viewDir = normalize(farPos - ubo.viewPos.xyz);
        float2 envUV = DirectionToEquirectUV(viewDir);
        float3 skyHdr = envMapRaw.SampleLevel(envUV, 0).rgb * 0.1f;
        outImage[coord] = float4(ACESFilm(skyHdr, exposure), 1.0);
        return;
    }

    float4 albedoMetalSample = albedoTex.SampleLevel(uv, 0);
    float4 normalRoughSample = normalTex.SampleLevel(uv, 0);
    float3 emissive = emissiveTex.SampleLevel(uv, 0).rgb * 10.0f;

    float3 albedo    = albedoMetalSample.rgb;
    float  metalness = albedoMetalSample.a;

    float3 N         = normalize(normalRoughSample.xyz * 2.0f - 1.0f);
    float  roughness = normalRoughSample.a;

    float shadow = shadowMaskTex.SampleLevel(uv, 0).r;

    float3 worldPos = ReconstructPosition(uv, depth);
    float3 V = normalize(ubo.viewPos.xyz - worldPos);

    float3 direct = 0.0f.xxx;

    for (uint i = 0; i < pc.numLights; ++i)
    {
        Light light = pc.lightBuffer[i];

        float3 current = EvaluateLight(
            light,
            worldPos,
            N,
            V,
            albedo,
            metalness,
            roughness
        );

        if (i == 0)
            current *= shadow;

        direct += current;
    }

    float3 hdrResult = gi + direct + emissive;
    float3 ldrResult = ACESFilm(hdrResult, exposure);
    outImage[coord] = float4(ldrResult, 1.0);
}