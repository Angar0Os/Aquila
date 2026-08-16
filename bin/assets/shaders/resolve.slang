struct PushConstants
{
  
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

static const float3 sunDir = normalize(float3(-1.0, -0.2, -0.3));
static const float3 sunColor = float3(1.0, 1.0, 1.0);

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
        outImage[coord] = float4(gi, 1.0);
        return;
    }

    float3 albedo = albedoTex.SampleLevel(uv, 0).rgb;
    float3 N = normalize(normalTex.SampleLevel(uv, 0).xyz * 2.0 - 1.0);
    float shadow = shadowMaskTex.SampleLevel(uv, 0).r;

    float NdotL = max(dot(N, -sunDir), 0.0);
    float3 direct = albedo * sunColor * NdotL * shadow;

    float3 result = direct * gi;

    outImage[coord] = float4(result, 1.0);
}