[[vk::binding(0, 0)]]
Sampler2D gInput;

[[vk::binding(1, 0)]]
Sampler2D gDepth;

[[vk::binding(2, 0)]]
Sampler2D gNormal;

[[vk::binding(3, 0)]]
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> gOutput;

[[vk::push_constant]]
cbuffer ATrousPushConstants
{
    int   stepSize;
    float sigmaDepth;
    float sigmaNormal;
    float _pad;
};

static const float kernel[5] = { 1.0f/16.0f, 4.0f/16.0f, 6.0f/16.0f, 4.0f/16.0f, 1.0f/16.0f };

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    uint2 pixel = id.xy;

    uint width, height;
    gOutput.GetDimensions(width, height);
    if (pixel.x >= width || pixel.y >= height) return;

    float2 texel = 1.0f / float2(width, height);
    float2 centerUV = (float2(pixel) + 0.5f) * texel;

    float3 centerColor  = gInput.SampleLevel(centerUV, 0).rgb;
    float3 centerNormal = normalize(gNormal.SampleLevel(centerUV, 0).xyz * 2.0f - 1.0f);
    float  centerDepth  = gDepth.SampleLevel(centerUV, 0).r;

    if (centerDepth >= 1.0f)
    {
        gOutput[pixel] = float4(centerColor, 1.0f);
        return;
    }

    float3 sum = float3(0, 0, 0);
    float  weightSum = 0.0f;

    for (int y = -2; y <= 2; ++y)
    {
        for (int x = -2; x <= 2; ++x)
        {
            float2 offset = float2(x, y) * float(stepSize) * texel;
            float2 sampleUV = centerUV + offset;

            float3 sampleColor  = gInput.SampleLevel(sampleUV, 0).rgb;
            float3 sampleNormal = normalize(gNormal.SampleLevel(sampleUV, 0).xyz * 2.0f - 1.0f);
            float  sampleDepth  = gDepth.SampleLevel(sampleUV, 0).r;

            float kernelWeight = kernel[x + 2] * kernel[y + 2];

            float depthWeight  = exp(-abs(centerDepth - sampleDepth) / max(sigmaDepth, 1e-5f));
            float normalWeight = pow(saturate(dot(centerNormal, sampleNormal)), sigmaNormal);

            float w = kernelWeight * depthWeight * normalWeight;

            sum += sampleColor * w;
            weightSum += w;
        }
    }

    gOutput[pixel] = float4(sum / max(weightSum, 1e-5f), 1.0f);
}