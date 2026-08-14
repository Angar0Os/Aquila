struct GIConstants
{
   
};

[[vk::binding(0, 0)]]
ConstantBuffer<GIConstants> gGIConstants;

[[vk::binding(1, 0)]]
Texture2D<float> gDepth;

[[vk::binding(2, 0)]]
Texture2D<float4> gNormal;

[[vk::binding(3, 0)]]
Texture2D<float4> gAlbedo;

[[vk::binding(4, 0)]]
RaytracingAccelerationStructure gTLAS;

[[vk::binding(5, 0)]]
Texture2D<float4> gEnvironment;

[[vk::binding(6, 0)]]
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> gGIOutput;

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchThreadID.xy;

}