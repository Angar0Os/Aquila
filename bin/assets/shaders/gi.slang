struct GIConstants
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewProjInverse;
    float4x4 prevViewProj;
    float4x4 prevViewProjInverse;
    float4   viewPos;
    uint     frameCount;
};

[[vk::binding(0, 0)]]
ConstantBuffer<GIConstants> gGIConstants;

[[vk::binding(1, 0)]]
Sampler2D gDepth;

[[vk::binding(2, 0)]]
Sampler2D gNormal;

[[vk::binding(3, 0)]]
Sampler2D gAlbedo;

[[vk::binding(4, 0)]]
RaytracingAccelerationStructure gTLAS;

[[vk::binding(5, 0)]]
Sampler2D gEnvironment;

[[vk::binding(6, 0)]]
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> gGIOutput;

[[vk::binding(7, 0)]]
Sampler2D gGIHistory;

[[vk::push_constant]]
cbuffer GIPushConstants
{
    uint  sampleCount;
    float maxDistance;
};

static const float PI = 3.14159265f;

float3 ReconstructPosition(float2 uv, float depth)
{
    float4 clipPos  = float4(uv * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = mul(gGIConstants.viewProjInverse, clipPos);
    return worldPos.xyz / worldPos.w;
}

uint Hash(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

float RandomFloat(inout uint seed)
{
    seed = Hash(seed);
    return float(seed) / 4294967296.0f;
}

void BuildBasis(float3 N, out float3 T, out float3 B)
{
    float3 up = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

float3 SampleHemisphere(float3 N, inout uint seed, out float pdf)
{
    float u1 = RandomFloat(seed);
    float u2 = RandomFloat(seed);

    float r     = sqrt(u1);
    float theta = 2.0f * PI * u2;

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(max(0.0f, 1.0f - u1));

    float3 T, B;
    BuildBasis(N, T, B);

    float3 dir = normalize(T * x + B * y + N * z);
    pdf = z / PI;

    return dir;
}

float2 DirectionToEquirectUV(float3 dir)
{
    float phi   = atan2(dir.z, dir.x);
    float theta = acos(clamp(dir.y, -1.0f, 1.0f));
    return float2(phi / (2.0f * PI) + 0.5f, theta / PI);
}

bool TraceOcclusion(float3 origin, float3 direction, float maxDist)
{
    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = direction;
    ray.TMin      = 0.001f;
    ray.TMax      = maxDist - 0.001f;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
    q.TraceRayInline(gTLAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, ray);

    while (q.Proceed()) { }

    return q.CommittedStatus() != COMMITTED_NOTHING;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchThreadID.xy;

    uint width, height;
    gGIOutput.GetDimensions(width, height);

    if (pixel.x >= width || pixel.y >= height)
        return;

    float2 uv    = (float2(pixel) + 0.5f) / float2(width, height);
    float  depth = gDepth.SampleLevel(uv, 0).r;

    if (depth >= 1.0f)
    {
        //float3 farPos = ReconstructPosition(uv, 1.0f);
        //float3 viewDir = normalize(farPos - gGIConstants.viewPos.xyz);
        //float2 envUV = DirectionToEquirectUV(viewDir);
        //gGIOutput[pixel] = float4(gEnvironment.SampleLevel(envUV, 0).rgb, 1.0f);
        gGIOutput[pixel] = float4(0.0f, 0.0f, 0.0f, 1.0f);   // TODO : This is for debug, we will keep it until gi is good enough.
        return;
    }

    float3 worldPos = ReconstructPosition(uv, depth);
    float3 N        = normalize(gNormal.SampleLevel(uv, 0).xyz * 2.0f - 1.0f);
    float3 albedo   = gAlbedo.SampleLevel(uv, 0).rgb;

    float3 origin = worldPos + N * 0.01f;

    uint seed = Hash(pixel.x + Hash(pixel.y + Hash(gGIConstants.frameCount)));

    float3 GI = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < sampleCount; ++i)
    {
        float pdf;
        float3 wi = SampleHemisphere(N, seed, pdf);

        if (pdf <= 0.0f)
            continue;

        bool hit = TraceOcclusion(origin, wi, maxDistance);

        float3 Li;
        if (hit)
        {
            Li = float3(0.05f, 0.05f, 0.05f);
        }
        else
        {
            float2 envUV = DirectionToEquirectUV(wi);
            Li = gEnvironment.SampleLevel(envUV, 0).rgb;
        }

        float3 brdf    = albedo / PI;    
        float  cosTerm = max(dot(N, wi), 0.0f);

        GI += (brdf * Li * cosTerm) / pdf;
    }

    GI /= float(sampleCount);

    float3 finalGI = GI;

    float4 prevClip = mul(gGIConstants.prevViewProj, float4(worldPos, 1.0f));
    if (prevClip.w > 0.0f)
    {
        float2 prevUV = (prevClip.xy / prevClip.w) * 0.5f + 0.5f;

        bool onScreen = all(prevUV >= 0.0f) && all(prevUV <= 1.0f);
        bool firstFrame = gGIConstants.frameCount == 0;

        if (onScreen && !firstFrame)
        {
            float3 history = gGIHistory.SampleLevel(prevUV, 0).rgb;
            float alpha = 0.05f;
            finalGI = lerp(history, GI, alpha);
        }
    }

    gGIOutput[pixel] = float4(finalGI, 1.0f);
}