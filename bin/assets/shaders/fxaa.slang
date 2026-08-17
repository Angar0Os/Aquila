[[vk::binding(0, 0)]]
Sampler2D inputTex;

[[vk::binding(1, 0)]]
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> outputTex;

[[vk::push_constant]]
cbuffer FXAAPushConstants
{
    float2 texelSize;
    float2 _pad;
};

static const float EDGE_THRESHOLD_MIN = 0.0312f;
static const float EDGE_THRESHOLD_MAX = 0.125f;
static const int   ITERATIONS         = 12;
static const float SUBPIXEL_QUALITY   = 0.75f;

float Luma(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float3 SampleColor(float2 uv)
{
    return inputTex.SampleLevel(uv, 0).rgb;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    outputTex.GetDimensions(width, height);

    if (dispatchThreadID.x >= width || dispatchThreadID.y >= height)
        return;

    float2 uv = (float2(dispatchThreadID.xy) + 0.5f) * texelSize;

    float3 colorCenter = SampleColor(uv);

    float lumaCenter = Luma(colorCenter);
    float lumaUp     = Luma(SampleColor(uv + float2(0.0f, -texelSize.y)));
    float lumaDown    = Luma(SampleColor(uv + float2(0.0f,  texelSize.y)));
    float lumaLeft    = Luma(SampleColor(uv + float2(-texelSize.x, 0.0f)));
    float lumaRight    = Luma(SampleColor(uv + float2( texelSize.x, 0.0f)));

    float lumaMin = min(lumaCenter, min(min(lumaUp, lumaDown), min(lumaLeft, lumaRight)));
    float lumaMax = max(lumaCenter, max(max(lumaUp, lumaDown), max(lumaLeft, lumaRight)));

    float lumaRange = lumaMax - lumaMin;

    if (lumaRange < max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX))
    {
        outputTex[dispatchThreadID.xy] = float4(colorCenter, 1.0f);
        return;
    }

    float lumaUpLeft    = Luma(SampleColor(uv + float2(-texelSize.x, -texelSize.y)));
    float lumaUpRight   = Luma(SampleColor(uv + float2( texelSize.x, -texelSize.y)));
    float lumaDownLeft  = Luma(SampleColor(uv + float2(-texelSize.x,  texelSize.y)));
    float lumaDownRight = Luma(SampleColor(uv + float2( texelSize.x,  texelSize.y)));

    float lumaDownUp    = lumaDown + lumaUp;
    float lumaLeftRight = lumaLeft + lumaRight;

    float lumaLeftCorners  = lumaUpLeft + lumaDownLeft;
    float lumaDownCorners  = lumaDownLeft + lumaDownRight;
    float lumaRightCorners = lumaUpRight + lumaDownRight;
    float lumaUpCorners    = lumaUpLeft + lumaUpRight;

    float edgeHorizontal =
        abs(-2.0f * lumaLeft + lumaLeftCorners) +
        abs(-2.0f * lumaCenter + lumaDownUp) * 2.0f +
        abs(-2.0f * lumaRight + lumaRightCorners);

    float edgeVertical =
        abs(-2.0f * lumaUp + lumaUpCorners) +
        abs(-2.0f * lumaCenter + lumaLeftRight) * 2.0f +
        abs(-2.0f * lumaDown + lumaDownCorners);

    bool isHorizontal = edgeHorizontal >= edgeVertical;

    float luma1 = isHorizontal ? lumaDown : lumaLeft;
    float luma2 = isHorizontal ? lumaUp   : lumaRight;

    float gradient1 = luma1 - lumaCenter;
    float gradient2 = luma2 - lumaCenter;

    bool  is1Steepest = abs(gradient1) >= abs(gradient2);
    float gradientScaled = 0.25f * max(abs(gradient1), abs(gradient2));

    float stepLength = isHorizontal ? texelSize.y : texelSize.x;
    float lumaLocalAverage = 0.0f;

    if (is1Steepest)
    {
        stepLength = -stepLength;
        lumaLocalAverage = 0.5f * (luma1 + lumaCenter);
    }
    else
    {
        lumaLocalAverage = 0.5f * (luma2 + lumaCenter);
    }

    float2 currentUv = uv;
    if (isHorizontal)
        currentUv.y += stepLength * 0.5f;
    else
        currentUv.x += stepLength * 0.5f;

    float2 offset = isHorizontal ? float2(texelSize.x, 0.0f) : float2(0.0f, texelSize.y);

    float2 uv1 = currentUv - offset;
    float2 uv2 = currentUv + offset;

    float lumaEnd1 = Luma(SampleColor(uv1)) - lumaLocalAverage;
    float lumaEnd2 = Luma(SampleColor(uv2)) - lumaLocalAverage;

    bool reached1 = abs(lumaEnd1) >= gradientScaled;
    bool reached2 = abs(lumaEnd2) >= gradientScaled;
    bool reachedBoth = reached1 && reached2;

    if (!reached1) uv1 -= offset;
    if (!reached2) uv2 += offset;

    if (!reachedBoth)
    {
        for (int i = 0; i < ITERATIONS; ++i)
        {
            if (!reached1)
            {
                lumaEnd1 = Luma(SampleColor(uv1)) - lumaLocalAverage;
                reached1 = abs(lumaEnd1) >= gradientScaled;
                if (!reached1) uv1 -= offset;
            }
            if (!reached2)
            {
                lumaEnd2 = Luma(SampleColor(uv2)) - lumaLocalAverage;
                reached2 = abs(lumaEnd2) >= gradientScaled;
                if (!reached2) uv2 += offset;
            }
            if (reached1 && reached2)
                break;
        }
    }

    float distance1 = isHorizontal ? (uv.x - uv1.x) : (uv.y - uv1.y);
    float distance2 = isHorizontal ? (uv2.x - uv.x) : (uv2.y - uv.y);

    bool  isDirection1 = distance1 < distance2;
    float distanceFinal = min(distance1, distance2);

    float edgeThickness = distance1 + distance2;
    float pixelOffset = -distanceFinal / edgeThickness + 0.5f;

    bool isLumaCenterSmaller = lumaCenter < lumaLocalAverage;
    bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0f) != isLumaCenterSmaller;
    float finalOffset = correctVariation ? pixelOffset : 0.0f;

    float lumaAverage = (1.0f / 12.0f) * (2.0f * (lumaDownUp + lumaLeftRight) +
        lumaLeftCorners + lumaRightCorners);
    float subPixelOffset1 = saturate(abs(lumaAverage - lumaCenter) / lumaRange);
    float subPixelOffset2 = (-2.0f * subPixelOffset1 + 3.0f) * subPixelOffset1 * subPixelOffset1;
    float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;

    finalOffset = max(finalOffset, subPixelOffsetFinal);

    float2 finalUv = uv;
    if (isHorizontal)
        finalUv.y += finalOffset * stepLength;
    else
        finalUv.x += finalOffset * stepLength;

    float3 finalColor = SampleColor(finalUv);

    outputTex[dispatchThreadID.xy] = float4(finalColor, 1.0f);
}