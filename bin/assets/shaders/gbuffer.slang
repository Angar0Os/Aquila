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
        float  intensity;
        int    enabled;
        int    type;
        float  lightRadius;
    };
    LightData lights[10];
    int      numLights;
    uint     frameCount;
};

[[vk::push_constant]]
cbuffer PushConstants {
    float4x4 model;
    float4x4 prevModel;
};

struct MaterialData
{
    float4 baseColor;
    float4 params;
    // params.x = metalness multiplier
    // params.y = roughness multiplier
};

[[vk::binding(0, 1)]]
ConstantBuffer<MaterialData> materialData;
[[vk::binding(1, 1)]] Sampler2D albedoTex;
[[vk::binding(2, 1)]] Sampler2D normalTex;
[[vk::binding(3, 1)]] Sampler2D roughMetalTex;

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal   : NORMAL;
    [[vk::location(2)]] float2 uv       : TEXCOORD0;
};

struct VSOutput
{
    float4 position      : SV_Position;
    float3 worldPos      : TEXCOORD0;
    float3 normal        : TEXCOORD1;
    float2 uv            : TEXCOORD2;
    float4 clipPosCur    : TEXCOORD3;
    float4 clipPosPrev   : TEXCOORD4;
};

struct FSOutput
{
    [[vk::location(0)]] float4 albedoMetal : SV_Target0;
    [[vk::location(1)]] float4 normalRough : SV_Target1;
    [[vk::location(2)]] float2 velocity    : SV_Target2;
};

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput o;

    float4 worldPos4     = mul(model,     float4(input.position, 1.0));
    float4 prevWorldPos4 = mul(prevModel, float4(input.position, 1.0));

    float4 clipPos     = mul(proj, mul(view, worldPos4));
    float4 prevClipPos = mul(prevViewProj, prevWorldPos4);

    o.position    = clipPos;
    o.worldPos    = worldPos4.xyz;
    o.normal      = normalize(mul((float3x3)model, input.normal));
    o.uv          = input.uv;

    o.clipPosCur  = clipPos;
    o.clipPosPrev = prevClipPos;

    return o;
}

[shader("fragment")]
FSOutput fragMain(VSOutput input)
{
    FSOutput o;

    float4 albedoSample = albedoTex.Sample(input.uv);
    float4 roughMetSample = roughMetalTex.Sample(input.uv);

    if (albedoSample.a < 0.2f)
        discard;

    float3 albedo = albedoSample.rgb * materialData.baseColor.rgb;

    float metalness = roughMetSample.g * materialData.params.x;
    float roughness = roughMetSample.r * materialData.params.y;

    o.albedoMetal = float4(albedo, metalness);

    float2 ndcCur = input.clipPosCur.xy / input.clipPosCur.w;
    float2 ndcPrev = input.clipPosPrev.xy / input.clipPosPrev.w;
    o.velocity = (ndcCur - ndcPrev) * 0.5f;

    float3 Ngeo = normalize(input.normal);
    float3 texNormal = normalTex.Sample(input.uv).xyz * 2.0 - 1.0;

    float3 Nworld;

    if (abs(texNormal.x) < 0.01 && abs(texNormal.y) < 0.01)
    {
        Nworld = Ngeo;
    }
    else
    {
        float3 up = abs(Ngeo.y) < 0.99
            ? float3(0.0, 1.0, 0.0)
            : float3(1.0, 0.0, 0.0);

        float3 T = normalize(cross(up, Ngeo));
        float3 B = cross(Ngeo, T);
        float3x3 TBN = float3x3(T, B, Ngeo);

        Nworld = normalize(mul(texNormal, TBN));
    }

    o.normalRough = float4(Nworld * 0.5 + 0.5, roughness);

    return o;
}