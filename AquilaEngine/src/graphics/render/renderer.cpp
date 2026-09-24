#include <graphics/render/renderer.h>
#include <graphics/render/mesh.h>
#include <graphics/render/material.h>
#include <graphics/render/textureLibrary.h>

#include <core/gpu/accelerationStructure.h>
#include <core/gpu/buffer.h>
#include <core/gpu/commandBuffer.h>
#include <core/gpu/descriptorSet.h>
#include <core/gpu/descriptorSetLayout.h>
#include <core/gpu/image.h>
#include <core/gpu/pipeline.h>
#include <core/gpu/texture.h>

#include <core/gpu/utils/enums.h>
#include <core/gpu/utils/converters.h>
#include <graphics/render/graph/lambdaPass.h>

#include <graphics/render/graph/renderGraph.h>
#include <graphics/render/graph/resourceAllocator.h>

#include <core/debug/debug.h>
#include <core/debug/debugNames.h>

#include <loaders/shaderLoader.h>

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>

using namespace graphics::render;
using namespace graphics::render::graph;

graphics::render::graph::ResourceState ToResourceState(core::gpu::utils::EImageLayout _layout)
{
	using namespace core::gpu::utils;
	using namespace graphics::render::graph;

	switch (_layout)
	{
	case EImageLayout::Undefined:				return ResourceState::Undefined;
	case EImageLayout::ColorAttachment:			return ResourceState::RenderTarget;
	case EImageLayout::DepthStencilAttachment:	return ResourceState::DepthWrite;
	case EImageLayout::ShaderReadOnly:			return ResourceState::ShaderRead;
	case EImageLayout::General:					return ResourceState::UnorderedAccess;
	case EImageLayout::TransferSrc:				return ResourceState::TransferSrc;
	case EImageLayout::TransferDst:				return ResourceState::TransferDst;
	case EImageLayout::Present:					return ResourceState::Present;
	}

	return graphics::render::graph::ResourceState::Undefined;
}

// TODO : I need to set a name for my render Passes.

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildGBufferPipeline()
{
	auto shaderCode = loaders::ReadFile("assets/shaders/gBuffer.slang.spv");

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
		.stageFlags = static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Vertex | core::gpu::utils::EShaderStageFlags::Fragment),
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
	pipelineInfo.pipelineType = core::gpu::utils::EPipelineType::Graphics;
	pipelineInfo.vertexAttributes = vertexAttributes;
	pipelineInfo.topology = core::gpu::utils::EPrimitiveTopology::TriangleList;
	pipelineInfo.polygonMode = core::gpu::utils::EPolygonMode::Fill;
	pipelineInfo.cullMode = core::gpu::utils::ECullMode::Back;
	pipelineInfo.frontFace = core::gpu::utils::EFrontFace::CounterClockwise;
	pipelineInfo.depthTestEnable = true;
	pipelineInfo.depthWriteEnable = true;
	pipelineInfo.depthCompareOp = core::gpu::utils::ECompareOp::Less;
	pipelineInfo.blendEnable = false;
	pipelineInfo.samples = core::gpu::utils::ESampleCount::e1;
	pipelineInfo.colorAttachmentFormats = { 
		core::gpu::utils::ETextureFormat::RGBA8_SRGB,
		core::gpu::utils::ETextureFormat::RGBA16_Float, 
		core::gpu::utils::ETextureFormat::RG32_Float,
		core::gpu::utils::ETextureFormat::RGBA16_Float
	};
	pipelineInfo.depthAttachmentFormat = core::gpu::utils::ETextureFormat::Depth32F;
	pipelineInfo.descriptorSetLayouts = { gBufferDsLayouts[0].get(), materialLayout.get() };
	pipelineInfo.pushConstantRanges = pushConstants;
	pipelineInfo.dynamicStates = { core::gpu::utils::EDynamicState::Viewport, core::gpu::utils::EDynamicState::Scissor };

	return std::make_unique<core::gpu::Pipeline>(m_device, pipelineInfo);
}

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildShadowPipeline()
{
	auto shaderCode = loaders::ReadFile("assets/shaders/shadow.slang.spv");

	std::vector<core::gpu::PushConstantRange> pushConstants = {
		{
			.stageFlags = static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
			.offset = 0,
			.size = sizeof(ShadowPushConstants)
		}
	};

	core::gpu::PipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaderStages = { { core::gpu::utils::EShaderStageFlags::Compute, shaderCode, "cs_main" } };
	pipelineInfo.pipelineType = core::gpu::utils::EPipelineType::Compute;
	pipelineInfo.descriptorSetLayouts = { shadowDsLayouts[0].get() };
	pipelineInfo.pushConstantRanges = pushConstants;

	return std::make_unique<core::gpu::Pipeline>(m_device, pipelineInfo);
}

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildGIPipeline()
{
	auto shaderCode = loaders::ReadFile("assets/shaders/gi.slang.spv");

	std::vector<core::gpu::PushConstantRange> pushConstants = {
		{
			.stageFlags = static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
			.offset = 0,
			.size = sizeof(GIPushConstants)
		}
	};

	core::gpu::PipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaderStages = { { core::gpu::utils::EShaderStageFlags::Compute, shaderCode, "cs_main" } };
	pipelineInfo.pipelineType = core::gpu::utils::EPipelineType::Compute;
	pipelineInfo.descriptorSetLayouts = { giDsLayouts[0].get() };
	pipelineInfo.pushConstantRanges = pushConstants;

	return std::make_unique<core::gpu::Pipeline>(m_device, pipelineInfo);
}

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildResolvePipeline()
{
	auto shaderCode = loaders::ReadFile("assets/shaders/resolve.slang.spv");

	core::gpu::PipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaderStages = { { core::gpu::utils::EShaderStageFlags::Compute, shaderCode, "cs_main" } };
	pipelineInfo.pipelineType = core::gpu::utils::EPipelineType::Compute;
	pipelineInfo.descriptorSetLayouts = { resolveDsLayouts[0].get() };

	std::vector<core::gpu::PushConstantRange> pushConstants = {
		{
			.stageFlags = static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
			.offset = 0, 
			.size = sizeof(ResolvePushConstants)
		}
	};
	pipelineInfo.pushConstantRanges = pushConstants;

	return std::make_unique<core::gpu::Pipeline>(m_device, pipelineInfo);
}

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildFXAAPipeline()
{
	auto shaderCode = loaders::ReadFile("assets/shaders/fxaa.slang.spv");

	std::vector<core::gpu::PushConstantRange> pushConstants = {
		{.stageFlags = static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
		  .offset = 0, .size = sizeof(FXAAPushConstants) }
	};

	core::gpu::PipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaderStages = { { core::gpu::utils::EShaderStageFlags::Compute, shaderCode, "cs_main" } };
	pipelineInfo.pipelineType = core::gpu::utils::EPipelineType::Compute;
	pipelineInfo.descriptorSetLayouts = { fxaaDsLayouts[0].get() };
	pipelineInfo.pushConstantRanges = pushConstants;

	return std::make_unique<core::gpu::Pipeline>(m_device, pipelineInfo);
}

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildATrousPipeline()
{
	auto shaderCode = loaders::ReadFile("assets/shaders/atrous.slang.spv");

	std::vector<core::gpu::PushConstantRange> pushConstants = {
		{
			.stageFlags = static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
			.offset = 0, .size = sizeof(ATrousPushConstants)
		}
	};

	core::gpu::PipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaderStages = { { core::gpu::utils::EShaderStageFlags::Compute, shaderCode, "cs_main" } };
	pipelineInfo.pipelineType = core::gpu::utils::EPipelineType::Compute;
	pipelineInfo.descriptorSetLayouts = { atrousDsLayouts[0].get() };
	pipelineInfo.pushConstantRanges = pushConstants;

	return std::make_unique<core::gpu::Pipeline>(m_device, pipelineInfo);
}

void Renderer::LoadEnvironmentMaps()
{
	m_envMap.image = TextureLibrary::LoadHDRImage(m_device, "assets/textures/skyboxes/HDR_artificial_planet.hdr");
	AQUILA_CHECK(m_envMap.image != nullptr, "Renderer: Failed to load environment map");

	m_envMap.texture = std::make_unique<core::gpu::Texture>(m_device, *m_envMap.image);

	AQUILA_SET_DEBUG_NAME(m_device, *m_envMap.image, "Renderer::EnvironmentMap::Image");
	AQUILA_SET_DEBUG_NAME(m_device, *m_envMap.texture, "Renderer::EnvironmentMap::Texture");
}

void Renderer::CreateUniformBuffers()
{
	uniformBuffers.clear();
	uniformBuffers.reserve(m_device.FRAMES_IN_FLIGHT);


	for (size_t i = 0; i < m_device.FRAMES_IN_FLIGHT; i++)
	{
		core::gpu::BufferCreateInfo bufferInfo {
			.size = sizeof(UniformBufferObject),
			.usage = core::gpu::utils::EBufferUsage::UniformBuffer,
			.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
		};
		uniformBuffers.push_back(std::make_unique<core::gpu::Buffer>(m_device, bufferInfo));

		AQUILA_SET_DEBUG_NAME(m_device, *uniformBuffers.back(), "Renderer::UniformBuffer[" + std::to_string(i) + "]");
	}

	core::gpu::BufferCreateInfo bufferInfo {
		.size = sizeof(GPULight) * MAX_LIGHTS,
		.usage = core::gpu::utils::EBufferUsage::StorageBuffer,
		.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
	};
	lightBuffer = std::make_unique<core::gpu::Buffer>(m_device, bufferInfo);
	AQUILA_SET_DEBUG_NAME(m_device, *lightBuffer, "Renderer::LightBuffer");

	core::gpu::BufferCreateInfo meshTableInfo{
		.size = sizeof(MeshTableEntry) * MAX_MESHES,
		.usage = core::gpu::utils::EBufferUsage::StorageBuffer,
		.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
	};
	m_meshTableBuffer = std::make_unique<core::gpu::Buffer>(m_device, meshTableInfo);
	AQUILA_SET_DEBUG_NAME(m_device, *m_meshTableBuffer, "Renderer::MeshTableBuffer");
}

void Renderer::CreateAttachments(uint32_t width, uint32_t height)
{
	m_renderWidth = width;
	m_renderHeight = height;

	const core::gpu::utils::ETextureFormat colorFormats[] = {
		core::gpu::utils::ETextureFormat::RGBA8_SRGB,
		core::gpu::utils::ETextureFormat::RGBA16_Float,
		core::gpu::utils::ETextureFormat::RG32_Float,
		core::gpu::utils::ETextureFormat::RGBA16_Float
	};

	gBufferColorAttachments.clear();
	gBufferColorAttachments.resize(4);

	for (int i = 0; i < 4; ++i)
	{
		core::gpu::ImageCreateInfo info
		{
			.width = width,
			.height = height,
			.mipLevels = 1,
			.format = colorFormats[i],
			.tiling = core::gpu::utils::EImageTiling::Optimal,
			.usage = core::gpu::utils::EImageUsage::ColorAttachment | core::gpu::utils::EImageUsage::Sampled,
			.memoryProperties = core::gpu::utils::EMemoryProperty::DeviceLocal,
			.samples = core::gpu::utils::ESampleCount::e1
		};

		gBufferColorAttachments[i].image = std::make_unique<core::gpu::Image>(m_device, info);
		gBufferColorAttachments[i].texture = std::make_unique<core::gpu::Texture>(m_device, *gBufferColorAttachments[i].image, core::gpu::utils::ETextureFilter::Nearest);

		AQUILA_SET_DEBUG_NAME(m_device, *gBufferColorAttachments[i].image, "Renderer::GBuffer[" + std::to_string(i) + "]::Image");
		AQUILA_SET_DEBUG_NAME(m_device, *gBufferColorAttachments[i].texture, "Renderer::GBuffer[" + std::to_string(i) + "]::Texture");
	}

	core::gpu::ImageCreateInfo depthInfo
	{
		.width = width,
		.height = height,
		.mipLevels = 1,
		.format = core::gpu::utils::ETextureFormat::Depth32F,
		.tiling = core::gpu::utils::EImageTiling::Optimal,
		.usage = core::gpu::utils::EImageUsage::DepthStencilAttachment | core::gpu::utils::EImageUsage::Sampled,
		.memoryProperties = core::gpu::utils::EMemoryProperty::DeviceLocal,
		.samples = core::gpu::utils::ESampleCount::e1
	};

	gBufferDepthAttachment.image = std::make_unique<core::gpu::Image>(m_device, depthInfo);
	gBufferDepthAttachment.texture = std::make_unique<core::gpu::Texture>(m_device, *gBufferDepthAttachment.image, core::gpu::utils::ETextureFilter::Nearest);

	AQUILA_SET_DEBUG_NAME(m_device, *gBufferDepthAttachment.image, "Renderer::GBufferDepth::Image");
	AQUILA_SET_DEBUG_NAME(m_device, *gBufferDepthAttachment.texture, "Renderer::GBufferDepth::Texture");

	core::gpu::ImageCreateInfo shadowInfo
	{
		.width = width,
		.height = height,
		.mipLevels = 1,
		.format = core::gpu::utils::ETextureFormat::R32_Float,
		.tiling = core::gpu::utils::EImageTiling::Optimal,
		.usage = core::gpu::utils::EImageUsage::Storage | core::gpu::utils::EImageUsage::Sampled,
		.memoryProperties = core::gpu::utils::EMemoryProperty::DeviceLocal,
		.samples = core::gpu::utils::ESampleCount::e1
	};

	shadowMaskAttachment.image = std::make_unique<core::gpu::Image>(m_device, shadowInfo);
	shadowMaskAttachment.texture = std::make_unique<core::gpu::Texture>(m_device, *shadowMaskAttachment.image);

	AQUILA_SET_DEBUG_NAME(m_device, *shadowMaskAttachment.image, "Renderer::ShadowMask::Image");
	AQUILA_SET_DEBUG_NAME(m_device, *shadowMaskAttachment.texture, "Renderer::ShadowMask::Texture");

	core::gpu::ImageCreateInfo giInfo
	{
		.width = width,
		.height = height,
		.mipLevels = 1,
		.format = core::gpu::utils::ETextureFormat::RGBA16_Float,
		.tiling = core::gpu::utils::EImageTiling::Optimal,
		.usage = core::gpu::utils::EImageUsage::Storage | core::gpu::utils::EImageUsage::Sampled,
		.memoryProperties = core::gpu::utils::EMemoryProperty::DeviceLocal,
		.samples = core::gpu::utils::ESampleCount::e1
	};

	giColorAttachments.resize(2);
	for (int i = 0; i < 2; ++i)
	{
		giColorAttachments[i].image = std::make_unique<core::gpu::Image>(m_device, giInfo);
		giColorAttachments[i].texture = std::make_unique<core::gpu::Texture>(m_device, *giColorAttachments[i].image);

		AQUILA_SET_DEBUG_NAME(m_device, *giColorAttachments[i].image, "Renderer::GI[" + std::to_string(i) + "]::Image");
		AQUILA_SET_DEBUG_NAME(m_device, *giColorAttachments[i].texture, "Renderer::GI[" + std::to_string(i) + "]::Texture");
	}

	auto commandBuffer = m_device.AcquireCommandBuffer();
	commandBuffer->Record([&]() {
		for (int i = 0; i < 2; ++i)
		{
			commandBuffer->TransitionImageLayout(
				*giColorAttachments[i].image,
				core::gpu::utils::EImageLayout::ShaderReadOnly,
				false
			);
		}
		});
	commandBuffer->Submit(m_device, true);
	m_device.ReleaseCommandBuffer(commandBuffer);

	core::gpu::ImageCreateInfo resolveInfo
	{
		.width = width,
		.height = height,
		.mipLevels = 1,
		.format = core::gpu::utils::ETextureFormat::RGBA16_Float,
		.tiling = core::gpu::utils::EImageTiling::Optimal,
		.usage = core::gpu::utils::EImageUsage::Storage | core::gpu::utils::EImageUsage::Sampled | core::gpu::utils::EImageUsage::TransferSrc,
		.memoryProperties = core::gpu::utils::EMemoryProperty::DeviceLocal,
		.samples = core::gpu::utils::ESampleCount::e1
	};

	resolveColorAttachments.resize(1);

	resolveColorAttachments[0].image = std::make_unique<core::gpu::Image>(m_device, resolveInfo);
	resolveColorAttachments[0].texture = std::make_unique<core::gpu::Texture>(m_device, *resolveColorAttachments[0].image);

	AQUILA_SET_DEBUG_NAME(m_device, *resolveColorAttachments[0].image, "Renderer::Resolve::Image");
	AQUILA_SET_DEBUG_NAME(m_device, *resolveColorAttachments[0].texture, "Renderer::Resolve::Texture");

	core::gpu::ImageCreateInfo aaInfo
	{
		.width = width,
		.height = height,
		.mipLevels = 1,
		.format = core::gpu::utils::ETextureFormat::RGBA16_Float,
		.tiling = core::gpu::utils::EImageTiling::Optimal,
		.usage = core::gpu::utils::EImageUsage::Storage | core::gpu::utils::EImageUsage::Sampled | core::gpu::utils::EImageUsage::TransferSrc,
		.memoryProperties = core::gpu::utils::EMemoryProperty::DeviceLocal,
		.samples = core::gpu::utils::ESampleCount::e1
	};

	aaColorAttachments.resize(1);
	aaColorAttachments[0].image = std::make_unique<core::gpu::Image>(m_device, aaInfo);
	aaColorAttachments[0].texture = std::make_unique<core::gpu::Texture>(m_device, *aaColorAttachments[0].image);

	AQUILA_SET_DEBUG_NAME(m_device, *aaColorAttachments[0].image, "Renderer::AntiAliasing::Image");
	AQUILA_SET_DEBUG_NAME(m_device, *aaColorAttachments[0].texture, "Renderer::AntiAliasing::Texture");

	core::gpu::ImageCreateInfo atrousInfo
	{
		.width = width,
		.height = height,
		.mipLevels = 1,
		.format = core::gpu::utils::ETextureFormat::RGBA16_Float,
		.tiling = core::gpu::utils::EImageTiling::Optimal,
		.usage = core::gpu::utils::EImageUsage::Storage | core::gpu::utils::EImageUsage::Sampled,
		.memoryProperties = core::gpu::utils::EMemoryProperty::DeviceLocal,
		.samples = core::gpu::utils::ESampleCount::e1
	};

	atrousAttachments.resize(2);
	for (int i = 0; i < 2; ++i)
	{
		atrousAttachments[i].image = std::make_unique<core::gpu::Image>(m_device, atrousInfo);
		atrousAttachments[i].texture = std::make_unique<core::gpu::Texture>(m_device, *atrousAttachments[i].image, core::gpu::utils::ETextureFilter::Nearest);

		AQUILA_SET_DEBUG_NAME(m_device, *atrousAttachments[i].image, "Renderer::ATrous[" + std::to_string(i) + "]::Image");
		AQUILA_SET_DEBUG_NAME(m_device, *atrousAttachments[i].texture, "Renderer::ATrous[" + std::to_string(i) + "]::Texture");
	}

	auto atrousInitCmd = m_device.AcquireCommandBuffer();
	atrousInitCmd->Record([&]() {
		for (int i = 0; i < 2; ++i)
		{
			atrousInitCmd->TransitionImageLayout(
				*atrousAttachments[i].image,
				core::gpu::utils::EImageLayout::ShaderReadOnly,
				false
			);
		}
		});
	atrousInitCmd->Submit(m_device, true);
	m_device.ReleaseCommandBuffer(atrousInitCmd);
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

	core::gpu::DescriptorSetLayoutBinding shadowUboBinding{
		.binding = 0,
		.descriptorType = core::gpu::utils::EDescriptorType::UniformBuffer,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding shadowDepthBinding{
		.binding = 1,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding shadowNormalBinding{
		.binding = 2,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding shadowTlasBinding{
		.binding = 3,
		.descriptorType = core::gpu::utils::EDescriptorType::AccelerationStructure,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding shadowOutputBinding{
		.binding = 4,
		.descriptorType = core::gpu::utils::EDescriptorType::StorageImage,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutCreateInfo shadowLayoutInfo{
		.bindings = { shadowUboBinding, shadowDepthBinding, shadowNormalBinding, shadowTlasBinding, shadowOutputBinding }
	};

	shadowDsLayouts.clear();
	shadowDsLayouts.push_back(std::make_unique<core::gpu::DescriptorSetLayout>(m_device, shadowLayoutInfo));

	core::gpu::DescriptorSetLayoutBinding giUboBinding{
	.binding = 0,
	.descriptorType = core::gpu::utils::EDescriptorType::UniformBuffer,
	.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding giDepthBinding{
		.binding = 1,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding giNormalBinding{
		.binding = 2,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding giAlbedoBinding{
		.binding = 3,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding giTlasBinding{
		.binding = 4,
		.descriptorType = core::gpu::utils::EDescriptorType::AccelerationStructure,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding giEnvironmentBinding{
		.binding = 5,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding giOutputBinding{
		.binding = 6,
		.descriptorType = core::gpu::utils::EDescriptorType::StorageImage,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding giHistoryBinding{
		.binding = 7,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding giEnvironmentRawBinding{
		.binding = 9,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding giMeshTableBinding{
		.binding = 10, .descriptorType = core::gpu::utils::EDescriptorType::StorageBuffer,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding giMaterialTableBinding{
		.binding = 11, .descriptorType = core::gpu::utils::EDescriptorType::StorageBuffer,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding giBindlessTexturesBinding{
		.binding = 12,
		.descriptorCount = MAX_BINDLESS_TEXTURES,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute,
		.partiallyBound = true,
		.updateAfterBind = true
	};

	core::gpu::DescriptorSetLayoutCreateInfo giLayoutInfo{
		.bindings = { giUboBinding, giDepthBinding, giNormalBinding, giAlbedoBinding,
			giTlasBinding, giEnvironmentBinding, giOutputBinding, giHistoryBinding,
			giEnvironmentRawBinding,
			giMeshTableBinding, giMaterialTableBinding, giBindlessTexturesBinding }
	};

	giDsLayouts.clear();
	giDsLayouts.push_back(
		std::make_unique<core::gpu::DescriptorSetLayout>(
			m_device,
			giLayoutInfo
		)
	);

	core::gpu::DescriptorSetLayoutBinding resolveUboBinding {
		.binding = 0, .descriptorType = core::gpu::utils::EDescriptorType::UniformBuffer,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding resolveAlbedoBinding {
		.binding = 1, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding resolveNormalBinding {
		.binding = 2, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding resolveShadowBinding {
		.binding = 3, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding resolveOutputBinding {
		.binding = 4, .descriptorType = core::gpu::utils::EDescriptorType::StorageImage,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding resolveGiBinding {
		.binding = 5, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding resolveDepthBinding {
		.binding = 6, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding resolveEnvironmentRawBinding {
		.binding = 7, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding resolveEmissiveBinding {        
		.binding = 8, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	resolveDsLayouts.clear();
	resolveDsLayouts.push_back(std::make_unique<core::gpu::DescriptorSetLayout>(
		m_device,
		core::gpu::DescriptorSetLayoutCreateInfo{
			.bindings = { resolveUboBinding, resolveAlbedoBinding, resolveNormalBinding,
						  resolveShadowBinding, resolveOutputBinding, resolveGiBinding,
						  resolveDepthBinding, resolveEnvironmentRawBinding, resolveEmissiveBinding  }
		}
	));

	core::gpu::DescriptorSetLayoutBinding fxaaInputBinding{
		.binding = 0, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding fxaaOutputBinding{
		.binding = 1, .descriptorType = core::gpu::utils::EDescriptorType::StorageImage,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	fxaaDsLayouts.clear();
	fxaaDsLayouts.push_back(std::make_unique<core::gpu::DescriptorSetLayout>(
		m_device,
		core::gpu::DescriptorSetLayoutCreateInfo{ .bindings = { fxaaInputBinding, fxaaOutputBinding } }
	));

	core::gpu::DescriptorSetLayoutBinding atrousInputBinding{
		.binding = 0, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding atrousDepthBinding{
		.binding = 1, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding atrousNormalBinding{
		.binding = 2, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding atrousOutputBinding{
		.binding = 3, .descriptorType = core::gpu::utils::EDescriptorType::StorageImage,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	atrousDsLayouts.clear();
	atrousDsLayouts.push_back(std::make_unique<core::gpu::DescriptorSetLayout>(
		m_device,
		core::gpu::DescriptorSetLayoutCreateInfo{
			.bindings = { atrousInputBinding, atrousDepthBinding, atrousNormalBinding, atrousOutputBinding }
		}
	));
}

void Renderer::CreateMaterialLayout()
{
	core::gpu::DescriptorSetLayoutBinding materialBufferBinding{
		.binding = 0,
		.descriptorType = core::gpu::utils::EDescriptorType::StorageBuffer,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment
	};

	core::gpu::DescriptorSetLayoutBinding bindlessTexturesBinding{
		.binding = 1,
		.descriptorCount = MAX_BINDLESS_TEXTURES,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Fragment,
		.partiallyBound = true,
		.updateAfterBind = true
	};

	materialLayout = std::make_unique<core::gpu::DescriptorSetLayout>(
		m_device,
		core::gpu::DescriptorSetLayoutCreateInfo{
			.bindings = { materialBufferBinding, bindlessTexturesBinding }
		}
	);
}

void Renderer::CreateFallbackTLAS()
{
	core::gpu::AccelerationStructureCreateInfo emptyTlasInfo{};
	emptyTlasInfo.type = core::gpu::utils::EAccelerationStructureType::TopLevel;
	emptyTlasInfo.instances = {};
	emptyTlasInfo.preferFastTrace = true;
	emptyTlasInfo.allowUpdate = false;

	m_fallbackTlas = std::make_unique<core::gpu::AccelerationStructure>(m_device, emptyTlasInfo);

	auto commandBuffer = m_device.AcquireCommandBuffer();
	commandBuffer->Record([&]() {
		m_fallbackTlas->Build(m_device);
		});
	commandBuffer->Submit(m_device, true);
	m_device.ReleaseCommandBuffer(commandBuffer);
}

void Renderer::CreateDescriptorSets()
{
	gBufferDescriptorSets.clear();
	shadowDescriptorSets.clear();

	gBufferDescriptorSets.reserve(m_device.FRAMES_IN_FLIGHT);
	shadowDescriptorSets.reserve(m_device.FRAMES_IN_FLIGHT);

	for (uint32_t i = 0; i < m_device.FRAMES_IN_FLIGHT; ++i)
	{
		auto ds = std::make_unique<core::gpu::DescriptorSet>(m_device, *gBufferDsLayouts[0]);
		ds->Bind(0, *uniformBuffers[i]);
		ds->Update(m_device);
		gBufferDescriptorSets.push_back(std::move(ds));
	}

	for (uint32_t i = 0; i < m_device.FRAMES_IN_FLIGHT; ++i)
	{
		auto ds = std::make_unique<core::gpu::DescriptorSet>(m_device, *shadowDsLayouts[0]);
		ds->Bind(0, *uniformBuffers[i]);
		ds->Bind(1, *gBufferDepthAttachment.texture);
		ds->Bind(2, *gBufferColorAttachments[1].texture);
		ds->Bind(3, *m_fallbackTlas);
		ds->Bind(4, *shadowMaskAttachment.image);
		ds->Update(m_device);
		shadowDescriptorSets.push_back(std::move(ds));
	}

	giDescriptorSets.clear();
	giDescriptorSets.reserve(m_device.FRAMES_IN_FLIGHT);

	for (uint32_t i = 0; i < m_device.FRAMES_IN_FLIGHT; ++i)
	{
		auto ds = std::make_unique<core::gpu::DescriptorSet>(
			m_device,
			*giDsLayouts[0]
		);

		ds->Bind(0, *uniformBuffers[i]);

		ds->Bind(1, *gBufferDepthAttachment.texture);
		ds->Bind(2, *gBufferColorAttachments[1].texture);
		ds->Bind(3, *gBufferColorAttachments[0].texture);
		ds->Bind(4, *m_fallbackTlas);
		ds->Bind(5, *m_envMap.texture);
		ds->Bind(6, *giColorAttachments[0].image);
		ds->Bind(7, *giColorAttachments[1].texture);
		ds->Bind(9, *m_envMap.texture);
		ds->Bind(10, *m_meshTableBuffer);
		ds->Bind(11, m_materialLibrary->GetGPUBuffer());
		ds->Update(m_device);

		giDescriptorSets.push_back(std::move(ds));
	}

	resolveDescriptorSets.clear();
	resolveDescriptorSets.reserve(m_device.FRAMES_IN_FLIGHT);

	for (uint32_t i = 0; i < m_device.FRAMES_IN_FLIGHT; ++i)
	{
		auto ds = std::make_unique<core::gpu::DescriptorSet>(m_device, *resolveDsLayouts[0]);
		ds->Bind(0, *uniformBuffers[i]);
		ds->Bind(1, *gBufferColorAttachments[0].texture);
		ds->Bind(2, *gBufferColorAttachments[1].texture);
		ds->Bind(3, *shadowMaskAttachment.texture);
		ds->Bind(4, *resolveColorAttachments[0].image);
		ds->Bind(5, *atrousAttachments[0].texture);
		ds->Bind(6, *gBufferDepthAttachment.texture);
		ds->Bind(7, *m_envMap.texture);
		ds->Bind(8, *gBufferColorAttachments[3].texture);
		ds->Update(m_device);

		resolveDescriptorSets.push_back(std::move(ds));
	}

	fxaaDescriptorSets.clear();
	fxaaDescriptorSets.reserve(m_device.FRAMES_IN_FLIGHT);

	for (uint32_t i = 0; i < m_device.FRAMES_IN_FLIGHT; ++i)
	{
		auto ds = std::make_unique<core::gpu::DescriptorSet>(m_device, *fxaaDsLayouts[0]);
		ds->Bind(0, *resolveColorAttachments[0].texture);
		ds->Bind(1, *aaColorAttachments[0].image);
		ds->Update(m_device);
		fxaaDescriptorSets.push_back(std::move(ds));
	}

	materialDescriptorSet = std::make_unique<core::gpu::DescriptorSet>(m_device, *materialLayout);
	materialDescriptorSet->Bind(0, m_materialLibrary->GetGPUBuffer());
	materialDescriptorSet->Update(m_device);

	atrousDescriptorSets.clear();
	atrousDescriptorSets.resize(m_device.FRAMES_IN_FLIGHT);

	for (uint32_t frame = 0; frame < m_device.FRAMES_IN_FLIGHT; ++frame)
	{
		for (int pass = 0; pass < 5; ++pass)
		{
			auto ds = std::make_unique<core::gpu::DescriptorSet>(m_device, *atrousDsLayouts[0]);

			core::gpu::Texture* inputTex = (pass == 0)
				? giColorAttachments[0].texture.get()    
				: atrousAttachments[(pass - 1) % 2].texture.get();

			core::gpu::Image* outputImg = atrousAttachments[pass % 2].image.get();

			ds->Bind(0, *inputTex);
			ds->Bind(1, *gBufferDepthAttachment.texture);
			ds->Bind(2, *gBufferColorAttachments[1].texture);
			ds->Bind(3, *outputImg);
			ds->Update(m_device);

			atrousDescriptorSets[frame][pass] = std::move(ds);
		}
	}
}

void Renderer::UpdateDescriptorSets()
{
	auto& currentTlas = m_tlasPerFrame[m_device.currentFrame];
	if (currentTlas)
	{
		giDescriptorSets[m_device.currentFrame]->Bind(4, *currentTlas);
		shadowDescriptorSets[m_device.currentFrame]->Bind(3, *currentTlas);
	}

	m_giParity = 1 - m_giParity;
	uint32_t writeIdx = m_giParity;
	uint32_t readIdx = 1 - m_giParity;

	giDescriptorSets[m_device.currentFrame]->Bind(6, *giColorAttachments[writeIdx].image);
	giDescriptorSets[m_device.currentFrame]->Bind(7, *giColorAttachments[readIdx].texture);
	giDescriptorSets[m_device.currentFrame]->Update(m_device);

	shadowDescriptorSets[m_device.currentFrame]->Update(m_device);

	atrousDescriptorSets[m_device.currentFrame][0]->Bind(0, *giColorAttachments[m_giParity].texture);
	atrousDescriptorSets[m_device.currentFrame][0]->Update(m_device);
}

Renderer::Renderer(const core::gpu::Device& _device)
	: m_device(_device)
{
	CreateUniformBuffers();
	LoadEnvironmentMaps();

	m_textureLibrary = std::make_unique<TextureLibrary>(m_device);
	m_materialLibrary = std::make_unique<MaterialLibrary>(m_device, *m_textureLibrary);
	m_materialLibrary->UploadGPUData();

	auto [initWidth, initHeight] = m_device.GetSwapchainExtent();

	CreateAttachments(initWidth, initHeight);
	CreateDescriptorSetLayout();
	CreateMaterialLayout();

	m_gBufferPipeline = BuildGBufferPipeline();
	m_shadowPipeline = BuildShadowPipeline();
	m_giPipeline = BuildGIPipeline();
	m_resolvePipeline = BuildResolvePipeline();
	m_fxaaPipeline = BuildFXAAPipeline();
	m_atrousPipeline = BuildATrousPipeline();

	AQUILA_SET_DEBUG_NAME(m_device, *m_gBufferPipeline, "Renderer::Pipeline::GBuffer");
	AQUILA_SET_DEBUG_NAME(m_device, *m_shadowPipeline, "Renderer::Pipeline::Shadow");
	AQUILA_SET_DEBUG_NAME(m_device, *m_giPipeline, "Renderer::Pipeline::GI");
	AQUILA_SET_DEBUG_NAME(m_device, *m_resolvePipeline, "Renderer::Pipeline::Resolve");
	AQUILA_SET_DEBUG_NAME(m_device, *m_fxaaPipeline, "Renderer::Pipeline::FXAA");
	AQUILA_SET_DEBUG_NAME(m_device, *m_atrousPipeline, "Renderer::Pipeline::ATrous");

	CreateFallbackTLAS();
	CreateDescriptorSets();

	SyncMaterialsAndTextures();

	m_cameraPosition = glm::vec3(2.0f, 2.5f, 8.5f);
	m_cameraTarget = glm::vec3(3.5f, 1.0f, 2.0f);

	m_projMatrix = glm::perspective(glm::radians(m_cameraFovY), float(initWidth) / float(initHeight), m_cameraNear, m_cameraFar);
	m_projMatrix[1][1] *= -1.0f;

	UpdateCamera();
}

Renderer::~Renderer() = default;

void Renderer::Render(core::gpu::CommandBuffer* _cmdBuf, core::gpu::Image& _outputImage)
{
	AQUILA_CHECK(_cmdBuf != nullptr, "Renderer::Render: commandBuffer is null");

	const uint32_t outWidth = _outputImage.GetSize().first;
	const uint32_t outHeight = _outputImage.GetSize().second;

	AQUILA_CHECK(outWidth > 0 && outHeight > 0, "Renderer::Render: output image has invalid dimensions");

	if (outWidth != m_renderWidth || outHeight != m_renderHeight)
	{
		m_device.WaitIdle();
		CreateAttachments(outWidth, outHeight);
		CreateDescriptorSets();
		m_nextBindlessTextureIndex = 0;
	}

	m_projMatrix = glm::perspective(glm::radians(m_cameraFovY), float(outWidth) / float(outHeight), m_cameraNear, m_cameraFar);
	m_projMatrix[1][1] *= -1.0f;

	m_tlasPerFrame.resize(m_device.FRAMES_IN_FLIGHT);
	SyncMaterialsAndTextures();

	BuildTLAS();
	RebuildAccelerationStructures();
	UpdateCamera();
	UpdateUniformBuffers();
	UpdateDescriptorSets();

	lightBuffer->CopyFrom(m_gpuLights.data(), sizeof(GPULight) * m_gpuLights.size());

	BuildAndExecuteFrameGraph(*_cmdBuf, _outputImage);

	m_meshInstances.clear();
	m_gpuLights.clear();
}

void Renderer::PushMesh(graphics::render::Mesh* _mesh, glm::mat4& _transform)
{
	if (!_mesh) return;
	AQUILA_CHECK(_mesh->blas != nullptr, "Renderer::PushMesh: Mesh pushed without BLAS");

	RegisterMesh(_mesh);

	m_meshInstances.push_back({ _mesh, _transform });
}

void Renderer::BuildAndExecuteFrameGraph(core::gpu::CommandBuffer& _cmdBuf, core::gpu::Image& _outputImage)
{
	m_renderGraph.Reset();
	m_framePasses.clear();
	m_frameResourcePool.clear();
	m_frameResourcePool.resize(16);

	const uint32_t prevFrame = (m_device.currentFrame + m_device.FRAMES_IN_FLIGHT - 1) % m_device.FRAMES_IN_FLIGHT;

	auto importAttachment = [this](PassAttachment& _attachment, const char* _name, bool _isDepth = false) -> ResourceHandle
		{
			Resource& res = m_frameResourcePool.emplace_back();
			res.name = _name;
			res.desc.type = ResourceType::Texture;
			res.desc.isDepthFormat = _isDepth;
			res.image = _attachment.image.get();
			res.texture = _attachment.texture.get();
			res.currentState = ToResourceState(_attachment.image->GetLayout());
			return m_renderGraph.ImportResource(res);
		};

	ResourceHandle color0Handle = importAttachment(gBufferColorAttachments[0], "gBufferAlbedo");
	ResourceHandle color1Handle = importAttachment(gBufferColorAttachments[1], "gBufferNormal");
	ResourceHandle color2Handle = importAttachment(gBufferColorAttachments[2], "gBufferMotion");
	ResourceHandle color3Handle = importAttachment(gBufferColorAttachments[3], "gBufferEmissive");
	ResourceHandle depthHandle = importAttachment(gBufferDepthAttachment, "gBufferDepth", true);
	ResourceHandle shadowHandle = importAttachment(shadowMaskAttachment, "shadowMask");
	ResourceHandle resolveHandle = importAttachment(resolveColorAttachments[0], "resolveColor");
	ResourceHandle aaHandle = importAttachment(aaColorAttachments[0], "aaColor");

	ResourceHandle giWriteHandle = importAttachment(giColorAttachments[m_giParity], "giWrite");
	ResourceHandle giReadHandle = importAttachment(giColorAttachments[1 - m_giParity], "giRead");

	auto gBufferPass = std::make_unique<LambdaPass>(
		PassCreateInfo{
			.name = "GBuffer",
			.inputs = {},
			.outputs = {
				{ color0Handle, ResourceState::RenderTarget },
				{ color1Handle, ResourceState::RenderTarget },
				{ color2Handle, ResourceState::RenderTarget },
				{ color3Handle, ResourceState::RenderTarget },
				{ depthHandle,  ResourceState::DepthWrite }
			}
		},
		[this, &_cmdBuf, prevFrame]()
		{
			std::vector<core::gpu::CommandBuffer::RenderingAttachmentInfo> colorInfos;
			colorInfos.reserve(gBufferColorAttachments.size());

			for (const auto& colorAttachment : gBufferColorAttachments)
			{
				colorInfos.push_back({
					.image = colorAttachment.image.get(),
					.clear = true,
					});
			}

			core::gpu::CommandBuffer::DepthAttachmentInfo depthInfo
			{
				.image = gBufferDepthAttachment.image.get(),
				.clear = true,
				.clearDepth = 1.0f
			};

			_cmdBuf.BeginRendering(m_device, colorInfos, depthInfo);
			_cmdBuf.Bind<core::gpu::Pipeline>(*m_gBufferPipeline);
			_cmdBuf.Bind(*gBufferDescriptorSets[m_device.currentFrame], *m_gBufferPipeline, 0u);
			_cmdBuf.Bind(*materialDescriptorSet, *m_gBufferPipeline, 1u);
			_cmdBuf.SetViewport(0.0f, 0.0f, static_cast<float>(m_renderWidth), static_cast<float>(m_renderHeight), 0.0f, 1.0f);
			_cmdBuf.SetScissor(0, 0, m_renderWidth, m_renderHeight);

			if (!m_meshInstances.empty())
			{
				for (const auto& [mesh, transform] : m_meshInstances)
				{
					if (!mesh->vertexBuffer || !mesh->indexBuffer)
						continue;

					_cmdBuf.Bind<core::gpu::Buffer>(*mesh->vertexBuffer);
					_cmdBuf.Bind<core::gpu::Buffer>(*mesh->indexBuffer);

					glm::mat4 prevModel = transform;
					auto it = m_prevMeshInstances[prevFrame].find(mesh);
					if (it != m_prevMeshInstances[prevFrame].end())
						prevModel = it->second;

					for (const auto& submesh : mesh->instance.subMeshes)
					{
						MaterialHandle matHandle = submesh.material;
						if (matHandle == INVALID_MATERIAL)
							matHandle = m_materialLibrary->GetDefaultMaterial();

						GBufferPushConstants pushConstants{};
						pushConstants.model = transform;
						pushConstants.prevModel = prevModel;
						pushConstants.materialId = matHandle;

						_cmdBuf.PushConstants(
							*m_gBufferPipeline,
							static_cast<uint32_t>(
								core::gpu::utils::EShaderStageFlags::Vertex |
								core::gpu::utils::EShaderStageFlags::Fragment),
							0,
							sizeof(GBufferPushConstants),
							&pushConstants
						);

						_cmdBuf.DrawIndexed(submesh.indexCount, 1, submesh.firstIndex, submesh.vertexOffset, 0);
					}

					m_prevMeshInstances[m_device.currentFrame][mesh] = transform;
				}
			}

			_cmdBuf.EndRendering();
		}
	);

	m_renderGraph.AddPass(*gBufferPass);
	m_framePasses.push_back(std::move(gBufferPass));

	auto shadowPass = std::make_unique<LambdaPass>(
		PassCreateInfo{
			.name = "Shadow",
			.inputs = {
				{ depthHandle,  ResourceState::ShaderRead },
				{ color1Handle, ResourceState::ShaderRead }
			},
			.outputs = {
				{ shadowHandle, ResourceState::UnorderedAccess }
			}
		},
		[this, &_cmdBuf]()
		{
			ShadowPushConstants pc{};
			pc.lightBuffer = lightBuffer->GetDeviceAddress();
			pc.maxDistance = 10000.0f;

			_cmdBuf.Bind<core::gpu::Pipeline>(*m_shadowPipeline);
			_cmdBuf.Bind(*shadowDescriptorSets[m_device.currentFrame], *m_shadowPipeline, 0u);
			_cmdBuf.PushConstants(
				*m_shadowPipeline,
				static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
				0, sizeof(ShadowPushConstants), &pc
			);

			_cmdBuf.Dispatch((m_renderWidth + 15) / 16, (m_renderHeight + 15) / 16, 1);
		}
	);

	m_renderGraph.AddPass(*shadowPass);
	m_framePasses.push_back(std::move(shadowPass));

	auto giPass = std::make_unique<LambdaPass>(
		PassCreateInfo{
			.name = "GI",
			.inputs = {
				{ depthHandle,   ResourceState::ShaderRead },
				{ color1Handle,  ResourceState::ShaderRead },
				{ color0Handle,  ResourceState::ShaderRead },
				{ giReadHandle,  ResourceState::ShaderRead }
			},
			.outputs = {
				{ giWriteHandle, ResourceState::UnorderedAccess }
			}
		},
		[this, &_cmdBuf]()
		{
			GIPushConstants pc{};
			pc.sampleCount = 16;
			pc.maxDistance = 50.0f;
			pc.numLights = static_cast<uint32_t>(m_gpuLights.size());
			pc.lightBuffer = lightBuffer->GetDeviceAddress();

			_cmdBuf.Bind<core::gpu::Pipeline>(*m_giPipeline);
			_cmdBuf.Bind(*giDescriptorSets[m_device.currentFrame], *m_giPipeline, 0u);
			_cmdBuf.PushConstants(
				*m_giPipeline,
				static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
				0, sizeof(GIPushConstants), &pc
			);

			_cmdBuf.Dispatch((m_renderWidth + 15) / 16, (m_renderHeight + 15) / 16, 1);
		}
	);

	m_renderGraph.AddPass(*giPass);
	m_framePasses.push_back(std::move(giPass));

	constexpr int steps[5] = { 1, 2, 4, 8, 16 };

	ResourceState atrousState[2] = {
		ToResourceState(atrousAttachments[0].image->GetLayout()),
		ToResourceState(atrousAttachments[1].image->GetLayout())
	};

	auto importAtrous = [this, &atrousState](int _idx) -> ResourceHandle
		{
			Resource& res = m_frameResourcePool.emplace_back();
			res.name = std::string("atrous") + std::to_string(_idx);
			res.desc.type = ResourceType::Texture;
			res.image = atrousAttachments[_idx].image.get();
			res.texture = atrousAttachments[_idx].texture.get();
			res.currentState = atrousState[_idx];
			return m_renderGraph.ImportResource(res);
		};

	ResourceHandle atrousPrevHandle = giWriteHandle;

	for (int pass = 0; pass < 5; ++pass)
	{
		const int outIdx = pass % 2;
		ResourceHandle outHandle = importAtrous(outIdx);

		auto atrousPass = std::make_unique<LambdaPass>(
			PassCreateInfo{
				.name = "ATrous" + std::to_string(pass),
				.inputs = {
					{ atrousPrevHandle, ResourceState::ShaderRead },
					{ depthHandle,      ResourceState::ShaderRead },
					{ color1Handle,     ResourceState::ShaderRead }
				},
				.outputs = {
					{ outHandle, ResourceState::UnorderedAccess }
				}
			},
			[this, &_cmdBuf, pass, &steps]()
			{
				_cmdBuf.Bind<core::gpu::Pipeline>(*m_atrousPipeline);
				_cmdBuf.Bind(*atrousDescriptorSets[m_device.currentFrame][pass], *m_atrousPipeline, 0u);

				ATrousPushConstants pc{};
				pc.stepSize = steps[pass];

				_cmdBuf.PushConstants(
					*m_atrousPipeline,
					static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
					0, sizeof(ATrousPushConstants), &pc
				);

				_cmdBuf.Dispatch((m_renderWidth + 15) / 16, (m_renderHeight + 15) / 16, 1);
			}
		);

		m_renderGraph.AddPass(*atrousPass);
		m_framePasses.push_back(std::move(atrousPass));

		if (pass > 0)
		{
			const int prevIdx = (pass - 1) % 2;
			atrousState[prevIdx] = ResourceState::ShaderRead;
		}
		atrousState[outIdx] = ResourceState::UnorderedAccess;

		atrousPrevHandle = outHandle;
	}

	auto resolvePass = std::make_unique<LambdaPass>(
		PassCreateInfo{
			.name = "Resolve",
			.inputs = {
				{ color0Handle,     ResourceState::ShaderRead },
				{ color1Handle,     ResourceState::ShaderRead },
				{ color3Handle,     ResourceState::ShaderRead },
				{ shadowHandle,     ResourceState::ShaderRead },
				{ depthHandle,      ResourceState::ShaderRead },
				{ atrousPrevHandle, ResourceState::ShaderRead }
			},
			.outputs = {
				{ resolveHandle, ResourceState::UnorderedAccess }
			}
		},
		[this, &_cmdBuf]()
		{
			_cmdBuf.Bind<core::gpu::Pipeline>(*m_resolvePipeline);
			_cmdBuf.Bind(*resolveDescriptorSets[m_device.currentFrame], *m_resolvePipeline, 0u);

			ResolvePushConstants pc{};
			pc.numLights = static_cast<uint32_t>(m_gpuLights.size());
			pc.lightBuffer = lightBuffer->GetDeviceAddress();

			_cmdBuf.PushConstants(
				*m_resolvePipeline,
				static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
				0, sizeof(ResolvePushConstants), &pc
			);

			_cmdBuf.Dispatch((m_renderWidth + 15) / 16, (m_renderHeight + 15) / 16, 1);
		}
	);

	m_renderGraph.AddPass(*resolvePass);
	m_framePasses.push_back(std::move(resolvePass));

	auto fxaaPass = std::make_unique<LambdaPass>(
		PassCreateInfo{
			.name = "FXAA",
			.inputs = {
				{ resolveHandle, ResourceState::ShaderRead }
			},
			.outputs = {
				{ aaHandle, ResourceState::UnorderedAccess }
			}
		},
		[this, &_cmdBuf]()
		{
			FXAAPushConstants pc{};
			pc.texelSize = glm::vec2(1.0f / static_cast<float>(m_renderWidth), 1.0f / static_cast<float>(m_renderHeight));

			_cmdBuf.Bind<core::gpu::Pipeline>(*m_fxaaPipeline);
			_cmdBuf.Bind(*fxaaDescriptorSets[m_device.currentFrame], *m_fxaaPipeline, 0u);
			_cmdBuf.PushConstants(
				*m_fxaaPipeline,
				static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
				0, sizeof(FXAAPushConstants), &pc
			);

			_cmdBuf.Dispatch((m_renderWidth + 15) / 16, (m_renderHeight + 15) / 16, 1);
		}
	);

	m_renderGraph.AddPass(*fxaaPass);
	m_framePasses.push_back(std::move(fxaaPass));

	auto presentPass = std::make_unique<LambdaPass>(
		PassCreateInfo{
			.name = "Present",
			.inputs = {
				{ aaHandle, ResourceState::TransferSrc }
			},
			.outputs = {}
		},
		[this, &_cmdBuf, &_outputImage]()
		{
			_cmdBuf.TransitionImageLayout(_outputImage, core::gpu::utils::EImageLayout::TransferDst, false);
			_cmdBuf.BlitImage(*aaColorAttachments[0].image, _outputImage);
			_cmdBuf.TransitionImageLayout(_outputImage, core::gpu::utils::EImageLayout::ColorAttachment, false);
		}
	);

	m_renderGraph.AddPass(*presentPass);
	m_framePasses.push_back(std::move(presentPass));

	m_renderGraph.Compile();
	m_renderGraph.Execute(_cmdBuf);
}

void Renderer::UpdateUniformBuffers()
{
	UniformBufferObject ubo
	{
		.view = m_viewMatrix,
		.proj = m_projMatrix,
		.viewProjInverse = glm::inverse(m_projMatrix * m_viewMatrix),
		.prevViewProj = m_prevViewProj,
		.prevViewProjInverse = glm::inverse(m_prevViewProj),
		.viewPos = glm::vec4(m_cameraPosition, 1.0f),
		.frameCount = m_device.currentFrame
	};

	m_prevViewProj = m_projMatrix * m_viewMatrix;
	uniformBuffers[m_device.currentFrame]->CopyFrom(&ubo, sizeof(UniformBufferObject));
}

void Renderer::BuildTLAS()
{
	if (m_meshInstances.empty()) return;

	std::vector<core::gpu::AccelerationStructureInstance> instances;
	instances.reserve(m_meshInstances.size());

	for (const auto& meshInstance : m_meshInstances)
	{
		if (!meshInstance.first->blas)
			continue;

		const glm::mat4& mat = meshInstance.second;
		float transform[3][4] = {
			{mat[0][0], mat[1][0], mat[2][0], mat[3][0]},
			{mat[0][1], mat[1][1], mat[2][1], mat[3][1]},
			{mat[0][2], mat[1][2], mat[2][2], mat[3][2]}
		};

		core::gpu::AccelerationStructureInstance instance{};
		std::memcpy(&instance.transform, &transform, sizeof(transform));
		instance.mask = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.blas = meshInstance.first->blas.get();

		if (meshInstance.first->blas->GetDeviceAddress(m_device) == 0)
		{
			std::cerr << "ERROR: BLAS has invalid device address!\n";
			continue;
		}

		if (meshInstance.first->meshTableIndex == UINT32_MAX)
		{
			std::cerr << "Warning: Mesh not registered in mesh table, call RegisterMesh() before pushing it!\n";
			continue;
		}

		instance.instanceCustomIndex = meshInstance.first->meshTableIndex;
		instances.push_back(instance);
	}

	if (instances.empty()) return;

	core::gpu::AccelerationStructureCreateInfo tlasInfo{};
	tlasInfo.type = core::gpu::utils::EAccelerationStructureType::TopLevel;
	tlasInfo.instances = instances;
	tlasInfo.preferFastTrace = true;
	tlasInfo.allowUpdate = false;

	m_tlasPerFrame[m_device.currentFrame] = std::make_unique<core::gpu::AccelerationStructure>(m_device, tlasInfo);
}

uint32_t Renderer::RegisterBindlessTexture(const core::gpu::Texture& _texture)
{
	if (m_nextBindlessTextureIndex >= MAX_BINDLESS_TEXTURES)
	{
		std::cerr << "Bindless texture array full!\n";
		return 0;
	}

	uint32_t idx = m_nextBindlessTextureIndex++;

	for (uint32_t i = 0; i < m_device.FRAMES_IN_FLIGHT; ++i)
	{
		giDescriptorSets[i]->BindArray(12, idx, _texture);
		giDescriptorSets[i]->Update(m_device);
	}

	materialDescriptorSet->BindArray(1, idx, _texture);
	materialDescriptorSet->Update(m_device);

	return idx;
}

void Renderer::SyncMaterialsAndTextures()
{
	for (uint32_t t = m_nextBindlessTextureIndex; t < static_cast<uint32_t>(m_textureLibrary->Size()); ++t)
	{
		RegisterBindlessTexture(m_textureLibrary->Get(static_cast<TextureHandle>(t)));
	}

	m_materialLibrary->UploadGPUData();
}

void Renderer::RegisterMesh(graphics::render::Mesh* _mesh)
{
	AQUILA_CHECK(_mesh != nullptr, "Renderer::RegisterMesh: mesh is null");

	if (_mesh->meshTableIndex != UINT32_MAX)
		return;

	AQUILA_CHECK(m_nextMeshTableIndex < MAX_MESHES, "Renderer::RegisterMesh: mesh table capacity exceeded");

	_mesh->meshTableIndex = m_nextMeshTableIndex++;

	MaterialHandle firstMat = _mesh->instance.subMeshes.empty()
		? m_materialLibrary->GetDefaultMaterial()
		: _mesh->instance.subMeshes[0].material;

	if (firstMat == INVALID_MATERIAL)
		firstMat = m_materialLibrary->GetDefaultMaterial();

	AQUILA_CHECK(_mesh->vertexBuffer != nullptr, "Renderer::RegisterMesh: vertex buffer is null");
	AQUILA_CHECK(_mesh->indexBuffer != nullptr, "Renderer::RegisterMesh: index buffer is null");

	uintptr_t vertexBase = _mesh->vertexBuffer->GetDeviceAddress();

	MeshTableEntry entry{
		.indices = _mesh->indexBuffer->GetDeviceAddress(),
		.positions = vertexBase + offsetof(graphics::render::Vertex, position),
		.normals = vertexBase + offsetof(graphics::render::Vertex, normal),
		.uvs = vertexBase + offsetof(graphics::render::Vertex, uv),
		.tangents = vertexBase + offsetof(graphics::render::Vertex, tangent),
		.materialId = firstMat,
		.vertexStride = sizeof(graphics::render::Vertex)
	};


	m_meshTableBuffer->CopyFrom(&entry, sizeof(MeshTableEntry), _mesh->meshTableIndex * sizeof(MeshTableEntry));
}

void Renderer::RebuildAccelerationStructures()
{
	if (m_device.currentFrame >= m_tlasPerFrame.size()) return;
	if (!m_tlasPerFrame[m_device.currentFrame]) return;

	auto commandBuffer = m_device.AcquireCommandBuffer();

	commandBuffer->Record([&]() {
		m_tlasPerFrame[m_device.currentFrame]->Build(m_device);
		});
	commandBuffer->Submit(m_device, true);

	m_device.ReleaseCommandBuffer(commandBuffer);
}

void Renderer::SetCamera(const glm::vec3& position)
{
	m_cameraPosition = position;
}

void Renderer::SetCameraTarget(const glm::vec3& target)
{
	m_cameraTarget = target;
}

void Renderer::MoveCamera(const glm::vec3& delta)
{
	m_cameraPosition += delta;
	m_cameraTarget += delta;
}

void Renderer::UpdateCamera()
{
	m_viewMatrix = glm::lookAt(
		m_cameraPosition,
		m_cameraTarget,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
}