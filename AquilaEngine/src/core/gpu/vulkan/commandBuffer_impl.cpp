#include "commandBuffer_impl.h"
#include "accelerationStructure_impl.h"
#include "buffer_impl.h"
#include "commandPool_impl.h"
#include "descriptorSet_impl.h"
#include "device_impl.h"
#include "image_impl.h"
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

void CommandBuffer::Record(std::function<void()> content) const
{
	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.flags = m_impl->isSingleTime ? vk::CommandBufferUsageFlagBits::eOneTimeSubmit : vk::CommandBufferUsageFlags(0);
	m_impl->commandBuffer.begin(beginInfo);

	content();

	m_impl->commandBuffer.end();
}

void CommandBuffer::Submit(const Device& _device, bool _isImmediate) const
{
	if (_device.currentFrame >= _device.GetImpl().frameSyncObjects.size())
	{
		throw std::runtime_error("Frame index is out of range.");
	}

	vk::CommandBuffer tempCmdBuf = *m_impl->commandBuffer;
	auto& frameSync = _device.GetImpl().frameSyncObjects[_device.currentFrame];

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
		auto waitResult = _device.GetImpl().device.waitForFences(*frameSync.inFlightFence, VK_TRUE, UINT64_MAX);
		_device.GetImpl().device.resetFences(*frameSync.inFlightFence);

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
	vk::DeviceSize vkOffset = 0;
	// TODO : We will need to fix this after evoke.

	auto usage = _buffer.GetImpl().usage;

	if ((usage & utils::EBufferUsage::IndexBuffer) == utils::EBufferUsage::IndexBuffer)
	{
		m_impl->commandBuffer.bindIndexBuffer(_buffer.GetImpl().buffer, vkOffset, vk::IndexType::eUint32);
	}
	else if ((usage & utils::EBufferUsage::VertexBuffer) == utils::EBufferUsage::VertexBuffer)
	{
		m_impl->commandBuffer.bindVertexBuffers(0, *_buffer.GetImpl().buffer, vkOffset);
	}
	else
	{
		throw std::runtime_error("You must bind a correct type of buffer.");
	}
}

template<>
void CommandBuffer::Bind<Pipeline>(const DescriptorSet& _dsSet, Pipeline& _pipeline, uint32_t _firstSet) const
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
void CommandBuffer::Bind<Pipeline>(Pipeline& _pipeline)
{
	if (_pipeline.GetImpl().type == utils::EPipelineType::Compute)
	{
		m_impl->commandBuffer.bindPipeline(
			vk::PipelineBindPoint::eCompute,
			vk::Pipeline(_pipeline.GetImpl().pipeline)
		);

		m_impl->lastBoundPipeline = vk::PipelineBindPoint::eCompute;
	}
	else if (_pipeline.GetImpl().type == utils::EPipelineType::Graphics)
	{

		m_impl->commandBuffer.bindPipeline(
			vk::PipelineBindPoint::eGraphics,
			vk::Pipeline(_pipeline.GetImpl().pipeline)
		);

		m_impl->lastBoundPipeline = vk::PipelineBindPoint::eGraphics;
	}
	else
	{
		m_impl->commandBuffer.bindPipeline(
			vk::PipelineBindPoint::eRayTracingKHR,
			vk::Pipeline(_pipeline.GetImpl().pipeline)
		);
		m_impl->lastBoundPipeline = vk::PipelineBindPoint::eRayTracingKHR;
	}
}

void CommandBuffer::SetViewport(float _x, float _y, float _width, float _height, float _minDepth, float _maxDepth)
{
	vk::Viewport viewport(
		_x,
		_y,
		_width,
		_height,
		_minDepth,
		_maxDepth
	);
		
	m_impl->commandBuffer.setViewport(0, viewport);
}

void CommandBuffer::SetScissor(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height)
{
	vk::Rect2D scissor(
		{ _x, _y }, 
		{ _width, _height }
	);

	m_impl->commandBuffer.setScissor(0, scissor);
}

void CommandBuffer::Dispatch(uint32_t _x, uint32_t _y, uint32_t _z)
{
	m_impl->commandBuffer.dispatch(_x, _y, _z);
}

void CommandBuffer::BeginRendering(const Device& _device, const std::vector<RenderingAttachmentInfo>& _colorAttachments, const DepthAttachmentInfo& _depthAttachment)
{
	std::vector<vk::RenderingAttachmentInfo> vkColorAttachments;
	vkColorAttachments.reserve(_colorAttachments.size());

	for (const auto& ca : _colorAttachments)
	{
		vk::RenderingAttachmentInfo info{};
		info.imageView = ca.image->GetImpl().view;
		info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		info.loadOp = ca.clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
		info.storeOp = vk::AttachmentStoreOp::eStore;
		info.clearValue = vk::ClearColorValue(ca.clearR, ca.clearG, ca.clearB, ca.clearA);
		vkColorAttachments.push_back(info);
	}

	vk::RenderingAttachmentInfo vkDepth{};
	if (_depthAttachment.image)
	{
		vkDepth.imageView = _depthAttachment.image->GetImpl().view;
		vkDepth.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
		vkDepth.loadOp = _depthAttachment.clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
		vkDepth.storeOp = vk::AttachmentStoreOp::eStore;
		vkDepth.clearValue = vk::ClearDepthStencilValue(_depthAttachment.clearDepth, 0);
	}

	uint32_t width = _colorAttachments.empty() ? _device.GetImpl().swapchainExtent.width
		: _colorAttachments[0].image->GetImpl().extent.width;
	uint32_t height = _colorAttachments.empty() ? _device.GetImpl().swapchainExtent.height
		: _colorAttachments[0].image->GetImpl().extent.height;

	vk::RenderingInfo info{};
	info.renderArea = vk::Rect2D({ 0, 0 }, { width, height });
	info.layerCount = 1;
	info.colorAttachmentCount = static_cast<uint32_t>(vkColorAttachments.size());
	info.pColorAttachments = vkColorAttachments.data();
	info.pDepthAttachment = _depthAttachment.image ? &vkDepth : nullptr;

	m_impl->commandBuffer.beginRendering(info);
}

void CommandBuffer::EndRendering()
{
	m_impl->commandBuffer.endRendering();
}

void CommandBuffer::TransitionImageLayout(const Image& _image, utils::EImageLayout _oldLayout, utils::EImageLayout _newLayout, bool _isDepth)
{
	vk::AccessFlags srcAccess, dstAccess;
	vk::PipelineStageFlags srcStage, dstStage;
	vk::ImageLayout vkOldLayout, vkNewLayout;

	if (_isDepth && _oldLayout == utils::EImageLayout::Undefined)
	{
		srcAccess = {};
		dstAccess = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		vkOldLayout = vk::ImageLayout::eUndefined;
		vkNewLayout = vk::ImageLayout::eDepthAttachmentOptimal;

		vk::ImageMemoryBarrier barrier{};
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;
		barrier.oldLayout = vkOldLayout;
		barrier.newLayout = vkNewLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = _image.GetImpl().image;

		vk::ImageSubresourceRange subRange{};
		subRange.aspectMask = _isDepth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
		subRange.baseMipLevel = 0;
		subRange.levelCount = 1;
		subRange.baseArrayLayer = 0;
		subRange.layerCount = 1;

		barrier.subresourceRange = subRange;

		m_impl->commandBuffer.pipelineBarrier(srcStage, dstStage, {}, {}, {}, { barrier });
		return;
	}

	switch (_oldLayout)
	{
		case utils::EImageLayout::Undefined:		vkOldLayout = vk::ImageLayout::eUndefined;				break;
		case utils::EImageLayout::ColorAttachment:	vkOldLayout = vk::ImageLayout::eColorAttachmentOptimal; break;
		case utils::EImageLayout::TransferSrc:		vkOldLayout = vk::ImageLayout::eTransferSrcOptimal;		break;
		case utils::EImageLayout::TransferDst:		vkOldLayout = vk::ImageLayout::eTransferDstOptimal;		break;
		case utils::EImageLayout::Present:			vkOldLayout = vk::ImageLayout::ePresentSrcKHR;			break;
		case utils::EImageLayout::ShaderReadOnly:	vkOldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;	break;
		case utils::EImageLayout::General:			vkOldLayout = vk::ImageLayout::eGeneral;				break;
		default:									vkOldLayout = vk::ImageLayout::eUndefined;
	}

	switch (_newLayout)
	{
		case utils::EImageLayout::Undefined:		vkNewLayout = vk::ImageLayout::eUndefined;				break;
		case utils::EImageLayout::ColorAttachment:	vkNewLayout = vk::ImageLayout::eColorAttachmentOptimal; break;
		case utils::EImageLayout::TransferSrc:		vkNewLayout = vk::ImageLayout::eTransferSrcOptimal;		break;
		case utils::EImageLayout::TransferDst:		vkNewLayout = vk::ImageLayout::eTransferDstOptimal;		break;
		case utils::EImageLayout::Present:			vkNewLayout = vk::ImageLayout::ePresentSrcKHR;			break;
		case utils::EImageLayout::ShaderReadOnly:	vkNewLayout = vk::ImageLayout::eShaderReadOnlyOptimal;	break;
		case utils::EImageLayout::General:			vkNewLayout = vk::ImageLayout::eGeneral;				break;
		default:									vkNewLayout = vk::ImageLayout::eUndefined;
	}

	if (_oldLayout == utils::EImageLayout::Undefined && _newLayout == utils::EImageLayout::TransferDst)
	{
		srcAccess = {};
		dstAccess = vk::AccessFlagBits::eTransferWrite;
		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (_oldLayout == utils::EImageLayout::ColorAttachment && _newLayout == utils::EImageLayout::TransferSrc)
	{
		srcAccess = vk::AccessFlagBits::eColorAttachmentWrite;
		dstAccess = vk::AccessFlagBits::eTransferRead;
		srcStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (_oldLayout == utils::EImageLayout::TransferDst && _newLayout == utils::EImageLayout::Present)
	{
		srcAccess = vk::AccessFlagBits::eTransferWrite;
		dstAccess = vk::AccessFlagBits::eMemoryRead;
		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eBottomOfPipe;
	}
	else if (_oldLayout == utils::EImageLayout::Undefined && _newLayout == utils::EImageLayout::ColorAttachment)
	{
		srcAccess = {};
		dstAccess = vk::AccessFlagBits::eColorAttachmentWrite;
		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	}
	else if (_oldLayout == utils::EImageLayout::ColorAttachment && _newLayout == utils::EImageLayout::ShaderReadOnly)
	{
		srcAccess = vk::AccessFlagBits::eColorAttachmentWrite;
		dstAccess = vk::AccessFlagBits::eShaderRead;
		srcStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (_oldLayout == utils::EImageLayout::ShaderReadOnly && _newLayout == utils::EImageLayout::ColorAttachment)
	{
		srcAccess = vk::AccessFlagBits::eShaderRead;
		dstAccess = vk::AccessFlagBits::eColorAttachmentWrite;
		srcStage = vk::PipelineStageFlagBits::eFragmentShader;
		dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	}
	else if (_oldLayout == utils::EImageLayout::Undefined && _newLayout == utils::EImageLayout::ShaderReadOnly)
	{
		srcAccess = {};
		dstAccess = vk::AccessFlagBits::eShaderRead;
		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (_oldLayout == utils::EImageLayout::ShaderReadOnly && _newLayout == utils::EImageLayout::TransferSrc)
	{
		srcAccess = vk::AccessFlagBits::eShaderRead;
		dstAccess = vk::AccessFlagBits::eTransferRead;
		srcStage = vk::PipelineStageFlagBits::eFragmentShader;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (_oldLayout == utils::EImageLayout::DepthStencilAttachment && _newLayout == utils::EImageLayout::ShaderReadOnly)
	{
		srcAccess = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		dstAccess = vk::AccessFlagBits::eShaderRead;
		srcStage = vk::PipelineStageFlagBits::eLateFragmentTests;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (_oldLayout == utils::EImageLayout::TransferDst && _newLayout == utils::EImageLayout::ShaderReadOnly)
	{
		srcAccess = vk::AccessFlagBits::eTransferWrite;
		dstAccess = vk::AccessFlagBits::eShaderRead;
		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (_oldLayout == utils::EImageLayout::TransferDst && _newLayout == utils::EImageLayout::ColorAttachment)
	{
		srcAccess = vk::AccessFlagBits::eTransferWrite;
		dstAccess = vk::AccessFlagBits::eColorAttachmentWrite;
		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	}
	else if (_oldLayout == utils::EImageLayout::ColorAttachment && _newLayout == utils::EImageLayout::Present)
	{
		srcAccess = vk::AccessFlagBits::eColorAttachmentWrite;
		dstAccess = vk::AccessFlagBits::eMemoryRead;
		srcStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dstStage = vk::PipelineStageFlagBits::eBottomOfPipe;
	}
	else if (_oldLayout == utils::EImageLayout::Present && _newLayout == utils::EImageLayout::TransferDst)
	{
		srcAccess = vk::AccessFlagBits::eMemoryRead;
		dstAccess = vk::AccessFlagBits::eTransferWrite;
		srcStage = vk::PipelineStageFlagBits::eBottomOfPipe;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (_oldLayout == utils::EImageLayout::Present && _newLayout == utils::EImageLayout::ColorAttachment)
	{
		srcAccess = vk::AccessFlagBits::eMemoryRead;
		dstAccess = vk::AccessFlagBits::eColorAttachmentWrite;
		srcStage = vk::PipelineStageFlagBits::eBottomOfPipe;
		dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	}
	else if (_oldLayout == utils::EImageLayout::Undefined && _newLayout == utils::EImageLayout::General)
	{
		srcAccess = {};
		dstAccess = vk::AccessFlagBits::eShaderWrite;
		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eComputeShader;
	}
	else if (_oldLayout == utils::EImageLayout::General && _newLayout == utils::EImageLayout::ShaderReadOnly)
	{
		srcAccess = vk::AccessFlagBits::eShaderWrite;
		dstAccess = vk::AccessFlagBits::eShaderRead;
		srcStage = vk::PipelineStageFlagBits::eComputeShader;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (_oldLayout == utils::EImageLayout::ShaderReadOnly && _newLayout == utils::EImageLayout::TransferDst)
	{
		srcAccess = vk::AccessFlagBits::eShaderRead;
		dstAccess = vk::AccessFlagBits::eTransferWrite;
		srcStage = vk::PipelineStageFlagBits::eFragmentShader;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (_oldLayout == utils::EImageLayout::ColorAttachment && _newLayout == utils::EImageLayout::TransferDst)
	{
		srcAccess = vk::AccessFlagBits::eColorAttachmentWrite;
		dstAccess = vk::AccessFlagBits::eTransferWrite;
		srcStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (_oldLayout == utils::EImageLayout::General && _newLayout == utils::EImageLayout::TransferSrc)
	{
		srcAccess = vk::AccessFlagBits::eShaderWrite;
		dstAccess = vk::AccessFlagBits::eTransferRead;
		srcStage = vk::PipelineStageFlagBits::eComputeShader;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (_oldLayout == utils::EImageLayout::General && _newLayout == utils::EImageLayout::TransferDst)
	{
		srcAccess = vk::AccessFlagBits::eShaderWrite;
		dstAccess = vk::AccessFlagBits::eTransferWrite;
		srcStage = vk::PipelineStageFlagBits::eComputeShader;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (_oldLayout == utils::EImageLayout::TransferSrc && _newLayout == utils::EImageLayout::ShaderReadOnly)
	{
		srcAccess = vk::AccessFlagBits::eTransferRead;
		dstAccess = vk::AccessFlagBits::eShaderRead;
		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else
	{
		throw std::runtime_error("Unsupported layout transition!");
	}

	vk::ImageMemoryBarrier barrier{};
	barrier.srcAccessMask = srcAccess;
	barrier.dstAccessMask = dstAccess;
	barrier.oldLayout = vkOldLayout;
	barrier.newLayout = vkNewLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = _image.GetImpl().image;

	vk::ImageSubresourceRange subRange{};
	subRange.aspectMask = _isDepth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
	subRange.baseMipLevel = 0;
	subRange.levelCount = 1;
	subRange.baseArrayLayer = 0;
	subRange.layerCount = 1;

	barrier.subresourceRange = subRange;

	m_impl->commandBuffer.pipelineBarrier(srcStage, dstStage, {}, {}, {}, { barrier });
}

void CommandBuffer::BlitImage(const Image& _srcImage, const Image& _dstImage)
{
	auto srcWidth = _srcImage.GetImpl().extent.width;
	auto srcHeight = _srcImage.GetImpl().extent.height;

	auto dstWidth = _dstImage.GetImpl().extent.width;
	auto dstHeight = _dstImage.GetImpl().extent.height;

	vk::ImageSubresourceLayers subRes{};
	subRes.aspectMask = vk::ImageAspectFlagBits::eColor;
	subRes.mipLevel = 0;
	subRes.baseArrayLayer = 0;
	subRes.layerCount = 1;

	vk::ImageBlit region{};
	region.srcSubresource = subRes;
	region.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
	region.srcOffsets[1] = vk::Offset3D{ static_cast<int32_t>(srcWidth), static_cast<int32_t>(srcHeight), 1 };
	region.dstSubresource = subRes;
	region.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
	region.dstOffsets[1] = vk::Offset3D{ static_cast<int32_t>(dstWidth), static_cast<int32_t>(dstHeight), 1 };

	m_impl->commandBuffer.blitImage(
		_srcImage.GetImpl().image, vk::ImageLayout::eTransferSrcOptimal,
		_dstImage.GetImpl().image, vk::ImageLayout::eTransferDstOptimal,
		region,
		vk::Filter::eLinear
	);
}

void CommandBuffer::CopyBuffer(const Buffer& _srcBuffer, const Buffer& _dstBuffer, uint32_t _size) const
{
	vk::BufferCopy copyRegion;
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = _size;

	m_impl->commandBuffer.copyBuffer(_srcBuffer.GetImpl().buffer, _dstBuffer.GetImpl().buffer, copyRegion);
}

void CommandBuffer::CopyBufferToImage(const Buffer& _srcBuffer, const Image& _dstImage, uint32_t _width, uint32_t _height)
{
	vk::BufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = vk::Offset3D{ 0, 0, 0 };
	region.imageExtent = vk::Extent3D{ _width, _height, 1 };

	m_impl->commandBuffer.copyBufferToImage(
		_srcBuffer.GetImpl().buffer,
		_dstImage.GetImpl().image,
		vk::ImageLayout::eTransferDstOptimal,
		region
	);
}

void CommandBuffer::PushConstants(const Pipeline& _pipeline, uint32_t _stageFlags, uint32_t _offset, uint32_t _size, const void* _pValues)
{
	vk::ShaderStageFlags vkStageFlags;

	if (_stageFlags & static_cast<uint32_t>(utils::EShaderStageFlags::Vertex))
		vkStageFlags |= vk::ShaderStageFlagBits::eVertex;

	if (_stageFlags & static_cast<uint32_t>(utils::EShaderStageFlags::Fragment))
		vkStageFlags |= vk::ShaderStageFlagBits::eFragment;

	if (_stageFlags & static_cast<uint32_t>(utils::EShaderStageFlags::Compute))
		vkStageFlags |= vk::ShaderStageFlagBits::eCompute;

	m_impl->commandBuffer.pushConstants<uint8_t>(
		_pipeline.GetImpl().pipelineLayout,
		vkStageFlags,
		_offset,
		vk::ArrayProxy<const uint8_t>(_size, static_cast<const uint8_t*>(_pValues))
	);
}

void CommandBuffer::DrawIndexed(uint32_t _indexCount, uint32_t _instanceCount, uint32_t _firstIndex, uint32_t _vertexOffset, uint32_t _firstInstance) const
{
	m_impl->commandBuffer.drawIndexed(_indexCount, _instanceCount, _firstIndex, _vertexOffset, _firstInstance);
}

void CommandBuffer::AccelerationStructureBarrier()
{
	auto barrier = vk::MemoryBarrier2{};
	barrier.srcStageMask = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR;
	barrier.srcAccessMask = vk::AccessFlagBits2::eAccelerationStructureWriteKHR;
	barrier.dstStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR | vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eFragmentShader;
	barrier.dstAccessMask = vk::AccessFlagBits2::eAccelerationStructureReadKHR;;

	auto dep = vk::DependencyInfo{};
	dep.memoryBarrierCount = 1;
	dep.pMemoryBarriers = &barrier;

	m_impl->commandBuffer.pipelineBarrier2(dep);
}

CommandBuffer::Impl& CommandBuffer::GetImpl() const
{
	return *m_impl;
}