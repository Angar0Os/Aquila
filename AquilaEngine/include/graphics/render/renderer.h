#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
#pragma once


namespace core::gpu { class CommandBuffer; }
namespace graphics::deferred { class RenderGraph; class PassAttachment; }

namespace graphics::render
{
	class Renderer
	{
	public:
		explicit Renderer();
		~Renderer() noexcept;

		void Render(core::gpu::CommandBuffer& _cmdBuf);

	private:
		void BuildGBufferPass(graphics::deferred::RenderGraph& _graph);
		void BuildLightingPass(graphics::deferred::RenderGraph& _graph);

		graphics::deferred::PassAttachment* m_albedo	= nullptr;
		graphics::deferred::PassAttachment* m_normal	= nullptr;
		graphics::deferred::PassAttachment* m_depth		= nullptr;
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
