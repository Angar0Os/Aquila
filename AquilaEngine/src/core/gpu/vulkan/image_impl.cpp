#include "image_impl.h"
#include "device_impl.h"

#include <core/gpu/utils/converters.h>

#include <stdexcept>

using namespace core::gpu;

Image::Impl::Impl(const Device& _device, const ImageCreateInfo& _info)
	: format(_info.format), samples(_info.samples)
{
	if (_info.width == 0 || _info.height == 0)
	{
		throw std::runtime_error("Image width and height cannot be zero");
	}

	vk::ImageCreateInfo imageInfo{};
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.format = utils::ToVulkan(_info.format);
	imageInfo.extent.width = _info.width;
	imageInfo.extent.height = _info.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = _info.mipLevels;
	imageInfo.arrayLayers = _info.arrayLayers;
	imageInfo.samples = utils::ToVulkan(_info.samples);
	imageInfo.tiling = utils::ToVulkan(_info.tiling);
	imageInfo.usage = utils::ToVulkan(_info.usage);
	imageInfo.sharingMode = vk::SharingMode::eExclusive;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;

	raiiImage = vk::raii::Image(_device.GetImpl().device, imageInfo);

	vk::MemoryRequirements memRequirements = raiiImage.getMemoryRequirements();

	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(
		_device,
		memRequirements.memoryTypeBits,
		utils::ToVulkan(_info.memoryProperties)
	);

	memory = vk::raii::DeviceMemory(_device.GetImpl().device, allocInfo);

	raiiImage.bindMemory(*memory, 0);
	image = *raiiImage;

	vk::Extent2D tempExtent = { _info.width, _info.height };
	extent = tempExtent;

	vk::ImageViewCreateInfo viewInfo{};
	viewInfo.image = image;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = utils::ToVulkan(_info.format);
	viewInfo.subresourceRange.aspectMask = utils::ToVulkanAspestMask(_info.format);
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	view = vk::raii::ImageView(_device.GetImpl().device, viewInfo);
}

Image::Impl::Impl(const Device& _device, const PredefinedImageCreateInfo& _info)
{
	image = _info.image;
	extent = _info.extent;

	vk::ImageViewCreateInfo viewInfo{};
	viewInfo.image = _info.image;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = _info.format;
	viewInfo.subresourceRange.aspectMask = _info.aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	view = vk::raii::ImageView(_device.GetImpl().device, viewInfo);
}

Image::Impl::~Impl() = default;

uint32_t Image::Impl::FindMemoryType(const Device& _device, uint32_t _typeFilter, vk::MemoryPropertyFlags _properties)
{
	vk::PhysicalDeviceMemoryProperties memProperties = _device.GetImpl().physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		if ((_typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & _properties) == _properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type for image");
}

Image::Image(const Device& _device, const ImageCreateInfo& _info)
{
	m_impl = std::make_unique<Impl>(_device, _info);
}

Image::Image(const Device& _device, const PredefinedImageCreateInfo& _info)
{
	m_impl = std::make_unique<Impl>(_device, _info);
}

Image::~Image() = default;

Image::Impl& Image::GetImpl() const
{
	return *m_impl;
}