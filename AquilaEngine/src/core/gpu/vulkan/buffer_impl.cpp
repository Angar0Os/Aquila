#include "buffer_impl.h"
#include "device_impl.h"

#include <core/gpu/utils/converters.h>

#include <string>
#include <stdexcept>

using namespace core::gpu;


/// == These functions are the actual Vulkan Impl


Buffer::Impl::Impl(core::gpu::Buffer& p, const core::gpu::Device* device, const BufferCreateInfo& info)
	: parent(p), buffer(nullptr), device(device), memory(nullptr), bufferSize(info.size), mappedData(nullptr)
{
	if (info.size == 0)
	{
		throw std::runtime_error("Buffer size cannot be zero");
	}

	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.flags = {};
	bufferInfo.size = static_cast<vk::DeviceSize>(info.size);
	bufferInfo.usage = utils::ToVulkan(info.usage);
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	buffer = vk::raii::Buffer(device->GetImpl().device, bufferInfo);

	vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

	vk::MemoryAllocateFlagsInfo allocFlagsInfo{};
	bool needsDeviceAddress = (info.usage & utils::EBufferUsage::ShaderDeviceAddress) != utils::EBufferUsage::None;

	if (needsDeviceAddress)
	{
		allocFlagsInfo.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;
	}

	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(
		memRequirements.memoryTypeBits,
		utils::ToVulkan(info.memoryProperties)
	);

	if (needsDeviceAddress)
	{
		allocInfo.pNext = &allocFlagsInfo;
	}

	memory = vk::raii::DeviceMemory(device->GetImpl().device, allocInfo);

	buffer.bindMemory(*memory, 0);
}

Buffer::Impl::~Impl() {}

uint32_t Buffer::Impl::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
	vk::PhysicalDeviceMemoryProperties memProperties = device->GetImpl().physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}


/// === These functions are public side impl

Buffer::Buffer(const core::gpu::Device* device, const BufferCreateInfo& info)
{
	m_impl = std::make_unique<Impl>(*this, device, info);
}

Buffer::~Buffer()
{
	Unmap();
};

void Buffer::Map(void** data)
{
	if (m_impl->mappedData)
	{
		*data = m_impl->mappedData;
		return;
	}

	m_impl->mappedData = m_impl->memory.mapMemory(0, m_impl->bufferSize);
	*data = m_impl->mappedData;
}

void Buffer::Unmap()
{
	if (m_impl->mappedData)
	{
		m_impl->memory.unmapMemory();
		m_impl->mappedData = nullptr;
	}
}

void Buffer::CopyFrom(const void* data, size_t size, size_t offset)
{
	if (offset + size > m_impl->bufferSize)
	{
		throw std::runtime_error("Copy operation exceeds buffer size");
	}

	void* mappedMem = nullptr;
	Map(&mappedMem);

	std::memcpy(static_cast<char*>(mappedMem) + offset, data, size);
}

Buffer::Impl& Buffer::GetImpl() const
{
	return *m_impl;
}