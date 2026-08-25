#ifndef AQUILA_EDITOR_UI_VIEWPORT_H
#define AQUILA_EDITOR_UI_VIEWPORT_H
#pragma once

namespace core
{
	class Window;
}

namespace core::gpu
{
	class CommandBuffer;
}

namespace graphics::render
{
	class Renderer;
}

namespace imgui
{
	class Context;
}

namespace ui
{
	class Viewport
	{
	public:
		Viewport(core::Window& _window, imgui::Context& _imguiContext, graphics::render::Renderer& _renderer);
		~Viewport();

		void Render(core::gpu::CommandBuffer* _cmdBuf);
		void Draw();

	private:
		core::Window* m_window;
		imgui::Context* m_imguiContext;
		graphics::render::Renderer* m_renderer;

		bool m_wasFocused = false;
	};
}

#endif //AQUILA_EDITOR_UI_VIEWPORT_H