static const float PI = 3.14159f;

[[vk_binding(0, 0)]] Sampler2D   envMap;
[[vk_binding(1, 0)]] RWTexture2D outIrradiance;

float3 hash33(float3 p3)
{
    p3 = frac(p3 * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return frac((p3.xxy + p3.yxx) * p3.zyx);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 threadId: SV_DispatchThreadID)
{
    uint width, height;
    outIrradiance.GetDimensions(width, height);

    const uint samples = 256;
    float3 result = 0.0f.xxx;

    if (threadId.x >= width || threadId.y >= height)
        return;

    float u = float(threadId.x) / float(width);
    float v = float(threadId.y) / float(height);

    float theta = v * PI;
    float phi   = (u * PI * 2.0f) - PI;

    float x = sin(theta) * sin(phi);
    float y = cos(theta);
    float z = sin(theta) * cos(phi);

    float3 N = normalize(float3(x, y, z));

    for (uint i = 0; i < samples; ++i)
    {
        float3 randVec = normalize(hash33(float3(threadId.xy, i)));

        float NdotL = dot(N, randVec);
        randVec *= sign(NdotL);

        float theta2 = acos(randVec.y);
        float phi2 = atan2(randVec.z, randVec.x);

        float v2 = theta2 / PI;
        float u2 = (phi2 + PI) / (2 * PI);

        float3 color = envMap.SampleLevel(float2(u2, v2), 0).xyz;

        result += color * abs(NdotL) / float(samples);
    }

    outIrradiance[threadId.xy] = float4(result, 1.0f);
}