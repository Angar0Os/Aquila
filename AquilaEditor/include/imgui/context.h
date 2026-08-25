#ifndef AQUILA_EDITOR_IMGUI_CONTEXT_H
#define AQUILA_EDITOR_IMGUI_CONTEXT_H
#pragma once

#include <memory>
#include <utility>
#include <vulkan/vulkan_raii.hpp>

namespace core { class Window; }
namespace core::gpu { class Device; class CommandBuffer; class Image; }
namespace graphics::render { class Renderer; }

namespace imgui
{
	struct ViewportInfo
	{
		std::unique_ptr<core::gpu::Image>	colorImage = nullptr;
		vk::raii::Sampler					sampler = nullptr;
		VkDescriptorSet						dsSet = VK_NULL_HANDLE;
		std::pair<uint32_t, uint32_t>		size = { 0, 0 };		
		std::pair<uint32_t, uint32_t>		desiredSize = { 0, 0 };	
		bool								isUsable = false;
		bool								hasBeenRendered = false;	
		bool								hovered = false;
		bool								focused = false;
	};

	class Context
	{
	private:
		const core::gpu::Device& m_device;

		ViewportInfo m_viewportInfo;
		ViewportInfo* currentViewportState = nullptr;
		VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;

		void InitViewport(std::pair<uint32_t, uint32_t> _viewportSize);
		void ResizeViewportIfNeeded();

	public:
		explicit Context(const core::Window& _window, const core::gpu::Device& _device);
		~Context() noexcept;

		void BeginFrame(core::gpu::CommandBuffer* _cmdBuf, core::gpu::Image* _outputImage);
		void EndFrame(core::gpu::CommandBuffer* _cmdBuf, core::gpu::Image* _outputImage);

		void RenderSceneToViewport(core::gpu::CommandBuffer* _cmdBuf, graphics::render::Renderer* _renderer);
		void DrawViewportComponent(uint32_t _width, uint32_t _height);

		core::gpu::Image* GetViewportImage();
		ViewportInfo* GetViewportState() { return &m_viewportInfo; }
	};
}

#endif //AQUILA_ENGINE_CORE_IMGUI_CONTEXT_H