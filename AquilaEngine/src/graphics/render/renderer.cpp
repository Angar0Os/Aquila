#include <graphics/render/renderer.h>
#include <graphics/render/mesh.h>

#include <core/gpu/accelerationStructure.h>
#include <core/gpu/buffer.h>
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

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildGBufferPipeline()
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

	return std::make_unique<core::gpu::Pipeline>(m_device, pipelineInfo);
}

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildLightingPipeline()
{
	auto shaderCode = loaders::ReadFile("assets/shaders/lighting.spv");

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

	return std::make_unique<core::gpu::Pipeline>(m_device, pipelineInfo);
}

void Renderer::CreateUniformBuffers()
{
	uniformBuffers.clear();
	uniformBuffers.reserve(m_device.FRAMES_IN_FLIGHT);

	
	for (size_t i = 0; i < m_device.FRAMES_IN_FLIGHT; i++)
	{
		core::gpu::BufferCreateInfo bufferInfo{
			.size				= sizeof(UniformBufferObject),
			.usage				= core::gpu::utils::EBufferUsage::UniformBuffer,
			.memoryProperties	= core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
		};
		uniformBuffers.push_back(std::make_unique<core::gpu::Buffer>(m_device, bufferInfo));
	}
}

void Renderer::CreateAttachments()
{
	auto [width, height] = m_device.GetSwapchainExtent();

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

		gBufferColorAttachments[i].image = std::make_unique<core::gpu::Image>(m_device, info);
		gBufferColorAttachments[i].texture = std::make_unique<core::gpu::Texture>(m_device, *gBufferColorAttachments[i].image);

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

		gBufferDepthAttachment.image = std::make_unique<core::gpu::Image>(m_device, depthInfo);
		gBufferDepthAttachment.texture = std::make_unique<core::gpu::Texture>(m_device, *gBufferDepthAttachment.image);
	}

	core::gpu::ImageCreateInfo lightingInfo{
		.width				= width,
		.height				= height,
		.mipLevels			= 1,
		.format				= core::gpu::utils::ETextureFormat::RGBA8_SRGB,
		.tiling				= core::gpu::utils::EImageTiling::Optimal,
		.usage				= core::gpu::utils::EImageUsage::ColorAttachment | core::gpu::utils::EImageUsage::TransferSrc | core::gpu::utils::EImageUsage::Sampled,
		.memoryProperties	= core::gpu::utils::EMemoryProperty::DeviceLocal,
		.samples			= core::gpu::utils::ESampleCount::e1
	};

	lightingColorAttachments.resize(1);
	lightingColorAttachments[0].image = std::make_unique<core::gpu::Image>(m_device, lightingInfo);
	lightingColorAttachments[0].texture = std::make_unique<core::gpu::Texture>(m_device, *lightingColorAttachments[0].image);
}

void Renderer::CreateDescriptorSetLayout()
{
	core::gpu::DescriptorSetLayoutBinding gBufferUboBinding
	{
		.binding = 0,
		.descriptorType = core::gpu::utils::EDescriptorType::UniformBuffer,
		.stageFlags = core::gpu::utils::EShaderStage::Vertex | core::gpu::utils::EShaderStage::Fragment
	};

	gBufferDsLayouts.clear();
	gBufferDsLayouts.push_back(std::make_unique<core::gpu::DescriptorSetLayout>(
		m_device,
		core::gpu::DescriptorSetLayoutCreateInfo{ .bindings = { gBufferUboBinding } }
	));

	core::gpu::DescriptorSetLayoutBinding lightingUboBinding{
		.binding = 0,
		.descriptorType = core::gpu::utils::EDescriptorType::UniformBuffer,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding albedoBinding{
		.binding = 1,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding normalBinding{
		.binding = 2,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding depthBinding{
		.binding = 3,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding tlasBinding{
		.binding = 4,
		.descriptorType = core::gpu::utils::EDescriptorType::AccelerationStructure,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding envMapBinding{
		.binding = 5,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};
	core::gpu::DescriptorSetLayoutBinding iblBinding{
		.binding = 6,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};

	core::gpu::DescriptorSetLayoutCreateInfo layoutInfo{
		.bindings = { lightingUboBinding, albedoBinding, normalBinding,
					  depthBinding, tlasBinding, envMapBinding, iblBinding }
	};

	lightingDsLayouts.clear();
	lightingDsLayouts.push_back(std::make_unique<core::gpu::DescriptorSetLayout>(m_device, layoutInfo));
}

void Renderer::CreateMaterialLayout()
{
	core::gpu::DescriptorSetLayoutBinding materialUBOBinding{
		.binding = 0,
		.descriptorType = core::gpu::utils::EDescriptorType::UniformBuffer,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};

	core::gpu::DescriptorSetLayoutBinding albedoBinding{
		.binding = 1,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};

	core::gpu::DescriptorSetLayoutBinding normalBinding{
		.binding = 2,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};

	core::gpu::DescriptorSetLayoutBinding roughMetalBinding{
		.binding = 3,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};

	materialLayout = std::make_unique<core::gpu::DescriptorSetLayout>(
		m_device,
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

void Renderer::CreateDescriptorSets()
{
	auto cmdBuf = m_device.AcquireCommandBuffer();

	gBufferDescriptorSets.clear();
	lightingDescriptorSets.clear();

	gBufferDescriptorSets.reserve(m_device.FRAMES_IN_FLIGHT);
	lightingDescriptorSets.reserve(m_device.FRAMES_IN_FLIGHT);

	for (uint32_t i = 0; i < m_device.FRAMES_IN_FLIGHT; ++i)
	{
		auto ds = std::make_unique<core::gpu::DescriptorSet>(m_device, *gBufferDsLayouts[0]);
		ds->Bind(0, *uniformBuffers[i]);
		ds->Update(m_device);
		gBufferDescriptorSets.push_back(std::move(ds));
	}

	for (uint32_t i = 0; i < m_device.FRAMES_IN_FLIGHT; ++i)
	{
		auto ds = std::make_unique<core::gpu::DescriptorSet>(m_device, *lightingDsLayouts[0]);
		ds->Bind(0, *uniformBuffers[i]);
		ds->Update(m_device);
		lightingDescriptorSets.push_back(std::move(ds));
	}

	cmdBuf->Bind(*gBufferDescriptorSets[m_device.currentFrame], *m_gBufferPipeline, 0u);
	cmdBuf->Bind(*lightingDescriptorSets[m_device.currentFrame], *m_lightingPipeline, 0u);
}

void Renderer::UpdateDescriptorSets()
{
	if (!tlas) return;

	lightingDescriptorSets[m_device.currentFrame]->Bind(4, *tlas);
	lightingDescriptorSets[m_device.currentFrame]->Update(m_device);
}

Renderer::Renderer(const core::gpu::Device& _device)
	: m_device(_device)
{
	CreateUniformBuffers();
	// TODO : We need to create IB/VB properly

	CreateAttachments();
	CreateDescriptorSetLayout();
	CreateMaterialLayout();

	m_gBufferPipeline = BuildGBufferPipeline();
	m_lightingPipeline = BuildLightingPipeline();

	CreateDescriptorSets();
	//CreateFallbackMaterial()
}

Renderer::~Renderer() = default;

void Renderer::Render(core::gpu::CommandBuffer& _cmdBuf)
{

}