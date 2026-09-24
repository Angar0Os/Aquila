#include <core/window.h>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")

using namespace core;

Window::Window(const WindowDesc _wndDesc)
	: m_wndInfo(_wndDesc)
{
	if (!glfwInit())
	{
		throw std::runtime_error("Failed to initialize GLFW.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, m_wndInfo.isResizable ? GLFW_TRUE : GLFW_FALSE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	int width = _wndDesc.windowSize[0];
	int height = _wndDesc.windowSize[1];
	GLFWmonitor* targetMonitor = nullptr;

	if (_wndDesc.isFullscreen)
	{
		if (_wndDesc.exclusiveFullscreen)
		{
			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

			targetMonitor = monitor;
			width = mode->width;
			height = mode->height;
		}
		else
		{
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
			width = mode->width;
			height = mode->height;
		}
	}

	m_window = glfwCreateWindow(width, height, _wndDesc.appName.c_str(), targetMonitor, nullptr);

	if (!m_window)
	{
		throw std::runtime_error("Failed to create GLFW window");
	}

	if (_wndDesc.isFullscreen && !_wndDesc.exclusiveFullscreen)
	{
		int monitorX, monitorY;
		glfwGetMonitorPos(monitor, &monitorX, &monitorY);
		glfwSetWindowPos(m_window, monitorX, monitorY);
	}

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
}

void Window::FramebufferResizeCallback(GLFWwindow* _window, int, int)
{
	auto* self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(_window));
	if (self)
	{
		self->m_framebufferResized = true;
	}
}

bool Window::WasFramebufferResized()
{
	bool wasResized = m_framebufferResized;
	m_framebufferResized = false;
	return wasResized;
}

void Window::SetFullscreen(bool _fullscreen, bool _exclusive)
{
	if (_fullscreen == m_wndInfo.isFullscreen)
		return;

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	if (_fullscreen)
	{
		glfwGetWindowPos(m_window, &m_windowedPosX, &m_windowedPosY);
		glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);

		if (_exclusive)
		{
			glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		else
		{
			glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_FALSE);

			int monitorX, monitorY;
			glfwGetMonitorPos(monitor, &monitorX, &monitorY);
			glfwSetWindowMonitor(m_window, nullptr, monitorX, monitorY, mode->width, mode->height, GLFW_DONT_CARE);
		}
	}
	else
	{
		glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);
		glfwSetWindowMonitor(m_window, nullptr, m_windowedPosX, m_windowedPosY, m_windowedWidth, m_windowedHeight, GLFW_DONT_CARE);
	}

	m_wndInfo.isFullscreen = _fullscreen;
	m_wndInfo.exclusiveFullscreen = _exclusive;

	m_framebufferResized = true;
}

void Window::ToggleFullscreen()
{
	SetFullscreen(!m_wndInfo.isFullscreen, m_wndInfo.exclusiveFullscreen);
}

std::pair<int, int> Window::GetFramebufferSize() const
{
	int w, h;
	glfwGetFramebufferSize(m_window, &w, &h);
	return { w, h };
}

Window::~Window()
{
	glfwTerminate();
}

void Window::PollEvents()
{
	glfwPollEvents();
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(m_window);
}

void Window::Close() const 
{
	if (m_window) 
	{
		glfwSetWindowShouldClose(m_window, GLFW_TRUE);
	}
}
