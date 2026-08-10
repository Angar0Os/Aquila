static const float PI = 3.14159f;

[[vk::binding(0, 0)]]
cbuffer UniformBufferObject
{
    float4x4 view;
    float4x4 proj;
    float4x4 lightSpaceMatrix;
    float4x4 viewProjInverse;
    float4x4 prevViewProj;
    float4x4 prevViewProjInverse;
    float4 viewPos;
    struct LightData
    {
        float4 position;
        float4 color;
        float intensity;
        int enabled;
        int type;
        float lightRadius;
    };
    LightData lights[10];
    int numLights;
    uint frameCount;
};

[[vk::binding(1, 0)]] Sampler2D albedoMetal;
[[vk::binding(2, 0)]] Sampler2D normalRough;
[[vk::binding(3, 0)]] Sampler2D depthTex;
[[vk::binding(4, 0)]] RaytracingAccelerationStructure tlas;
[[vk::binding(5, 0)]] Sampler2D envMapTex;
[[vk::binding(6, 0)]] Sampler2D irradianceMapTex;

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertMain(uint vertexID : SV_VertexID)
{
    VSOutput o;
    o.uv       = float2((vertexID << 1) & 2, vertexID & 2);
    o.position = float4(o.uv * 2.0 - 1.0, 0.0, 1.0);
    return o;
}

float3 ReconstructPosition(float2 uv, float depth)
{
    float4 clipPos  = float4(uv * 2.0 - 1.0, depth, 1.0);
    float4 worldPos = mul(viewProjInverse, clipPos);
    return worldPos.xyz / worldPos.w;
}

bool TraceShadow(float3 origin, float3 direction, float maxDist)
{
    RayDesc ray;
    ray.Origin    = origin;
    ray.Direction = direction;
    ray.TMin      = 0.001;
    ray.TMax      = maxDist - 0.001;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
    query.TraceRayInline(tlas, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, ray);
    query.Proceed();

    return query.CommittedStatus() != COMMITTED_NOTHING;
}

float D_GGX(float3 N, float3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float G_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float3 CookTorrance(float3 worldPos, float3 N, float3 albedo,
                    float metalness, float roughness, LightData light)
{
    float3 V    = normalize(viewPos.xyz - worldPos);
    float3 L    = normalize(light.position.xyz - worldPos);
    float3 H    = normalize(V + L);
    float  dist = length(light.position.xyz - worldPos);

    float  atten    = light.intensity / (dist * dist);
    float3 radiance = light.color.rgb * atten;

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metalness);

    float  D        = D_GGX(N, H, roughness);
    float  G        = G_Smith(N, V, L, roughness);
    float3 F        = F_Schlick(max(dot(H, V), 0.0), F0);

    float  NdotL    = max(dot(N, L), 0.0);
    float  NdotV    = max(dot(N, V), 0.0);
    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);

    float3 kS      = F;
    float3 kD      = (float3(1.0, 1.0, 1.0) - kS) * (1.0 - metalness);
    float3 diffuse = kD * albedo / PI;

    bool  inShadow = TraceShadow(worldPos + N * 0.005, L, dist);
    float shadow   = inShadow ? 0.0 : 1.0;

    return (diffuse + specular) * radiance * NdotL * shadow;
}

float3 ACESFilm(float3 color, float exposure)
{
    float3 x = exp2(exposure) * color;
    float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 SampleEquirectangular(Sampler2D s, float3 dir)
{
    float theta = acos(clamp(dir.y, -1.0, 1.0));
    float phi   = atan2(dir.z, dir.x);
    float U     = (phi + PI) / (2.0 * PI);
    float V     = theta / PI;
    return s.Sample(float2(U, V));
}

float4 SampleEquirectangularLod(Sampler2D s, float3 dir, float lod)
{
    float theta = acos(clamp(dir.y, -1.0, 1.0));
    float phi   = atan2(dir.z, dir.x);
    float U     = (phi + PI) / (2.0 * PI);
    float V     = theta / PI;
    return s.SampleLevel(float2(U, V), lod);
}

float2 EnvBRDFApprox(float roughness, float NdotV)
{
    const float4 c0 = float4(-1.0, -0.0275, -0.572,  0.022);
    const float4 c1 = float4( 1.0,  0.0425,  1.040, -0.040);
    float4 r = roughness * c0 + c1;
    float  a = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    return float2(-1.04, 1.04) * a + r.zw;
}

static const float MAX_REFLECTION_LOD = 7.0f;

float3 hash33(float3 p3)
{
    p3 = frac(p3 * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return frac((p3.xxy + p3.yxx) * p3.zyx);
}

float3 IBL_Specular(float3 N, float3 V, float3 F0, float roughness,
                    float3 worldPos, uint frameCount)
{
    float  NdotV = max(dot(N, V), 0.0);
    float3 R     = reflect(-V, N);
    float3 F     = F_Schlick(NdotV, F0);
    float2 brdf  = EnvBRDFApprox(roughness, NdotV);

    float3 color = 0.0f;

    for (int i = 0; i < 4; ++i)
    {
        float3 randVec  = hash33(worldPos * 1000.0f + float3(frameCount * 1000 + i) * 0.01f);
        float  RdotRand = dot(R, randVec);
        randVec        *= sign(RdotRand);
        float3 rayDir   = normalize(lerp(R, randVec, roughness));

        float visibility = TraceShadow(worldPos + N * 0.001f, rayDir, 1000.0f) ? 0.0f : 1.0f;

        color += SampleEquirectangular(envMapTex, rayDir).rgb * visibility * 0.25f;
    }

    return color * (F * brdf.x + brdf.y);
}

float3 IBL_Diffuse(float3 N, float3 albedo, float metalness,
                   float3 F0, float NdotV, float roughness, float3 worldPos)
{
    float3 color = 0.0f;

    for (int i = 0; i < 4; ++i)
    {
        float3 randVec = hash33(worldPos * 1000.0f + float3(frameCount * 1000 + i) * 0.01f);
        float  NdotL   = dot(N, randVec);
        randVec       *= sign(NdotL);

        float visibility = TraceShadow(worldPos + N * 0.001f, randVec, 1000.0f) ? 0.0f : 1.0f;

        float3 F  = F_Schlick(NdotV, F0);
        float3 kD = (1.0 - F) * (1.0 - metalness);
        color += kD * albedo * SampleEquirectangular(irradianceMapTex, randVec).rgb * visibility * 0.25f;
    }

    return color;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float2 uv = input.uv;

    float  depth    = depthTex.Sample(uv).r;
    float3 worldPos = ReconstructPosition(uv, depth);
    float3 color    = 0.0f.xxx;

    if (depth < 1.0f)
    {
        float4 albMet = albedoMetal.Sample(uv);
        float4 norRgh = normalRough.Sample(uv);

        float3 albedo    = albMet.rgb;
        float  metalness = albMet.w;
        float  roughness = norRgh.w;
        float3 N         = normalize(norRgh.xyz * 2.0 - 1.0);
        float3 V         = normalize(viewPos.xyz - worldPos);
        float  NdotV     = max(dot(N, V), 0.0);
        float3 F0        = lerp(float3(0.04, 0.04, 0.04), albedo, metalness);

        color += IBL_Diffuse(N, albedo, metalness, F0, NdotV, roughness, worldPos);
        color += IBL_Specular(N, V, F0, roughness, worldPos, frameCount);

        for (int i = 0; i < numLights; i++)
        {
            if (lights[i].enabled == 0) continue;
            color += CookTorrance(worldPos, N, albedo, metalness, roughness, lights[i]);
        }
    }
    else
    {
        float3 dir = normalize(worldPos - viewPos.xyz);
        color = SampleEquirectangular(envMapTex, dir).xyz;
    }

    return float4(color, 1.0);
}