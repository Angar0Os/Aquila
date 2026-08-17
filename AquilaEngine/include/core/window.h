#ifndef AQUILA_ENGINE_CORE_WINDOW_H
#define AQUILA_ENGINE_CORE_WINDOW_H
#pragma once

#include <string>
#include <glm/vec2.hpp>

struct GLFWwindow;

namespace core
{
	struct WindowDesc
	{
		std::string appName;
		glm::vec2	windowSize;
		bool		isFullscreen	= false;
		bool		isResizable		= false;
		bool		exclusiveFullscreen = false;
	};

	class Window
	{
	private:
		WindowDesc m_wndInfo;
		GLFWwindow* m_window;

		static void FramebufferResizeCallback(GLFWwindow* _window, int _width, int _height);
		bool		m_framebufferResized = false;
	public:
		explicit Window(const WindowDesc _wndDesc);
		~Window() noexcept;

		void				PollEvents();
		void				Close()			const;

		bool				ShouldClose()	const;

		std::string			GetAppName()	const { return m_wndInfo.appName; } 
		
		const glm::vec2		GetWindowSize() { return m_wndInfo.windowSize; }
		
		const bool			IsFullscreen()	{ return m_wndInfo.isFullscreen; }
		const bool			IsResizable()	{ return m_wndInfo.isResizable; }

		GLFWwindow*			GetHandle()		const { return m_window; }

		bool				WasFramebufferResized();          
		std::pair<int, int> GetFramebufferSize() const;
	};
}

#endif //AQUILA_ENGINE_CORE_WINDOW_H
