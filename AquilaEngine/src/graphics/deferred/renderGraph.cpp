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
	PassBuilder builder(pass, attachments);

	setup(builder);

	pass.execute = execute;

	m_passes.push_back(std::move(pass));
}

void RenderGraph::BuildDependencies()
{
	for (auto& producer : m_passes)
	{
		for (auto& output : producer.outputs)
		{
			for (auto& consumer : m_passes)
			{
				if(&producer == &consumer)
					continue;

				for (auto* input : consumer.inputs)
				{
					AddDependency(producer, consumer, *output);
				}
			}
		}
	}
}

void RenderGraph::AddDependency(Pass& _producer, Pass& _consumer, PassAttachment& _attachment)
{
	m_dependencies.push_back({
		&_producer,
		&_consumer,
		&_attachment
	});
}
