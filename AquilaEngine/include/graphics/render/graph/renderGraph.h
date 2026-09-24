#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_H
#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include <graphics/render/graph/pass.h>
#include <graphics/render/graph/resourceAllocator.h>

namespace core::gpu { class CommandBuffer; }

namespace graphics::render::graph
{
	class RenderGraph
	{
	private:
		struct PassNode
		{
			Pass* pass = nullptr;

			std::vector<size_t> dependencies;

			std::vector<ResourceHandle> toAcquire;
			std::vector<ResourceHandle> toRelease;

			std::vector<ResourceBarrier> barriers;
		};

		std::vector<Pass*> m_passes;

		std::vector<ResourceDesc> m_resourceDescs;
		std::vector<Resource*> m_resolvedResources;

		std::unordered_map<ResourceHandle, Resource*> m_importedResources;

		std::vector<PassNode> m_passNodes;
		std::vector<Pass*> m_compiledPasses;

		std::vector<PooledResource> m_pool;

		ResourceAllocator* m_allocator = nullptr;

		size_t m_passCounter = 0;

		Resource& AcquirePhysicalResource(const ResourceDesc& _desc, size_t _firstUse, size_t _lastUse);
		void ApplyBarriers(const PassNode& _node, core::gpu::CommandBuffer& _cmdBuf);

	public:
		explicit RenderGraph(ResourceAllocator* _allocator = nullptr)
			: m_allocator(_allocator)
		{
		}

		void AddPass(Pass& _pass);

		ResourceHandle CreateResource(const ResourceDesc& _desc);
		ResourceHandle ImportResource(Resource& _resource);

		void Reset();

		Resource& GetResource(ResourceHandle _handle);

		void Compile();
		void Execute(core::gpu::CommandBuffer& _cmdBuf);
	};
}

#endif // AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_H