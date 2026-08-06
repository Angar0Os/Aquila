#ifndef AQUILA_ENGINE_GRAPHICS_DEFERRED_RENDER_GRAPH_H
#define AQUILA_ENGINE_GRAPHICS_DEFERRED_RENDER_GRAPH_H
#pragma once

#include <vector>
#include <functional>
#include <graphics/deferred/passBuilder.h>

namespace graphics::deferred
{
	class RenderGraph
	{
	public:
		void Compile();
		void Execute(core::gpu::CommandBuffer& cmd);
		void AddPass(const std::string& name, std::function<void(PassBuilder&)> setup, std::function<void(core::gpu::CommandBuffer&)> execute);

		std::vector<std::unique_ptr<PassAttachment>> attachments;
	private:
		void BuildDependencies();
		void TopologicalSort();
		void AllocateResources();
		void BuildBarriers();
		void Optimize();
		void BuildGraph();

		std::vector<Pass> m_passes;
		std::vector<Pass> m_passesOrdered;
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_DEFERRED_RENDER_GRAPH_H
