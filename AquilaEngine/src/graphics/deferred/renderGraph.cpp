#include <core/gpu/commandBuffer.h>

#include <graphics/deferred/renderGraph.h>
#include <graphics/deferred/pass.h>

using namespace graphics::deferred;

void RenderGraph::Compile()
{
	BuildDependencies();
	TopologicalSort();
	AllocateResources();
	BuildBarriers();
	Optimize();
	BuildGraph();
}

void RenderGraph::Execute(core::gpu::CommandBuffer& _cmdBuf)
{
	for (auto pass : m_passesOrdered)
	{
		pass.execute(_cmdBuf);
	}
}

void RenderGraph::AddPass(const std::string& name, std::function<void(PassBuilder&)> setup, std::function<void(core::gpu::CommandBuffer&)> execute)
{
	Pass pass;
	PassBuilder builder(pass, m_attachments);

	setup(builder);

	pass.execute = execute;

	m_passes.push_back(std::move(pass));
}

