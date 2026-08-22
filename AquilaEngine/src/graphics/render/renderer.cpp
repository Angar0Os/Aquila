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

#include <loaders/shaderLoader.h>

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

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
	pipelineInfo.colorAttachmentFormats = { core::gpu::utils::ETextureFormat::RGBA8_SRGB, core::gpu::utils::ETextureFormat::RGBA16_Float, core::gpu::utils::ETextureFormat::RG32_Float };
	pipelineInfo.depthAttachmentFormat = core::gpu::utils::ETextureFormat::Depth32F;
	pipelineInfo.descriptorSetLayouts = { gBufferDsLayouts[0].get(), materialLayout.get() };
	pipelineInfo.pushConstantRanges = pushConstants;
	pipelineInfo.dynamicStates = { core::gpu::utils::EDynamicState::Viewport, core::gpu::utils::EDynamicState::Scissor };

	return std::make_unique<core::gpu::Pipeline>(m_device, pipelineInfo);
}

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildShadowPipeline()
{
	auto shaderCode = loaders::ReadFile("assets/shaders/shadow.spv");

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
	auto shaderCode = loaders::ReadFile("assets/shaders/gi.spv");

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
	auto shaderCode = loaders::ReadFile("assets/shaders/resolve.spv");

	core::gpu::PipelineCreateInfo pipelineInfo{};
	pipelineInfo.shaderStages = { { core::gpu::utils::EShaderStageFlags::Compute, shaderCode, "cs_main" } };
	pipelineInfo.pipelineType = core::gpu::utils::EPipelineType::Compute;
	pipelineInfo.descriptorSetLayouts = { resolveDsLayouts[0].get() };

	std::vector<core::gpu::PushConstantRange> pushConstants = {
		{
			.stageFlags = static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
			.offset = 0, .size = sizeof(ResolvePushConstants)
		}
	};
	pipelineInfo.pushConstantRanges = pushConstants;

	return std::make_unique<core::gpu::Pipeline>(m_device, pipelineInfo);
}

std::unique_ptr<core::gpu::Pipeline> Renderer::BuildFXAAPipeline()
{
	auto shaderCode = loaders::ReadFile("assets/shaders/fxaa.spv");

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
	auto shaderCode = loaders::ReadFile("assets/shaders/atrous.spv");

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
	m_envMap.image = TextureLibrary::LoadHDRImage(m_device, "assets/textures/skyboxes/citrus_1k.hdr");
	m_envMap.texture = std::make_unique<core::gpu::Texture>(m_device, *m_envMap.image);

	auto shaderCode = loaders::ReadFile("assets/shaders/irradiance_convolution.spv");

	core::gpu::DescriptorSetLayoutBinding envMapBinding{
		.binding = 0,
		.descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding irradianceOutputBinding{
		.binding = 1,
		.descriptorType = core::gpu::utils::EDescriptorType::StorageImage,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	auto convLayout = std::make_unique<core::gpu::DescriptorSetLayout>(
		m_device,
		core::gpu::DescriptorSetLayoutCreateInfo{ .bindings = { envMapBinding, irradianceOutputBinding } }
	);

	core::gpu::PipelineCreateInfo convPipelineInfo{};
	convPipelineInfo.shaderStages = { { core::gpu::utils::EShaderStageFlags::Compute, shaderCode, "cs_main" } };
	convPipelineInfo.pipelineType = core::gpu::utils::EPipelineType::Compute;
	convPipelineInfo.descriptorSetLayouts = { convLayout.get() };

	auto convPipeline = std::make_unique<core::gpu::Pipeline>(m_device, convPipelineInfo);

	core::gpu::ImageCreateInfo irradianceInfo{
		.width = 128,
		.height = 64,
		.mipLevels = 1,
		.format = core::gpu::utils::ETextureFormat::RGBA32_Float,
		.tiling = core::gpu::utils::EImageTiling::Optimal,
		.usage = core::gpu::utils::EImageUsage::Sampled | core::gpu::utils::EImageUsage::Storage,
		.memoryProperties = core::gpu::utils::EMemoryProperty::DeviceLocal,
		.samples = core::gpu::utils::ESampleCount::e1
	};
	m_irradianceMap.image = std::make_unique<core::gpu::Image>(m_device, irradianceInfo);

	auto convDs = std::make_unique<core::gpu::DescriptorSet>(m_device, *convLayout);
	convDs->Bind(0, *m_envMap.texture);
	convDs->Bind(1, *m_irradianceMap.image);
	convDs->Update(m_device);

	auto commandBuffer = m_device.AcquireCommandBuffer();
	commandBuffer->Record([&]() {
		commandBuffer->TransitionImageLayout(
			*m_envMap.image,
			core::gpu::utils::EImageLayout::Undefined,
			core::gpu::utils::EImageLayout::ShaderReadOnly,
			false
		);
		commandBuffer->TransitionImageLayout(
			*m_irradianceMap.image,
			core::gpu::utils::EImageLayout::Undefined,
			core::gpu::utils::EImageLayout::General,
			false
		);

		commandBuffer->Bind<core::gpu::Pipeline>(*convPipeline);
		commandBuffer->Bind(*convDs, *convPipeline, 0u);
		commandBuffer->Dispatch(128 / 16, 64 / 16, 1);

		commandBuffer->TransitionImageLayout(
			*m_irradianceMap.image,
			core::gpu::utils::EImageLayout::General,
			core::gpu::utils::EImageLayout::ShaderReadOnly,
			false
		);
		});
	commandBuffer->Submit(m_device, true);
	m_device.ReleaseCommandBuffer(commandBuffer);

	m_irradianceMap.texture = std::make_unique<core::gpu::Texture>(m_device, *m_irradianceMap.image);
}

void Renderer::CreateUniformBuffers()
{
	uniformBuffers.clear();
	uniformBuffers.reserve(m_device.FRAMES_IN_FLIGHT);


	for (size_t i = 0; i < m_device.FRAMES_IN_FLIGHT; i++)
	{
		core::gpu::BufferCreateInfo bufferInfo{
			.size = sizeof(UniformBufferObject),
			.usage = core::gpu::utils::EBufferUsage::UniformBuffer,
			.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
		};
		uniformBuffers.push_back(std::make_unique<core::gpu::Buffer>(m_device, bufferInfo));
	}

	lightBuffers.clear();
	lightBuffers.reserve(m_device.FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < m_device.FRAMES_IN_FLIGHT; i++)
	{
		core::gpu::BufferCreateInfo bufferInfo{
			.size = sizeof(GPULight) * MAX_LIGHTS,
			.usage = core::gpu::utils::EBufferUsage::StorageBuffer,
			.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
		};
		lightBuffers.push_back(std::make_unique<core::gpu::Buffer>(m_device, bufferInfo));
	}

	core::gpu::BufferCreateInfo meshTableInfo{
		.size = sizeof(MeshTableEntry) * MAX_MESHES,
		.usage = core::gpu::utils::EBufferUsage::StorageBuffer,
		.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
	};
	m_meshTableBuffer = std::make_unique<core::gpu::Buffer>(m_device, meshTableInfo);
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
	}

	auto commandBuffer = m_device.AcquireCommandBuffer();
	commandBuffer->Record([&]() {
		for (int i = 0; i < 2; ++i)
		{
			commandBuffer->TransitionImageLayout(
				*giColorAttachments[i].image,
				core::gpu::utils::EImageLayout::Undefined,
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
	}

	auto atrousInitCmd = m_device.AcquireCommandBuffer();
	atrousInitCmd->Record([&]() {
		for (int i = 0; i < 2; ++i)
		{
			atrousInitCmd->TransitionImageLayout(
				*atrousAttachments[i].image,
				core::gpu::utils::EImageLayout::Undefined,
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

	core::gpu::DescriptorSetLayoutBinding resolveUboBinding{
	.binding = 0, .descriptorType = core::gpu::utils::EDescriptorType::UniformBuffer,
	.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding resolveAlbedoBinding{
		.binding = 1, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding resolveNormalBinding{
		.binding = 2, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding resolveShadowBinding{
		.binding = 3, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};
	core::gpu::DescriptorSetLayoutBinding resolveOutputBinding{
		.binding = 4, .descriptorType = core::gpu::utils::EDescriptorType::StorageImage,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding resolveGiBinding{
		.binding = 5, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding resolveDepthBinding{
		.binding = 6, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding resolveEnvironmentRawBinding{
		.binding = 7, .descriptorType = core::gpu::utils::EDescriptorType::CombinedImageSampler,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	core::gpu::DescriptorSetLayoutBinding resolveLightBufferBinding{
		.binding = 8, .descriptorType = core::gpu::utils::EDescriptorType::StorageBuffer,
		.stageFlags = core::gpu::utils::EShaderStage::Compute
	};

	resolveDsLayouts.clear();
	resolveDsLayouts.push_back(std::make_unique<core::gpu::DescriptorSetLayout>(
		m_device,
		core::gpu::DescriptorSetLayoutCreateInfo{
			.bindings = { resolveUboBinding, resolveAlbedoBinding, resolveNormalBinding,
						  resolveShadowBinding, resolveOutputBinding, resolveGiBinding,
						  resolveDepthBinding, resolveEnvironmentRawBinding, resolveLightBufferBinding  }
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
		ds->Bind(5, *m_irradianceMap.texture);
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
		ds->Bind(8, *lightBuffers[i]);
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

	CreateAttachments();
	CreateDescriptorSetLayout();
	CreateMaterialLayout();

	m_gBufferPipeline = BuildGBufferPipeline();
	m_shadowPipeline = BuildShadowPipeline();
	m_giPipeline = BuildGIPipeline();
	m_resolvePipeline = BuildResolvePipeline();
	m_fxaaPipeline = BuildFXAAPipeline();
	m_atrousPipeline = BuildATrousPipeline();

	CreateFallbackTLAS();
	CreateDescriptorSets();

	SyncMaterialsAndTextures();

	m_cameraPosition = glm::vec3(2.0f, 2.5f, 8.5f);
	m_cameraTarget = glm::vec3(3.5f, 1.0f, 2.0f);

	UpdateCamera();

	auto [width, height] = m_device.GetSwapchainExtent();
	m_projMatrix = glm::perspective(glm::radians(60.0f), float(width) / float(height), 0.1f, 100.0f);
	m_projMatrix[1][1] *= -1.0f;
}

Renderer::~Renderer() = default;

void Renderer::Render(core::gpu::CommandBuffer* _cmdBuf, core::gpu::Image& _outputImage)
{
	m_tlasPerFrame.resize(m_device.FRAMES_IN_FLIGHT);
	SyncMaterialsAndTextures();

	BuildTLAS();
	RebuildAccelerationStructures();
	UpdateCamera();
	UpdateUniformBuffers();
	UpdateDescriptorSets();

	lightBuffers[m_device.currentFrame]->CopyFrom(m_pointLights.data(), sizeof(GPULight) * m_pointLights.size());

	_cmdBuf->Record([&]()
		{
			DrawGBuffer(*_cmdBuf);
			DrawShadow(*_cmdBuf);
			DrawGI(*_cmdBuf);
			DrawATrous(*_cmdBuf);
			DrawResolve(*_cmdBuf);
			DrawFXAA(*_cmdBuf);

			_cmdBuf->TransitionImageLayout(
				_outputImage,
				core::gpu::utils::EImageLayout::Undefined,
				core::gpu::utils::EImageLayout::TransferDst,
				false
			);

			_cmdBuf->BlitImage(*aaColorAttachments[0].image, _outputImage);

			_cmdBuf->TransitionImageLayout(
				_outputImage,
				core::gpu::utils::EImageLayout::TransferDst,
				core::gpu::utils::EImageLayout::Present,
				false
			);
		});

	_cmdBuf->Submit(m_device, false);
	m_device.ReleaseCommandBuffer(_cmdBuf);

	m_meshInstances.clear();
}

void Renderer::PushMesh(graphics::render::Mesh* _mesh, glm::mat4& _transform)
{
	if (!_mesh) return;
	if (!_mesh->blas)
	{
		std::cerr << "Warning : Mesh pushed without BLAS !" << std::endl;
	}

	RegisterMesh(_mesh);

	m_meshInstances.push_back({ _mesh, _transform });
}

void Renderer::DrawGBuffer(core::gpu::CommandBuffer& _cmdBuf)
{
	uint32_t prevFrame = (m_device.currentFrame + m_device.FRAMES_IN_FLIGHT - 1) % m_device.FRAMES_IN_FLIGHT;

	std::vector<ColorAttachmentDesc> colorDescs;

	for (const auto& colorAttachment : gBufferColorAttachments)
	{
		ColorAttachmentDesc desc{};
		desc.image = colorAttachment.image.get();
		desc.clear = true;
		colorDescs.push_back(desc);

		_cmdBuf.TransitionImageLayout(
			*colorAttachment.image,
			core::gpu::utils::EImageLayout::Undefined,
			core::gpu::utils::EImageLayout::ColorAttachment,
			false
		);
	}

	DepthAttachmentDesc depthDesc{};

	if (gBufferDepthAttachment.image)
	{
		depthDesc.image = gBufferDepthAttachment.image.get();
		depthDesc.clear = true;
		depthDesc.clearDepth = 1.0f;

		_cmdBuf.TransitionImageLayout(
			*gBufferDepthAttachment.image,
			core::gpu::utils::EImageLayout::Undefined,
			core::gpu::utils::EImageLayout::DepthStencilAttachment,
			true
		);
	}

	std::vector<core::gpu::CommandBuffer::RenderingAttachmentInfo> colorInfos;
	colorInfos.reserve(gBufferColorAttachments.size());

	for (const auto& colorAttachment : gBufferColorAttachments)
	{
		core::gpu::CommandBuffer::RenderingAttachmentInfo info
		{
			.image = colorAttachment.image.get(),
			.clear = true,
		};
		colorInfos.push_back(info);
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
	_cmdBuf.SetViewport(0.0f, 0.0f, m_device.GetSwapchainExtent().first, m_device.GetSwapchainExtent().second, 0.0f, 1.0f);
	_cmdBuf.SetScissor(0, 0, m_device.GetSwapchainExtent().first, m_device.GetSwapchainExtent().second);

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

	for (auto& colorAttachment : gBufferColorAttachments)
	{
		_cmdBuf.TransitionImageLayout(
			*colorAttachment.image,
			core::gpu::utils::EImageLayout::ColorAttachment,
			core::gpu::utils::EImageLayout::ShaderReadOnly,
			false
		);
	}

	_cmdBuf.TransitionImageLayout(
		*gBufferDepthAttachment.image,
		core::gpu::utils::EImageLayout::DepthStencilAttachment,
		core::gpu::utils::EImageLayout::ShaderReadOnly,
		true
	);
}

void Renderer::DrawShadow(core::gpu::CommandBuffer& _cmdBuf)
{
	auto [width, height] = m_device.GetSwapchainExtent();

	_cmdBuf.TransitionImageLayout(
		*shadowMaskAttachment.image,
		core::gpu::utils::EImageLayout::Undefined,
		core::gpu::utils::EImageLayout::General,
		false
	);

	ShadowPushConstants pc{};
	pc.lightDirection = glm::normalize(m_sunDirection);
	pc.maxDistance = 10000.0f;

	_cmdBuf.Bind<core::gpu::Pipeline>(*m_shadowPipeline);
	_cmdBuf.Bind(*shadowDescriptorSets[m_device.currentFrame], *m_shadowPipeline, 0u);
	_cmdBuf.PushConstants(
		*m_shadowPipeline,
		static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
		0, sizeof(ShadowPushConstants), &pc
	);

	_cmdBuf.Dispatch((width + 15) / 16, (height + 15) / 16, 1);

	_cmdBuf.TransitionImageLayout(
		*shadowMaskAttachment.image,
		core::gpu::utils::EImageLayout::General,
		core::gpu::utils::EImageLayout::ShaderReadOnly,
		false
	);
}

void Renderer::DrawGI(core::gpu::CommandBuffer& _cmdBuf)
{
	auto [width, height] = m_device.GetSwapchainExtent();

	_cmdBuf.TransitionImageLayout(
		*giColorAttachments[m_giParity].image,
		core::gpu::utils::EImageLayout::Undefined,
		core::gpu::utils::EImageLayout::General,
		false
	);

	GIPushConstants pc{};
	pc.sampleCount = 4;
	pc.maxDistance = 50.0f;
	pc.sunDirection = glm::normalize(m_sunDirection);
	pc.sunColor = glm::vec3(1.0f);

	_cmdBuf.Bind<core::gpu::Pipeline>(*m_giPipeline);
	_cmdBuf.Bind(*giDescriptorSets[m_device.currentFrame], *m_giPipeline, 0u);
	_cmdBuf.PushConstants(
		*m_giPipeline,
		static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
		0, sizeof(GIPushConstants), &pc
	);

	_cmdBuf.Dispatch((width + 15) / 16, (height + 15) / 16, 1);

	_cmdBuf.TransitionImageLayout(
		*giColorAttachments[m_giParity].image,
		core::gpu::utils::EImageLayout::General,
		core::gpu::utils::EImageLayout::ShaderReadOnly,
		false
	);
}

void Renderer::DrawResolve(core::gpu::CommandBuffer& _cmdBuf)
{
	auto [width, height] = m_device.GetSwapchainExtent();

	_cmdBuf.TransitionImageLayout(
		*resolveColorAttachments[0].image,
		core::gpu::utils::EImageLayout::Undefined,
		core::gpu::utils::EImageLayout::General,
		false
	);

	_cmdBuf.Bind<core::gpu::Pipeline>(*m_resolvePipeline);
	_cmdBuf.Bind(*resolveDescriptorSets[m_device.currentFrame], *m_resolvePipeline, 0u);

	ResolvePushConstants pc{};
	pc.numLights = static_cast<uint32_t>(m_pointLights.size());

	_cmdBuf.PushConstants(
		*m_resolvePipeline,
		static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
		0, sizeof(ResolvePushConstants), &pc
	);

	_cmdBuf.Dispatch((width + 15) / 16, (height + 15) / 16, 1);

	_cmdBuf.TransitionImageLayout(
		*resolveColorAttachments[0].image,
		core::gpu::utils::EImageLayout::General,
		core::gpu::utils::EImageLayout::ShaderReadOnly,
		false
	);
}

void Renderer::DrawFXAA(core::gpu::CommandBuffer& _cmdBuf)
{
	auto [width, height] = m_device.GetSwapchainExtent();

	_cmdBuf.TransitionImageLayout(
		*aaColorAttachments[0].image,
		core::gpu::utils::EImageLayout::Undefined,
		core::gpu::utils::EImageLayout::General,
		false
	);

	FXAAPushConstants pc{};
	pc.texelSize = glm::vec2(1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height));

	_cmdBuf.Bind<core::gpu::Pipeline>(*m_fxaaPipeline);
	_cmdBuf.Bind(*fxaaDescriptorSets[m_device.currentFrame], *m_fxaaPipeline, 0u);
	_cmdBuf.PushConstants(
		*m_fxaaPipeline,
		static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
		0, sizeof(FXAAPushConstants), &pc
	);

	_cmdBuf.Dispatch((width + 15) / 16, (height + 15) / 16, 1);

	_cmdBuf.TransitionImageLayout(
		*aaColorAttachments[0].image,
		core::gpu::utils::EImageLayout::General,
		core::gpu::utils::EImageLayout::TransferSrc,
		false
	);
}

void Renderer::DrawATrous(core::gpu::CommandBuffer& _cmdBuf)
{
	auto [width, height] = m_device.GetSwapchainExtent();

	constexpr int steps[5] = { 1, 2, 4, 8, 16 };

	for (int pass = 0; pass < 5; ++pass)
	{
		core::gpu::Image* outputImage = atrousAttachments[pass % 2].image.get();

		_cmdBuf.TransitionImageLayout(
			*outputImage,
			core::gpu::utils::EImageLayout::Undefined,
			core::gpu::utils::EImageLayout::General,
			false
		);

		_cmdBuf.Bind<core::gpu::Pipeline>(*m_atrousPipeline);
		_cmdBuf.Bind(*atrousDescriptorSets[m_device.currentFrame][pass], *m_atrousPipeline, 0u);

		ATrousPushConstants pc{};
		pc.stepSize = steps[pass];

		_cmdBuf.PushConstants(
			*m_atrousPipeline,
			static_cast<uint32_t>(core::gpu::utils::EShaderStageFlags::Compute),
			0, sizeof(ATrousPushConstants), &pc
		);

		_cmdBuf.Dispatch((width + 15) / 16, (height + 15) / 16, 1);

		_cmdBuf.TransitionImageLayout(
			*outputImage,
			core::gpu::utils::EImageLayout::General,
			core::gpu::utils::EImageLayout::ShaderReadOnly,
			false
		);
	}
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
		{
			std::cerr << "Warning: BLAS not found for mesh instance!\n";
			continue;
		}

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
	if (!_mesh || _mesh->meshTableIndex != UINT32_MAX) return;
	if (m_nextMeshTableIndex >= MAX_MESHES) { std::cerr << "Mesh table full!\n"; return; }

	_mesh->meshTableIndex = m_nextMeshTableIndex++;

	MaterialHandle firstMat = _mesh->instance.subMeshes.empty()
		? m_materialLibrary->GetDefaultMaterial()
		: _mesh->instance.subMeshes[0].material;

	if (firstMat == INVALID_MATERIAL)
		firstMat = m_materialLibrary->GetDefaultMaterial();

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