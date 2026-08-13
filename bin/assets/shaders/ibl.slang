static const float PI = 3.14159265359f;

[[vk::binding(0, 0)]]
cbuffer UniformBufferObject
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewProjInverse;
    float4x4 prevViewProj;
    float4x4 prevViewProjInverse;
    float4   viewPos;
    uint     frameCount;
};

[[vk::binding(1, 0)]] Sampler2D albedoMetal;
[[vk::binding(2, 0)]] Sampler2D normalRough;
[[vk::binding(3, 0)]] Sampler2D depthTex;
[[vk::binding(4, 0)]] RaytracingAccelerationStructure tlas;
[[vk::binding(5, 0)]] Sampler2D envMapTex;
[[vk::binding(6, 0)]] Sampler2D irradianceMapTex;
[[vk::binding(7, 0)]] RWTexture2D<float4> outColor;

float3 ReconstructPosition(float2 uv, float depth)
{
    float4 clipPos  = float4(uv * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = mul(viewProjInverse, clipPos);
    return worldPos.xyz / worldPos.w;
}

float3 hash33(float3 p)
{
    p = frac(p * float3(0.1031f, 0.1030f, 0.0973f));
    p += dot(p, p.yxz + 33.33f);
    return frac((p.xxy + p.yxx) * p.zyx);
}

float3 SampleCosineHemisphere(float2 xi)
{
    float phi      = 2.0f * PI * xi.x;
    float cosTheta = sqrt(1.0f - xi.y);
    float sinTheta = sqrt(xi.y);
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float3x3 BuildTBN(float3 N)
{
    float3 up  = abs(N.y) < 0.999f ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T   = normalize(cross(up, N));
    float3 B   = cross(N, T);
    return float3x3(T, B, N);
}

float3 SampleGGX(float2 xi, float roughness)
{
    float a        = roughness * roughness;
    float phi      = 2.0f * PI * xi.x;
    float cosTheta = sqrt((1.0f - xi.y) / max(1.0f + (a * a - 1.0f) * xi.y, 1e-6f));
    float sinTheta = sqrt(max(1.0f - cosTheta * cosTheta, 0.0f));
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float4 SampleEquirectangular(Sampler2D s, float3 dir)
{
    float theta = acos(clamp(dir.y, -1.0f, 1.0f));
    float phi   = atan2(dir.z, dir.x);
    float u     = (phi + PI) / (2.0f * PI);
    float v     = theta / PI;
    return s.SampleLevel(float2(u, v), 0);
}

float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float2 EnvBRDFApprox(float roughness, float NdotV)
{
    const float4 c0 = float4(-1.0f, -0.0275f, -0.572f,  0.022f);
    const float4 c1 = float4( 1.0f,  0.0425f,  1.040f, -0.040f);
    float4 r = roughness * c0 + c1;
    float  a = min(r.x * r.x, exp2(-9.28f * NdotV)) * r.x + r.y;
    return float2(-1.04f, 1.04f) * a + r.zw;
}

bool TraceOcclusion(float3 origin, float3 direction, float maxDist)
{
    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = direction;
    ray.TMin      = 0.001f;
    ray.TMax      = maxDist - 0.001f;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
    q.TraceRayInline(tlas, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, ray);
    q.Proceed();
    return q.CommittedStatus() != COMMITTED_NOTHING;
}

float3 IBL_Diffuse(float3 worldPos, float3 N,
                   float3 albedo, float metalness,
                   float3 F0, float NdotV,
                   float2 pixelCoord)
{
    float3 seed  = float3(pixelCoord, float(frameCount & 0xFFFFu));
    float3 rand3 = hash33(seed);
    float2 xi    = rand3.xy;

    float3x3 TBN     = BuildTBN(N);
    float3   localL  = SampleCosineHemisphere(xi);
    float3   L       = normalize(mul(localL, TBN));

    if (dot(L, N) <= 0.0f) return 0.0f.xxx;

    float visibility = TraceOcclusion(worldPos + N * 0.005f, L, 1000.0f) ? 0.0f : 1.0f;

    float3 irradiance = SampleEquirectangular(irradianceMapTex, N).rgb;

    float3 F  = F_Schlick(NdotV, F0);
    float3 kD = (1.0f - F) * (1.0f - metalness);

    return kD * albedo * irradiance * visibility;
}

float3 IBL_Specular(float3 worldPos, float3 N, float3 V,
                    float3 F0, float roughness,
                    float NdotV, float2 pixelCoord)
{
    float3 seed  = float3(pixelCoord, float(frameCount & 0xFFFFu) + 0.5f);
    float3 rand3 = hash33(seed);
    float2 xi    = rand3.xy;

    float3x3 TBN   = BuildTBN(N);
    float3   localH = SampleGGX(xi, roughness);
    float3   H      = normalize(mul(localH, TBN));
    float3   L      = reflect(-V, H);

    if (dot(L, N) <= 0.0f) return 0.0f.xxx;

    float visibility = TraceOcclusion(worldPos + N * 0.005f, L, 1000.0f) ? 0.0f : 1.0f;

    float3 envSample = SampleEquirectangular(envMapTex, L).rgb;
    float3 F         = F_Schlick(NdotV, F0);
    float2 brdf      = EnvBRDFApprox(roughness, NdotV);

    return envSample * visibility * (F * brdf.x + brdf.y);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 threadId : SV_DispatchThreadID)
{
    uint width, height;
    outColor.GetDimensions(width, height);

    if (threadId.x >= width || threadId.y >= height)
        return;

    float2 uv    = (float2(threadId.xy) + 0.5f) / float2(width, height);
    float  depth = depthTex.SampleLevel(uv, 0).r;

    if (depth >= 1.0f)
    {
        float3 worldPos = ReconstructPosition(uv, depth);
        float3 dir      = normalize(worldPos - viewPos.xyz);
        outColor[threadId.xy] = float4(SampleEquirectangular(envMapTex, dir).rgb, 1.0f);
        return;
    }

    float3 worldPos = ReconstructPosition(uv, depth);

    float4 albMet  = albedoMetal.SampleLevel(uv, 0);
    float4 norRgh  = normalRough.SampleLevel(uv, 0);

    float3 albedo    = albMet.rgb;
    float  metalness = albMet.a;
    float  roughness = norRgh.a;
    float3 N         = normalize(norRgh.xyz * 2.0f - 1.0f);
    float3 V         = normalize(viewPos.xyz - worldPos);
    float  NdotV     = max(dot(N, V), 0.0f);
    float3 F0        = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);

    float2 pixelCoord = float2(threadId.xy);

    float3 color = 0.0f.xxx;
    color += IBL_Diffuse (worldPos, N, albedo, metalness, F0, NdotV, pixelCoord);
    color += IBL_Specular(worldPos, N, V, F0, roughness, NdotV, pixelCoord);

    outColor[threadId.xy] = float4(color, 1.0f);
}
