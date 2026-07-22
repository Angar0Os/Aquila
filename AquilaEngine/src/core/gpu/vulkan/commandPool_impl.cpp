#include "commandPool_impl.h"
#include "device_impl.h"

#include <core/gpu/utils/converters.h>

using namespace core::gpu;

CommandPool::Impl::Impl(const Device* _device, const CommandPoolCreateInfo& _info)
	: device(_device), pool(nullptr), queueFamilyIndex(_info.queueFamilyIndex)
{
	vk::CommandPoolCreateInfo poolInfo{};

	poolInfo.flags = utils::ToVulkan(_info.flags);
	poolInfo.queueFamilyIndex = _info.queueFamilyIndex;

	pool = vk::raii::CommandPool(device->GetImpl().device, poolInfo);
}

CommandPool::Impl::~Impl() = default;

CommandPool::CommandPool(const Device* device, const CommandPoolCreateInfo& info)
{
	m_impl = std::make_unique<Impl>(device, info);
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