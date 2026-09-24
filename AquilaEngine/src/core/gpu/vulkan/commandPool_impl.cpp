#include "commandPool_impl.h"
#include "device_impl.h"

#include <core/gpu/utils/converters.h>

using namespace core::gpu;

CommandPool::CommandPool(const Device& _device, const CommandPoolCreateInfo& _info)
	: m_impl(new Impl)
{
	vk::CommandPoolCreateInfo poolInfo{};

	poolInfo.flags = utils::ToVulkan(_info.flags);
	poolInfo.queueFamilyIndex = _info.queueFamilyIndex;

	m_impl->pool = vk::raii::CommandPool(_device.GetImpl().device, poolInfo);
}

CommandPool::~CommandPool() = default;

void core::gpu::CommandPool::Reset(bool _releaseResources)
{
	vk::CommandPoolResetFlags flags;
	if (_releaseResources)
	{
		flags |= vk::CommandPoolResetFlagBits::eReleaseResources;
	}

	m_impl->pool.reset(flags);
}

CommandPool::Impl& CommandPool::GetImpl() const
{
	return *m_impl;
}