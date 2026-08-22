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

[[vk::binding(9, 0)]]
Sampler2D gEnvironmentRaw;

struct MeshTableEntry
{
    uint64_t indices;
    uint64_t positions;
    uint64_t normals;
    uint64_t uvs;
    uint64_t tangents;
    uint     materialId;
    uint     vertexStride;
};

struct GPUMaterial
{
    float4 baseColor;
    float4 emissive;
    float4 properties;
    uint4  textures;
};

[[vk::binding(10, 0)]]
StructuredBuffer<MeshTableEntry> gMeshTable;

[[vk::binding(11, 0)]]
StructuredBuffer<GPUMaterial> gMaterialTable;

[[vk::binding(12, 0)]]
Sampler2D gBindlessTextures[4096];

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

struct GIPushConstants
{
    uint sampleCount;
    float maxDistance;
    uint numLights;
    Light* lightBuffer;
};

[[vk::push_constant]]
ConstantBuffer<GIPushConstants> pc;

static const float PI = 3.14159265f;
static const float SURFACE_BIAS = 0.2f;

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

struct HitSurface
{
    float3 position;
    float3 normal;
    float3 geometricNormal;
    float2 uv;
    GPUMaterial material;
};

uint3 LoadTriangleIndices(uint64_t indicesAddr, uint primitiveIndex)
{
    uint* idxBase = reinterpret<uint*>(indicesAddr);
    uint base = primitiveIndex * 3;
    return uint3(idxBase[base + 0], idxBase[base + 1], idxBase[base + 2]);
}

float3 LoadVertexFloat3(uint64_t baseAddr, uint vertexIndex, uint stride)
{
    uint64_t addr = baseAddr + uint64_t(vertexIndex) * uint64_t(stride);
    return *reinterpret<float3*>(addr);
}

float2 LoadVertexFloat2(uint64_t baseAddr, uint vertexIndex, uint stride)
{
    uint64_t addr = baseAddr + uint64_t(vertexIndex) * uint64_t(stride);
    return *reinterpret<float2*>(addr);
}

HitSurface GetHitSurface(uint meshTableIndex, uint primitiveIndex, float2 barycentrics)
{
    MeshTableEntry meshEntry = gMeshTable[meshTableIndex];
    uint3 tri = LoadTriangleIndices(meshEntry.indices, primitiveIndex);

    float3 n0 = LoadVertexFloat3(meshEntry.normals, tri.x, meshEntry.vertexStride);
    float3 n1 = LoadVertexFloat3(meshEntry.normals, tri.y, meshEntry.vertexStride);
    float3 n2 = LoadVertexFloat3(meshEntry.normals, tri.z, meshEntry.vertexStride);

    float2 uv0 = LoadVertexFloat2(meshEntry.uvs, tri.x, meshEntry.vertexStride);
    float2 uv1 = LoadVertexFloat2(meshEntry.uvs, tri.y, meshEntry.vertexStride);
    float2 uv2 = LoadVertexFloat2(meshEntry.uvs, tri.z, meshEntry.vertexStride);

    float3 p0 = LoadVertexFloat3(meshEntry.positions, tri.x, meshEntry.vertexStride);
    float3 p1 = LoadVertexFloat3(meshEntry.positions, tri.y, meshEntry.vertexStride);
    float3 p2 = LoadVertexFloat3(meshEntry.positions, tri.z, meshEntry.vertexStride);

    float3 baryWeights = float3(1.0f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

    HitSurface surface;
    surface.geometricNormal = normalize(cross(p1 - p0, p2 - p0));
    surface.normal          = normalize(n0 * baryWeights.x + n1 * baryWeights.y + n2 * baryWeights.z);
    surface.uv              = uv0 * baryWeights.x + uv1 * baryWeights.y + uv2 * baryWeights.z;
    surface.material        = gMaterialTable[meshEntry.materialId];

    return surface;
}

float3 SampleMaterialAlbedo(GPUMaterial mat, float2 uv)
{
    float3 texSample = gBindlessTextures[NonUniformResourceIndex(mat.textures.x)].SampleLevel(uv, 0).rgb;
    return mat.baseColor.rgb * texSample;
}

float3 TraceRadiance(float3 origin, float3 direction, inout uint seed)
{
    float3 radiance = float3(0.0f, 0.0f, 0.0f);
    float3 throughput = float3(1.0f, 1.0f, 1.0f);

    float3 rayOrigin = origin;
    float3 rayDir = direction;

    const uint maxBounces = 4;

    for (uint bounce = 0; bounce < maxBounces; ++bounce)
    {
        RayDesc ray;
        ray.Origin = rayOrigin;
        ray.Direction = rayDir;
        ray.TMin = 0.0f;
        ray.TMax = pc.maxDistance;

        RayQuery<RAY_FLAG_NONE> q;
        q.TraceRayInline(gTLAS, RAY_FLAG_NONE, 0xFF, ray);
        while (q.Proceed()) { q.CommitNonOpaqueTriangleHit(); }

        if (q.CommittedStatus() == COMMITTED_NOTHING)
        {
            float2 envUV = DirectionToEquirectUV(rayDir);
            radiance += throughput * gEnvironmentRaw.SampleLevel(envUV, 0).rgb * 0.1f;
            break;
        }

        float3 hitPos = rayOrigin + rayDir * q.CommittedRayT();

        uint meshTableIndex   = q.CommittedInstanceID();
        uint primitiveIndex   = q.CommittedPrimitiveIndex();
        float2 barycentrics   = q.CommittedTriangleBarycentrics();

        HitSurface surface = GetHitSurface(meshTableIndex, primitiveIndex, barycentrics);

        float3 hitNormal = surface.normal;
        float3 geoNormal = surface.geometricNormal;

        if (dot(hitNormal, rayDir) > 0.0f)
            hitNormal = -hitNormal;

        if (dot(geoNormal, rayDir) > 0.0f) 
            geoNormal = -geoNormal;

        float3 approxAlbedo = SampleMaterialAlbedo(surface.material, surface.uv);

        for (int i = 0; i < pc.numLights; ++i)
        {
            Light light = pc.lightBuffer[i];
            bool occluded = TraceOcclusion(hitPos + geoNormal * SURFACE_BIAS, -pc.lightBuffer[i].direction, pc.maxDistance);
            if (!occluded)
            {
                float3 toLight = light.position - hitPos;
                if (light.type == ELightType.Sun)
                    toLight = -light.direction;

                float dist = light.type == ELightType.Sun ? 1.0f : length(toLight);
                float3 L = toLight / max(dist, 0.0001f);

                float attenuation = 1.0f / max(dist * dist, 0.0001f);
                if (light.radius > 0.0f)
                    attenuation *= saturate(1.0f - pow(dist / light.radius, 4.0f));

                float NdotL = max(dot(hitNormal, L), 0.0f);

                float3 current = approxAlbedo * light.color * light.intensity * NdotL * attenuation;

                radiance += throughput * current;
            }
        }

        float pdf;
        float3 newDir = SampleHemisphere(hitNormal, seed, pdf);
        if (pdf <= 0.0f)
            break;

        float3 brdf = approxAlbedo / PI;
        float cosTerm = max(dot(hitNormal, newDir), 0.0f);
        throughput *= (brdf * cosTerm) / pdf;

        rayOrigin = hitPos + geoNormal * SURFACE_BIAS;
        rayDir = newDir;
    }

    return radiance;
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
        gGIOutput[pixel] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    float3 worldPos = ReconstructPosition(uv, depth);
    float3 N        = normalize(gNormal.SampleLevel(uv, 0).xyz * 2.0f - 1.0f);
    float3 albedo   = gAlbedo.SampleLevel(uv, 0).rgb;

    float3 origin = worldPos + N * SURFACE_BIAS;

    uint seed = Hash(pixel.x + Hash(pixel.y + Hash(gGIConstants.frameCount)));

    float3 GI = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < pc.sampleCount; ++i)
    {
        float pdf;
        float3 wi = SampleHemisphere(N, seed, pdf);
        if (pdf <= 0.0f) continue;

        float3 Li = TraceRadiance(origin, wi, seed);

        float3 brdf = albedo / PI;
        float cosTerm = max(dot(N, wi), 0.0f);

        GI += (brdf * Li * cosTerm) / pdf;
    }

    GI /= float(pc.sampleCount);

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