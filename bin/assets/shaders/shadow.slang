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

[[vk::binding(1, 0)]] Sampler2D depthTex;
[[vk::binding(2, 0)]] Sampler2D normalRough;
[[vk::binding(3, 0)]] RaytracingAccelerationStructure tlas;
[[vk::binding(4, 0)]] RWTexture2D<float> outShadowMask;

[[vk::push_constant]]
cbuffer ShadowPushConstants
{
    float3 lightDirection;
    float  maxDistance;
};

float3 ReconstructPosition(float2 uv, float depth)
{
    float4 clipPos  = float4(uv * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = mul(viewProjInverse, clipPos); 
    return worldPos.xyz / worldPos.w;
}

bool TraceOcclusion(float3 origin, float3 direction, float maxDist)
{
    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = direction;
    ray.TMin      = 0.0f;
    ray.TMax      = maxDist;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
    q.TraceRayInline(tlas, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, ray);

    while (q.Proceed()) { }

    return q.CommittedStatus() != COMMITTED_NOTHING;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 threadId : SV_DispatchThreadID)
{
    uint width, height;
    outShadowMask.GetDimensions(width, height);

    if (threadId.x >= width || threadId.y >= height)
        return;

    float2 uv    = (float2(threadId.xy) + 0.5f) / float2(width, height);
    float  depth = depthTex.SampleLevel(uv, 0).r;

    if (depth >= 1.0f)
    {
        outShadowMask[threadId.xy] = 1.0f;
        return;
    }

    float3 worldPos = ReconstructPosition(uv, depth);
    float3 N = normalize(normalRough.SampleLevel(uv, 0).xyz * 2.0f - 1.0f);
   
    float3 origin = worldPos + N * 5e-3f; 
    bool   occluded = TraceOcclusion(origin, -lightDirection, maxDistance);

    outShadowMask[threadId.xy] = occluded ? 0.0f : 1.0f;
}