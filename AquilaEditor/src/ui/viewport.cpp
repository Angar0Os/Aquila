#include <ui/viewport.h>

#include <core/window.h>
#include <core/gpu/commandBuffer.h>

#include <graphics/render/renderer.h>

#include <imgui/context.h>
#include <imgui/imgui.h>

#include <algorithm>
#include <GLFW/glfw3.h>

using namespace ui;

Viewport::Viewport(core::Window& _window, imgui::Context& _imguiContext, graphics::render::Renderer& _renderer)
	: m_window(&_window), m_imguiContext(&_imguiContext), m_renderer(&_renderer)
{
}

Viewport::~Viewport()
{
}

void Viewport::Render(core::gpu::CommandBuffer* _cmdBuf)
{
	m_imguiContext->RenderSceneToViewport(_cmdBuf, m_renderer);
}

void Viewport::Draw()
{
	if (!ImGui::Begin("Viewport"))
	{
		ImGui::End();
		return;
	}

	const bool isFocused = ImGui::IsWindowFocused();

	if (isFocused != m_wasFocused)
	{
	}

	m_wasFocused = isFocused;

	ImVec2 avail = ImGui::GetContentRegionAvail();
	uint32_t width = std::max(1u, static_cast<uint32_t>(avail.x));
	uint32_t height = std::max(1u, static_cast<uint32_t>(avail.y));

	m_imguiContext->DrawViewportComponent(width, height);

	if (m_imguiContext->GetViewportState()->hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();
		float centerX = (min.x + max.x) * 0.5f;
		float centerY = (min.y + max.y) * 0.5f;

		glfwSetCursorPos(m_window->GetHandle(), centerX, centerY);
		glfwSetInputMode(m_window->GetHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		if (glfwRawMouseMotionSupported())
		{
			glfwSetInputMode(m_window->GetHandle(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
	}
	else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		if (glfwRawMouseMotionSupported())
		{
			glfwSetInputMode(m_window->GetHandle(), GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
		}
		glfwSetInputMode(m_window->GetHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	ImGui::End();
}