#ifndef AQUILA_ENGINE_CORE_IMGUI_CONTEXT_H
#define AQUILA_ENGINE_CORE_IMGUI_CONTEXT_H
#pragma once

#include <memory>

namespace core { class Window; }
namespace core::gpu { class Device; class CommandBuffer; class Image; }

namespace core::imgui
{
	class Context
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;

		const core::gpu::Device& m_device;
	public:
		explicit Context(const core::Window& _window, const core::gpu::Device& _device);
		~Context() noexcept;

		void Render(const core::gpu::CommandBuffer& _cmdBuf, const core::gpu::Image& _outputImage);
	};
}

#endif //AQUILA_ENGINE_CORE_IMGUI_CONTEXT_H
