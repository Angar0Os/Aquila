#include <imgui/context.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "../../../AquilaEngine/src/core/gpu/vulkan/commandBuffer_impl.h"
#include "../../../AquilaEngine/src/core/gpu/vulkan/device_impl.h"
#include "../../../AquilaEngine/src/core/gpu/vulkan/descriptorPool_impl.h"
#include "../../../AquilaEngine/src/core/gpu/vulkan/descriptorSet_impl.h"
#include "../../../AquilaEngine/src/core/gpu/vulkan/image_impl.h"

#include <core/gpu/utils/converters.h>

#include <core/window.h>

using namespace core::gpu;
using namespace imgui;

Context::Context(const core::Window& _window, const core::gpu::Device& _device)
	: m_device(_device)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	
	auto io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplGlfw_InitForVulkan(_window.GetHandle(), true);
	io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

	VkFormat format = static_cast<VkFormat>(_device.GetImpl().swapchainImageFormat);
	ImGui_ImplVulkan_InitInfo initInfo
	{
		.Instance = *_device.GetImpl().instance,
		.PhysicalDevice = *_device.GetImpl().physicalDevice,
		.Device = *_device.GetImpl().device,
		.QueueFamily = _device.GetImpl().queueIndex,
		.Queue = *_device.GetImpl().graphicsQueue,
		.DescriptorPool = *_device.GetImpl().descriptorPool->GetImpl().pool,
		.RenderPass = VK_NULL_HANDLE,
		.MinImageCount = 2,
		.ImageCount = static_cast<uint32_t>(_device.GetImpl().swapchainImages.size()),
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.UseDynamicRendering = VK_TRUE,
		.PipelineRenderingCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &format,
			.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
		}
	};

	ImGui_ImplVulkan_Init(&initInfo);

	ViewportInfo infos
	{
		.colorImage = nullptr,
		.dsSet = VK_NULL_HANDLE,
		.desiredSize = { 0.0, 0.0 },
		.isUsable = true
	};

	currentViewportState = &infos;
}

Context::~Context()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Context::BeginFrame(core::gpu::CommandBuffer* _cmdBuf, core::gpu::Image* _outputImage)
{
	if (!currentViewportState->colorImage || !currentViewportState->isUsable)
		return;

	if (currentViewportState->desiredSize.first <= 1 || currentViewportState->desiredSize.second <= 1)
		return;

	if (currentViewportState->desiredSize.first == 0 || currentViewportState->desiredSize.second == 0)
		return;

	if (!currentViewportState->colorImage)
	{
		InitViewport(currentViewportState->desiredSize);
	}

	CommandBuffer::RenderingAttachmentInfo colorAttachment
	{
		.image = _outputImage,
		.clear = false
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

void Context::InitViewport(std::pair<uint32_t, uint32_t> _viewportSize)
{
	currentViewportState->desiredSize = _viewportSize;

	core::gpu::ImageCreateInfo imageInfo
	{
		.width = _viewportSize.first,
		.height = _viewportSize.second,
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

	auto sampler = vk::raii::Sampler(m_device.GetImpl().device, samplerInfo);

	if (currentViewportState->dsSet == VK_NULL_HANDLE)
	{
		currentViewportState->dsSet = ImGui_ImplVulkan_AddTexture(
			*sampler,
			*currentViewportState->colorImage->GetImpl().view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	currentViewportState->isUsable = true;
}