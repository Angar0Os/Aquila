#include "commandBuffer_impl.h"
#include "commandPool_impl.h"
#include "device_impl.h"

using namespace core::gpu;

CommandBuffer::CommandBuffer(const Device& _device)
	: m_impl(new Impl)
{
	vk::CommandBufferAllocateInfo allocInfo{};

	allocInfo.commandPool = _device.GetImpl().commandPool->GetImpl().pool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = 1;

	m_impl->commandBuffer = std::move(vk::raii::CommandBuffers(_device.GetImpl().device, allocInfo).front());
	m_impl->semaphore = vk::raii::Semaphore(_device.GetImpl().device, vk::SemaphoreCreateInfo{});

	vk::FenceCreateInfo fenceInfos{};
	fenceInfos.flags = vk::FenceCreateFlagBits::eSignaled;

	m_impl->isGpuFree = vk::raii::Fence(_device.GetImpl().device, fenceInfos);
	m_impl->isCpuFree = true;
}

CommandBuffer::~CommandBuffer() = default;

bool CommandBuffer::IsCpuFree() const
{
	return m_impl->isCpuFree;
}

bool CommandBuffer::IsGpuFree() const
{
	return m_impl->isGpuFree.getStatus() == vk::Result::eSuccess;
}

void CommandBuffer::WaitForCompletion()
{
	auto fence = (VkFence)*m_impl->isGpuFree;
	vkWaitForFences(m_impl->isGpuFree.getDevice(), 1, &fence, true, UINT64_MAX);
}

void CommandBuffer::Record(std::function<void()> content)
{
	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.flags = m_impl->isSingleTime ? vk::CommandBufferUsageFlagBits::eOneTimeSubmit : vk::CommandBufferUsageFlags(0);
	m_impl->commandBuffer.begin(beginInfo);

	content();

	m_impl->commandBuffer.end();
}