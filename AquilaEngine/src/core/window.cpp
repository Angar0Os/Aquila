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

	m_window = glfwCreateWindow(_wndDesc.windowSize[0], _wndDesc.windowSize[1], _wndDesc.appName.c_str(), nullptr, nullptr);

	if (!m_window)
	{
		throw std::runtime_error("Failed to create GLFW window");
	}

	glfwSetWindowUserPointer(m_window, this);
	//glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback); // TODO : Penser à faire le resize, il faudra centraliser le resize pour le rendre plus simple
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
