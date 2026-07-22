#include "buffer_impl.h"
#include "device_impl.h"

#include <core/gpu/utils/converters.h>

#include <string>
#include <stdexcept>

using namespace core::gpu;


/// == These functions are the actual Vulkan Impl


Buffer::Impl::Impl(const Device* _device, const BufferCreateInfo& _info)
	: buffer(nullptr), device(_device), memory(nullptr), bufferSize(_info.size), mappedData(nullptr)
{
	if (_info.size == 0)
	{
		throw std::runtime_error("Buffer size cannot be zero");
	}

	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.flags = {};
	bufferInfo.size = static_cast<vk::DeviceSize>(_info.size);
	bufferInfo.usage = utils::ToVulkan(_info.usage);
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	buffer = vk::raii::Buffer(_device->GetImpl().device, bufferInfo);

	vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

	vk::MemoryAllocateFlagsInfo allocFlagsInfo{};
	bool needsDeviceAddress = (_info.usage & utils::EBufferUsage::ShaderDeviceAddress) != utils::EBufferUsage::None;

	if (needsDeviceAddress)
	{
		allocFlagsInfo.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;
	}

	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(
		memRequirements.memoryTypeBits,
		utils::ToVulkan(_info.memoryProperties)
	);

	if (needsDeviceAddress)
	{
		allocInfo.pNext = &allocFlagsInfo;
	}

	memory = vk::raii::DeviceMemory(_device->GetImpl().device, allocInfo);

	buffer.bindMemory(*memory, 0);
}

Buffer::Impl::~Impl() {}

uint32_t Buffer::Impl::FindMemoryType(uint32_t _typeFilter, vk::MemoryPropertyFlags _properties)
{
	vk::PhysicalDeviceMemoryProperties memProperties = device->GetImpl().physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		if ((_typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & _properties) == _properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}


/// === These functions are public side impl


Buffer::Buffer(const core::gpu::Device* _device, const BufferCreateInfo& _info)
{
	m_impl = std::make_unique<Impl>(_device, _info);
}

Buffer::~Buffer()
{
	Unmap();
};

void Buffer::Map(void** _data)
{
	if (m_impl->mappedData)
	{
		*_data = m_impl->mappedData;
		return;
	}

	m_impl->mappedData = m_impl->memory.mapMemory(0, m_impl->bufferSize);
	*_data = m_impl->mappedData;
}

void Buffer::Unmap()
{
	if (m_impl->mappedData)
	{
		m_impl->memory.unmapMemory();
		m_impl->mappedData = nullptr;
	}
}

void Buffer::CopyFrom(const void* _data, size_t _size, size_t _offset)
{
	if (_offset + _size > m_impl->bufferSize)
	{
		throw std::runtime_error("Copy operation exceeds buffer size");
	}

	void* mappedMem = nullptr;
	Map(&mappedMem);

	std::memcpy(static_cast<char*>(mappedMem) + _offset, _data, _size);
}

Buffer::Impl& Buffer::GetImpl() const
{
	return *m_impl;
}