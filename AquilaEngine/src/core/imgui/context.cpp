#include "context_impl.h"

#include "../gpu/vulkan/device_impl.h"
#include "../gpu/vulkan/descriptorPool_impl.h"
#include "../gpu/vulkan/commandBuffer_impl.h"
#include "../gpu/vulkan/image_impl.h"

#include <core/window.h>

#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

using namespace core::gpu;
using namespace core::imgui;

Context::Context(const core::Window& _window, const core::gpu::Device& _device)
	: m_impl(new Impl), m_device(_device)
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
}

Context::~Context()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Context::Render(const core::gpu::CommandBuffer& _cmdBuf, const core::gpu::Image& _outputImage)
{

}