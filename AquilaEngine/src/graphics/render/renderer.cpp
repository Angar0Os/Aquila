#include <graphics/render/renderer.h>

#include <core/gpu/accelerationStructure.h>
#include <core/gpu/commandBuffer.h>
#include <core/gpu/device.h>
#include <core/gpu/descriptorSet.h>
#include <core/gpu/descriptorSetLayout.h>
#include <core/gpu/image.h>
#include <core/gpu/pipeline.h>
#include <core/gpu/texture.h>

#include <core/gpu/utils/enums.h>

#include <loaders/shaderLoader.h>


using namespace graphics::render;

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildGBufferPipeline(const core::gpu::Device& _device)
{
	auto shaderCode = loaders::ReadFile("assets/shaders/gBuffer.spv");

	core::gpu::VertexInputBinding vertexBinding{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = core::gpu::utils::EVertexInputRate::Vertex
	};

	std::vector<core::gpu::VertexInputAttribute> vertexAttributes = {
		{0, 0, core::gpu::utils::ETextureFormat::RGB32_Float, offsetof(Vertex, position)},
		{1, 0, core::gpu::utils::ETextureFormat::RGB32_Float, offsetof(Vertex, normal)},
		{2, 0, core::gpu::utils::ETextureFormat::RG32_Float,  offsetof(Vertex, uv)}
	};

	std::vector<core::gpu::PushConstantRange> pushConstants = {
		{
			.stageFlags = static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Vertex),
			.offset = 0,
			.size = sizeof(GBufferPushConstants)
		}
	};

	core::gpu::PipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaderStages = {
		{core::gpu::utils::EShaderStageFlags::Vertex,   shaderCode, "vertMain"},
		{core::gpu::utils::EShaderStageFlags::Fragment, shaderCode, "fragMain"}
	};
	pipelineInfo.vertexBindings = { vertexBinding };
	pipelineInfo.vertexAttributes = vertexAttributes;
	pipelineInfo.topology = core::gpu::utils::EPrimitiveTopology::TriangleList;
	pipelineInfo.polygonMode = core::gpu::utils::EPolygonMode::Fill;
	pipelineInfo.cullMode = core::gpu::utils::ECullMode::Back;
	pipelineInfo.frontFace = core::gpu::utils::EFrontFace::Clockwise;
	pipelineInfo.depthTestEnable = true;
	pipelineInfo.depthWriteEnable = true;
	pipelineInfo.depthCompareOp = core::gpu::utils::ECompareOp::Less;
	pipelineInfo.blendEnable = false;
	pipelineInfo.samples = core::gpu::utils::ESampleCount::e1;
	pipelineInfo.colorAttachmentFormats = { core::gpu::utils::ETextureFormat::RGBA8_SRGB, core::gpu::utils::ETextureFormat::RGBA16_Float, core::gpu::utils::ETextureFormat::RG32_Float };
	pipelineInfo.depthAttachmentFormat = core::gpu::utils::ETextureFormat::Depth32F;
	pipelineInfo.descriptorSetLayouts = { gBufferDsLayouts[0].get(), materialLayout.get() };
	pipelineInfo.pushConstantRanges = pushConstants;
	pipelineInfo.dynamicStates = { core::gpu::utils::EDynamicState::Viewport, core::gpu::utils::EDynamicState::Scissor };

	return std::make_unique<core::gpu::Pipeline>(&_device, pipelineInfo);
}

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildLightingPipeline(const core::gpu::Device& _device)
{
	auto shaderCode = loaders::ReadFile("bin/assets/shaders/lighting.spv");

	core::gpu::PipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaderStages = {
		{core::gpu::utils::EShaderStageFlags::Vertex,   shaderCode, "vertMain"},
		{core::gpu::utils::EShaderStageFlags::Fragment, shaderCode, "fragMain"}
	};
	pipelineInfo.vertexBindings = {};
	pipelineInfo.vertexAttributes = {};
	pipelineInfo.topology = core::gpu::utils::EPrimitiveTopology::TriangleList;
	pipelineInfo.polygonMode = core::gpu::utils::EPolygonMode::Fill;
	pipelineInfo.cullMode = core::gpu::utils::ECullMode::None;
	pipelineInfo.frontFace = core::gpu::utils::EFrontFace::Clockwise;
	pipelineInfo.depthTestEnable = false;
	pipelineInfo.depthWriteEnable = false;
	pipelineInfo.depthCompareOp = core::gpu::utils::ECompareOp::Always;
	pipelineInfo.blendEnable = false;
	pipelineInfo.samples = core::gpu::utils::ESampleCount::e1;
	pipelineInfo.colorAttachmentFormats = { core::gpu::utils::ETextureFormat::RGBA8_SRGB };
	pipelineInfo.depthAttachmentFormat = core::gpu::utils::ETextureFormat::Undefined;
	pipelineInfo.descriptorSetLayouts = { lightingDsLayouts[0].get() };
	pipelineInfo.pushConstantRanges = {};
	pipelineInfo.dynamicStates = { core::gpu::utils::EDynamicState::Viewport, core::gpu::utils::EDynamicState::Scissor };

	return std::make_unique<core::gpu::Pipeline>(&_device, pipelineInfo);
}

void Renderer::CreateAttachments(core::gpu::Device& _device)
{
	auto [width, height] = _device.GetSwapchainExtent();

	const core::gpu::utils::ETextureFormat colorFormats[] = {
		core::gpu::utils::ETextureFormat::RGBA8_SRGB,
		core::gpu::utils::ETextureFormat::RGBA16_Float,
		core::gpu::utils::ETextureFormat::RG32_Float
	};

	gBufferColorAttachments.clear();
	gBufferColorAttachments.resize(3);

	for (int i = 0; i < 3; ++i)
	{
		core::gpu::ImageCreateInfo info
		{
			.width				= width,
			.height				= height,
			.mipLevels			= 1,
			.format				= colorFormats[i],
			.tiling				= core::gpu::utils::EImageTiling::Optimal,
			.usage				= core::gpu::utils::EImageUsage::ColorAttachment | core::gpu::utils::EImageUsage::Sampled,
			.memoryProperties	= core::gpu::utils::EMemoryProperty::DeviceLocal,
			.samples			= core::gpu::utils::ESampleCount::e1
		};

		gBufferColorAttachments[i].image	= std::make_unique<core::gpu::Image>(&_device, info);
		gBufferColorAttachments[i].texture	= std::make_unique<core::gpu::Texture>(_device, *gBufferColorAttachments[i].image);

		core::gpu::ImageCreateInfo depthInfo
		{
			.width				= width,
			.height				= height,
			.mipLevels			= 1,
			.format				= core::gpu::utils::ETextureFormat::Depth32F,
			.tiling				= core::gpu::utils::EImageTiling::Optimal,
			.usage				= core::gpu::utils::EImageUsage::DepthStencilAttachment | core::gpu::utils::EImageUsage::Sampled,
			.memoryProperties	= core::gpu::utils::EMemoryProperty::DeviceLocal,
			.samples			= core::gpu::utils::ESampleCount::e1
		};

		gBufferDepthAttachment.image	= std::make_unique<core::gpu::Image>(&_device, depthInfo);
		gBufferDepthAttachment.texture	= std::make_unique<core::gpu::Texture>(_device, *gBufferDepthAttachment.image);
	}
}

void Renderer::CreateDescriptorSetLayout(core::gpu::Device& _device)
{
	core::gpu::DescriptorSetLayoutBinding uboBinding
	{
		.binding		= 0,
		.descriptorType = core::gpu::utils::EDescriptorType::UniformBuffer,
		.stageFlags		= core::gpu::utils::EShaderStage::Vertex | core::gpu::utils::EShaderStage::Fragment
	};

	gBufferDsLayouts.clear();
	gBufferDsLayouts.push_back(std::make_unique<core::gpu::DescriptorSetLayout>(
		&_device,
		core::gpu::DescriptorSetLayoutCreateInfo{ .bindings = { uboBinding } }
	));

	core::gpu::DescriptorSetLayoutBinding uboBinding{
		.binding		= 0,
		.descriptorType = core::gpu::utils::EDescriptorType::UniformBuffer,
		.stageFlags		= core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding albedoBinding {
		.binding		= 1,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags		= core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding normalBinding {
		.binding		= 2,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags		= core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding depthBinding {
		.binding		= 3,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags		= core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding tlasBinding {
		.binding		= 4,
		.descriptorType = core::gpu::utils::EDescriptorType::AccelerationStructure,
		.stageFlags		= core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding envMapBinding {
		.binding		= 5,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags		= core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding iblBinding {
		.binding		= 6,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags		= core::gpu::utils::EShaderStage::Fragment
	};

	core::gpu::DescriptorSetLayoutCreateInfo layoutInfo {
		.bindings = { uboBinding, albedoBinding, normalBinding,
					  depthBinding, tlasBinding, envMapBinding, iblBinding }
	};

	lightingDsLayouts.clear();
	lightingDsLayouts.push_back(std::make_unique<core::gpu::DescriptorSetLayout>(&_device, layoutInfo));
}

void Renderer::CreateMaterialLayout(core::gpu::Device& _device)
{
	core::gpu::DescriptorSetLayoutBinding materialUBOBinding {
		.binding			= 0,
		.descriptorType		= core::gpu::utils::EDescriptorType::UniformBuffer,
		.stageFlags			= core::gpu::utils::EShaderStage::Fragment
	};

	core::gpu::DescriptorSetLayoutBinding albedoBinding {
		.binding		= 1,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags		= core::gpu::utils::EShaderStage::Fragment
	};

	core::gpu::DescriptorSetLayoutBinding normalBinding {
		.binding		= 2,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags		= core::gpu::utils::EShaderStage::Fragment
	};

	core::gpu::DescriptorSetLayoutBinding roughMetalBinding{
		.binding		= 3,
		.descriptorType	= core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags		= core::gpu::utils::EShaderStage::Fragment
	};

	materialLayout = std::make_unique<core::gpu::DescriptorSetLayout>(
		&_device,
		core::gpu::DescriptorSetLayoutCreateInfo{
			.bindings = {
				materialUBOBinding,
				albedoBinding,
				normalBinding,
				roughMetalBinding
			}
		}
	);
}

void Renderer::CreateDescriptorSets(const core::gpu::Device& _device, const core::gpu::CommandBuffer& _cmdBuf)
{
	gBufferDescriptorSets.clear();
	lightingDescriptorSets.clear();

	gBufferDescriptorSets.reserve(_device.FRAMES_IN_FLIGHT);
	lightingDescriptorSets.reserve(_device.FRAMES_IN_FLIGHT);
	
	for (uint32_t i = 0; i < _device.FRAMES_IN_FLIGHT; ++i)
	{
		auto ds = std::make_unique<core::gpu::DescriptorSet>(&_device, gBufferDescriptorSets[0].get());
		ds->Bind(0, *gBufferUniformBuffers[i]);
		ds->Update(_device);
		gBufferDescriptorSets.push_back(std::move(ds));
	} 

	for (uint32_t i = 0; i < _device.FRAMES_IN_FLIGHT; ++i)
	{
		auto ds = std::make_unique<core::gpu::DescriptorSet>(&_device, lightingDsLayouts[0].get());
		ds->Bind(0, *lightingUniformBuffers[i]);
		ds->Update(_device);
		lightingDescriptorSets.push_back(std::move(ds));
	}
	
	_cmdBuf.Bind(*gBufferDescriptorSets[_device.currentFrame], *m_gBufferPipeline, 0u);
	_cmdBuf.Bind(*lightingDescriptorSets[_device.currentFrame], *m_lightingPipeline, 0u);
}

void Renderer::UpdateDescriptorSets(const core::gpu::Device& _device)
{
	if (!tlas) return;

	lightingDescriptorSets[_device.currentFrame]->Bind(4, *tlas);
	lightingDescriptorSets[_device.currentFrame]->Update(_device);
}

Renderer::Renderer(core::gpu::Device _device, core::gpu::CommandBuffer _cmdBuf)
{
	CreateAttachments(_device);
	CreateDescriptorSetLayout(_device);
	CreateMaterialLayout(_device);

	m_gBufferPipeline = BuildGBufferPipeline(_device);
	m_lightingPipeline = BuildLightingPipeline(_device);

	CreateDescriptorSets(_device, _cmdBuf);
	//CreateFallbackMaterial()
}

Renderer::~Renderer() = default;

void Renderer::Render(core::gpu::CommandBuffer& _cmdBuf)
{
	
}
