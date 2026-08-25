#include <imgui/context.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "../../../AquilaEngine/src/core/gpu/vulkan/commandBuffer_impl.h"
#include "../../../AquilaEngine/src/core/gpu/vulkan/device_impl.h"
#include "../../../AquilaEngine/src/core/gpu/vulkan/descriptorPool_impl.h"
#include "../../../AquilaEngine/src/core/gpu/vulkan/descriptorSet_impl.h"
#include "../../../AquilaEngine/src/core/gpu/vulkan/image_impl.h"

#include <core/gpu/utils/converters.h>
#include <core/gpu/commandBuffer.h>

#include <graphics/render/renderer.h>

#include "IconsMaterialDesignIcons.h"

#include <core/window.h>

#include <algorithm>

using namespace core::gpu;
using namespace imgui;

Context::Context(const core::Window& _window, const core::gpu::Device& _device)
	: m_device(_device)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	io.Fonts->AddFontDefault();
	static const ImWchar icons_ranges[] = { ICON_MIN_MDI, ICON_MAX_MDI, 0 };

	ImFontConfig icons_config_small;
	icons_config_small.MergeMode = true;
	icons_config_small.PixelSnapH = true;
	icons_config_small.GlyphMinAdvanceX = 13.0f;
	io.Fonts->AddFontFromFileTTF("assets/fonts/" FONT_ICON_FILE_NAME_MDI, 13.0f, &icons_config_small, icons_ranges);

	ImFontConfig icons_config_large;
	icons_config_large.PixelSnapH = true;
	icons_config_large.GlyphMinAdvanceX = 50.0f;
	io.Fonts->AddFontFromFileTTF("assets/fonts/" FONT_ICON_FILE_NAME_MDI, 50.0f, &icons_config_large, icons_ranges);

	ImGui_ImplGlfw_InitForVulkan(_window.GetHandle(), true);

	io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	if (vkCreateDescriptorPool(*_device.GetImpl().device, &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create ImGui descriptor pool");

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = *_device.GetImpl().instance;
	init_info.PhysicalDevice = *_device.GetImpl().physicalDevice;
	init_info.Device = *_device.GetImpl().device;
	init_info.QueueFamily = _device.GetImpl().queueIndex;
	init_info.Queue = *_device.GetImpl().graphicsQueue;
	init_info.DescriptorPool = imguiDescriptorPool;
	init_info.MinImageCount = 2;
	init_info.ImageCount = _device.GetImpl().swapchainImages.size();
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.RenderPass = VK_NULL_HANDLE;
	init_info.UseDynamicRendering = VK_TRUE;
	init_info.PipelineRenderingCreateInfo = {};
	init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	VkFormat format = static_cast<VkFormat>(_device.GetImpl().swapchainImageFormat);
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
	init_info.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

	ImGui_ImplVulkan_Init(&init_info);

	currentViewportState = &m_viewportInfo;

	InitViewport({ 1080, 720 });
}

Context::~Context()
{
	if (m_viewportInfo.dsSet != VK_NULL_HANDLE)
	{
		ImGui_ImplVulkan_RemoveTexture(m_viewportInfo.dsSet);
		m_viewportInfo.dsSet = VK_NULL_HANDLE;
	}

	m_viewportInfo.sampler = nullptr;
	m_viewportInfo.colorImage.reset();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	if (imguiDescriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(*m_device.GetImpl().device, imguiDescriptorPool, nullptr);
		imguiDescriptorPool = VK_NULL_HANDLE;
	}
}

void Context::BeginFrame(core::gpu::CommandBuffer* _cmdBuf, core::gpu::Image* _outputImage)
{
	// L'image du swapchain n'est jamais transitionnee automatiquement par
	// AcquireNextImage(): elle sort soit en UNDEFINED (premiere frame), soit
	// dans le layout laisse par le Present precedent (PRESENT_SRC_KHR).
	// BeginRendering exige COLOR_ATTACHMENT_OPTIMAL pour un color attachment,
	// donc on transitionne explicitement avant. clear = true sur l'attachment
	// donc le contenu precedent (jete par oldLayout = Undefined) n'a pas
	// besoin d'etre preserve.
	_cmdBuf->TransitionImageLayout(
		*_outputImage,
		core::gpu::utils::EImageLayout::Undefined,
		core::gpu::utils::EImageLayout::ColorAttachment,
		false
	);

	CommandBuffer::RenderingAttachmentInfo colorAttachment
	{
		.image = _outputImage,
		.clear = true
	};

	_cmdBuf->BeginRendering(m_device, { colorAttachment }, {});

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

	ImGui::NewFrame();
}

void Context::EndFrame(core::gpu::CommandBuffer* _cmdBuf, core::gpu::Image* _outputImage)
{
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *_cmdBuf->GetImpl().commandBuffer);
	ImGui::EndFrame();

	_cmdBuf->EndRendering();
}

void Context::RenderSceneToViewport(core::gpu::CommandBuffer* _cmdBuf, graphics::render::Renderer* _renderer)
{
	ResizeViewportIfNeeded();

	if (!currentViewportState->colorImage || !currentViewportState->isUsable)
	{
		return;
	}

	_renderer->Render(_cmdBuf, *currentViewportState->colorImage);

	_cmdBuf->TransitionImageLayout(
		*currentViewportState->colorImage,
		core::gpu::utils::EImageLayout::ColorAttachment,
		core::gpu::utils::EImageLayout::ShaderReadOnly,
		false
	);

	currentViewportState->hasBeenRendered = true;
}

void Context::DrawViewportComponent(uint32_t _width, uint32_t _height)
{
	currentViewportState->desiredSize = { _width, _height };

	if (!currentViewportState->isUsable || currentViewportState->dsSet == VK_NULL_HANDLE)
	{
		currentViewportState->hovered = false;
		currentViewportState->focused = false;
		return;
	}

	ImGui::Image(
		(ImTextureID)currentViewportState->dsSet,
		ImVec2(
			static_cast<float>(currentViewportState->size.first),
			static_cast<float>(currentViewportState->size.second)
		)
	);

	currentViewportState->hovered = ImGui::IsItemHovered();
	currentViewportState->focused = ImGui::IsWindowFocused();
}

core::gpu::Image* Context::GetViewportImage()
{
	return currentViewportState->colorImage.get();
}

void Context::ResizeViewportIfNeeded()
{
	auto& desired = currentViewportState->desiredSize;

	if (desired.first <= 1 || desired.second <= 1) return;
	if (currentViewportState->colorImage && currentViewportState->size == desired) return;

	m_device.GetImpl().device.waitIdle();

	if (currentViewportState->dsSet != VK_NULL_HANDLE)
	{
		ImGui_ImplVulkan_RemoveTexture(currentViewportState->dsSet);
		currentViewportState->dsSet = VK_NULL_HANDLE;
	}

	currentViewportState->sampler = nullptr;
	currentViewportState->colorImage.reset();
	currentViewportState->isUsable = false;
	currentViewportState->hasBeenRendered = false;

	InitViewport(desired);
}

void Context::InitViewport(std::pair<uint32_t, uint32_t> _viewportSize)
{
	currentViewportState->size = {
		std::max(1u, _viewportSize.first),
		std::max(1u, _viewportSize.second)
	};

	core::gpu::ImageCreateInfo imageInfo
	{
		.width = currentViewportState->size.first,
		.height = currentViewportState->size.second,
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = core::gpu::utils::FromVulkan(m_device.GetImpl().swapchainImageFormat),
		.tiling = core::gpu::utils::EImageTiling::Optimal,
		.usage = core::gpu::utils::EImageUsage::ColorAttachment | core::gpu::utils::EImageUsage::Sampled | core::gpu::utils::EImageUsage::TransferDst,
		.memoryProperties = core::gpu::utils::EMemoryProperty::DeviceLocal,
		.samples = core::gpu::utils::ESampleCount::e1
	};

	currentViewportState->colorImage = std::make_unique<Image>(m_device, imageInfo);

	vk::SamplerCreateInfo samplerInfo{};
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	currentViewportState->sampler = vk::raii::Sampler(m_device.GetImpl().device, samplerInfo);

	currentViewportState->dsSet = ImGui_ImplVulkan_AddTexture(
		*currentViewportState->sampler,
		*currentViewportState->colorImage->GetImpl().view,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	currentViewportState->isUsable = true;
}