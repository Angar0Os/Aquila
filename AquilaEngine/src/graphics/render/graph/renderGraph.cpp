#include <graphics/render/graph/renderGraph.h>
#include <core/gpu/commandBuffer.h>
#include <core/debug/debug.h>

#include <algorithm>
#include <limits>
#include <unordered_map>

using namespace graphics::render::graph;
using namespace core::gpu;

void RenderGraph::AddPass(Pass& _pass)
{
	m_passes.push_back(&_pass);
}

ResourceHandle RenderGraph::CreateResource(const ResourceDesc& _desc)
{
	m_resourceDescs.push_back(_desc);
	return m_resourceDescs.size() - 1;
}

ResourceHandle RenderGraph::ImportResource(Resource& _resource)
{
	ResourceHandle handle = m_resourceDescs.size();

	m_resourceDescs.push_back(_resource.desc);
	m_importedResources.emplace(handle, &_resource);

	return handle;
}

void RenderGraph::Reset()
{
	m_passes.clear();
	m_resourceDescs.clear();
	m_importedResources.clear();
}

Resource& RenderGraph::GetResource(ResourceHandle _handle)
{
	if (_handle >= m_resolvedResources.size() ||
		m_resolvedResources[_handle] == nullptr)
	{
		AQUILA_ERROR(
			"RenderGraph : resource handle is not resolved yet "
			"(call Compile() first)."
		);
	}

	return *m_resolvedResources[_handle];
}

Resource& RenderGraph::AcquirePhysicalResource(
	const ResourceDesc& _desc,
	size_t _firstUse,
	size_t _lastUse)
{
	for (PooledResource& slot : m_pool)
	{
		if (!slot.allocated)
		{
			continue;
		}

		if (!slot.resource.desc.CanSatisfy(_desc))
		{
			continue;
		}

		if (slot.freeAtPassIndex >= _firstUse)
		{
			continue;
		}

		slot.freeAtPassIndex = _lastUse;

		return slot.resource;
	}

	PooledResource slot;

	slot.resource.name = _desc.name;
	slot.resource.desc = _desc;
	slot.resource.currentState = ResourceState::Undefined;

	slot.freeAtPassIndex = _lastUse;
	slot.allocated = true;

	if (m_allocator != nullptr)
	{
		m_allocator->Allocate(slot);
	}

	m_pool.push_back(std::move(slot));

	return m_pool.back().resource;
}

void RenderGraph::Compile()
{
	m_passNodes.clear();
	m_compiledPasses.clear();

	m_resolvedResources.assign(m_resourceDescs.size(), nullptr);

	m_passNodes.reserve(m_passes.size());

	for (Pass* pass : m_passes)
	{
		AQUILA_CHECK(pass != nullptr, "RenderGraph : null pass.");

		m_passNodes.push_back({ .pass = pass });
	}

	std::unordered_map<ResourceHandle, size_t> producers;

	for (size_t passIndex = 0; passIndex < m_passNodes.size(); ++passIndex)
	{
		Pass* pass = m_passNodes[passIndex].pass;

		for (const PassOutput& output : pass->GetOutputs())
		{
			if (output.handle == kInvalidResourceHandle)
			{
				AQUILA_ERROR(
					"RenderGraph : pass '" + pass->GetName() +
					"' has an invalid output resource."
				);
			}

			if (output.handle >= m_resourceDescs.size())
			{
				AQUILA_ERROR(
					"RenderGraph : pass '" + pass->GetName() +
					"' references an unknown output resource."
				);
			}

			auto [it, inserted] = producers.emplace(output.handle, passIndex);

			if (!inserted)
			{
				AQUILA_ERROR(
					"RenderGraph : resource '" + m_resourceDescs[output.handle].name +
					"' is produced by multiple passes."
				);
			}
		}
	}

	for (size_t passIndex = 0; passIndex < m_passNodes.size(); ++passIndex)
	{
		Pass* pass = m_passNodes[passIndex].pass;

		for (const PassInput& input : pass->GetInputs())
		{
			if (input.handle == kInvalidResourceHandle)
			{
				AQUILA_ERROR(
					"RenderGraph : pass '" + pass->GetName() +
					"' has an invalid input resource."
				);
			}

			if (input.handle >= m_resourceDescs.size())
			{
				AQUILA_ERROR(
					"RenderGraph : pass '" + pass->GetName() +
					"' references an unknown input resource."
				);
			}

			auto producer = producers.find(input.handle);

			if (producer == producers.end())
			{
				if (m_importedResources.find(input.handle) != m_importedResources.end())
				{
					continue;
				}

				AQUILA_ERROR(
					"RenderGraph : resource '" + m_resourceDescs[input.handle].name +
					"' required by pass '" + pass->GetName() +
					"' has no producer."
				);
			}

			const size_t producerIndex = producer->second;

			AQUILA_CHECK(producerIndex != passIndex,
				"RenderGraph : pass '" + pass->GetName() + "' depends on itself.");

			m_passNodes[passIndex].dependencies.push_back(producerIndex);
		}
	}

	for (PassNode& node : m_passNodes)
	{
		std::sort(node.dependencies.begin(), node.dependencies.end());
		node.dependencies.erase(
			std::unique(node.dependencies.begin(), node.dependencies.end()),
			node.dependencies.end()
		);
	}

	std::vector<size_t> compiledOrder;
	std::vector<bool> compiled(m_passNodes.size(), false);
	size_t compiledCount = 0;

	while (compiledCount < m_passNodes.size())
	{
		bool progress = false;

		for (size_t passIndex = 0; passIndex < m_passNodes.size(); ++passIndex)
		{
			if (compiled[passIndex])
			{
				continue;
			}

			bool dependenciesCompiled = true;

			for (size_t dependency : m_passNodes[passIndex].dependencies)
			{
				if (!compiled[dependency])
				{
					dependenciesCompiled = false;
					break;
				}
			}

			if (!dependenciesCompiled)
			{
				continue;
			}

			compiled[passIndex] = true;
			m_compiledPasses.push_back(m_passNodes[passIndex].pass);
			compiledOrder.push_back(passIndex);

			++compiledCount;
			progress = true;
		}

		AQUILA_CHECK(progress, "RenderGraph : dependency cycle detected.");
	}

	const size_t resourceCount = m_resourceDescs.size();

	std::vector<size_t> firstUse(resourceCount, std::numeric_limits<size_t>::max());
	std::vector<size_t> lastUse(resourceCount, 0);
	std::vector<bool> used(resourceCount, false);

	for (size_t order = 0; order < compiledOrder.size(); ++order)
	{
		const PassNode& node = m_passNodes[compiledOrder[order]];
		const size_t globalOrder = m_passCounter + order;

		auto touch = [&](ResourceHandle _handle)
			{
				if (_handle == kInvalidResourceHandle) return;

				AQUILA_CHECK(_handle < resourceCount, "RenderGraph : invalid resource handle.");

				used[_handle] = true;
				firstUse[_handle] = std::min(firstUse[_handle], globalOrder);
				lastUse[_handle] = std::max(lastUse[_handle], globalOrder);
			};

		for (const PassInput& input : node.pass->GetInputs()) touch(input.handle);
		for (const PassOutput& output : node.pass->GetOutputs()) touch(output.handle);
	}

	std::vector<ResourceHandle> handlesByFirstUse;
	handlesByFirstUse.reserve(resourceCount);

	for (ResourceHandle handle = 0; handle < resourceCount; ++handle)
	{
		if (used[handle])
		{
			handlesByFirstUse.push_back(handle);
		}
	}

	std::sort(handlesByFirstUse.begin(), handlesByFirstUse.end(),
		[&](ResourceHandle _a, ResourceHandle _b) { return firstUse[_a] < firstUse[_b]; });

	for (ResourceHandle handle : handlesByFirstUse)
	{
		auto importedIt = m_importedResources.find(handle);

		if (importedIt != m_importedResources.end())
		{
			m_resolvedResources[handle] = importedIt->second;
			continue;
		}

		Resource& physical = AcquirePhysicalResource(
			m_resourceDescs[handle], firstUse[handle], lastUse[handle]
		);

		m_resolvedResources[handle] = &physical;
	}

	for (ResourceHandle handle : handlesByFirstUse)
	{
		const size_t localFirstUse = firstUse[handle] - m_passCounter;
		const size_t localLastUse = lastUse[handle] - m_passCounter;

		m_passNodes[compiledOrder[localFirstUse]].toAcquire.push_back(handle);
		m_passNodes[compiledOrder[localLastUse]].toRelease.push_back(handle);
	}

	for (size_t order = 0; order < compiledOrder.size(); ++order)
	{
		PassNode& node = m_passNodes[compiledOrder[order]];
		node.barriers.clear();

		auto addBarrierIfNeeded = [&](ResourceHandle _handle, ResourceState _requiredState)
			{
				if (_handle == kInvalidResourceHandle) return;

				Resource& physical = *m_resolvedResources[_handle];

				if (physical.currentState == _requiredState) return;

				node.barriers.push_back({
					.resource = &physical,
					.from = physical.currentState,
					.to = _requiredState
					});

				physical.currentState = _requiredState;
			};

		for (const PassInput& input : node.pass->GetInputs())
			addBarrierIfNeeded(input.handle, input.requiredState);

		for (const PassOutput& output : node.pass->GetOutputs())
			addBarrierIfNeeded(output.handle, output.requiredState);
	}

	m_passCounter += compiledOrder.size();
}

void RenderGraph::ApplyBarriers(const PassNode& _node, core::gpu::CommandBuffer& _cmdBuf)
{
	for (const ResourceBarrier& barrier : _node.barriers)
	{
		if (barrier.resource == nullptr) continue;

		Resource& resource = *barrier.resource;

		if (resource.desc.type != ResourceType::Texture) continue;

		if (resource.image == nullptr)
		{
			AQUILA_ERROR(
				"RenderGraph : texture resource '" + resource.desc.name + "' has no image."
			);
		}

		const bool isDepth = resource.desc.isDepthFormat;

		_cmdBuf.TransitionImageLayout(*resource.image, ToImageLayout(barrier.to), isDepth);
	}
}

void RenderGraph::Execute(core::gpu::CommandBuffer& _cmdBuf)
{
	for (PassNode& node : m_passNodes)
	{
		if (node.pass == nullptr) continue;

		ApplyBarriers(node, _cmdBuf);

		_cmdBuf.BeginDebugLabel(node.pass->GetName());
		node.pass->Execute();
		_cmdBuf.EndDebugLabel();
	}
}