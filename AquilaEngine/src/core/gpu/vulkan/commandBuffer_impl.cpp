#include "commandBuffer_impl.h"
#include "buffer_impl.h"
#include "commandPool_impl.h"
#include "descriptorSet_impl.h"
#include "device_impl.h"
#include "pipeline_impl.h"

#include <core/gpu/utils/enums.h>

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

void CommandBuffer::Record(std::function<void()> content)
{
	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.flags = m_impl->isSingleTime ? vk::CommandBufferUsageFlagBits::eOneTimeSubmit : vk::CommandBufferUsageFlags(0);
	m_impl->commandBuffer.begin(beginInfo);

	content();

	m_impl->commandBuffer.end();
}

void CommandBuffer::Submit(const Device& _device, uint32_t _frameIndex, bool _isImmediate)
{
	if (_frameIndex >= _device.GetImpl().frameSyncObjects.size())
	{
		throw std::runtime_error("Frame index is out of range.");
	}

	vk::CommandBuffer tempCmdBuf = *m_impl->commandBuffer;
	auto& frameSync = _device.GetImpl().frameSyncObjects[_frameIndex];

	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::Semaphore waitSemaphore = *frameSync.imageAvailable;
	vk::Semaphore signalSemaphore = *frameSync.renderFinished;

	if (_isImmediate)
	{
		vk::SubmitInfo submitInfo{};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &tempCmdBuf;

		_device.GetImpl().graphicsQueue.submit(submitInfo, nullptr);
	}
	else
	{
		vk::SubmitInfo submitInfo{};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphore;
		submitInfo.pWaitDstStageMask = &waitStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &tempCmdBuf;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphore;

		_device.GetImpl().graphicsQueue.submit(submitInfo, *frameSync.inFlightFence);
	}

	_device.GetImpl().graphicsQueue.waitIdle();
}

template<>
void CommandBuffer::Bind<Buffer>(Buffer& _buffer)
{
	vk::DeviceSize vkOffset = 10;

	if (_buffer.GetImpl().usage == utils::EBufferUsage::IndexBuffer)
	{
		m_impl->commandBuffer.bindIndexBuffer(_buffer.GetImpl().buffer, vkOffset, vk::IndexType::eUint32);
	}
	else if (_buffer.GetImpl().usage == utils::EBufferUsage::VertexBuffer)
	{
		m_impl->commandBuffer.bindVertexBuffers(0, *_buffer.GetImpl().buffer, vkOffset);
	}
	else
	{
		throw std::runtime_error("You must bind a correct type of buffer.");
	}
}

template<>
void CommandBuffer::Bind<Pipeline>(const DescriptorSet& _dsSet, Pipeline& _pipeline, uint32_t _firstSet)
{
	m_impl->commandBuffer.bindDescriptorSets(
		m_impl->lastBoundPipeline,
		_pipeline.GetImpl().pipelineLayout,
		_firstSet,
		*_dsSet.GetImpl().descriptorSet,
		nullptr
	);
}

template<>
void CommandBuffer::Bind<Pipeline>(Pipeline& _dsSet)
{

}

CommandBuffer::Impl& CommandBuffer::GetImpl() const
{
	return *m_impl;
}