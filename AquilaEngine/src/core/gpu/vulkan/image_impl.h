#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_IMAGE_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_IMAGE_H
#pragma once

#include <core/gpu/image.h>

#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	struct PredefinedImageCreateInfo
	{
		vk::Image image						= nullptr;
		vk::Extent2D extent					= { 0, 0 };
		vk::ImageAspectFlags aspectFlags	= {};
		vk::Format format					= vk::Format::eUndefined;
	};

	struct Image::Impl
	{
		vk::Image				image			= nullptr;
		vk::raii::Image			raiiImage		= nullptr;
		vk::raii::DeviceMemory	memory			= nullptr;
		vk::raii::ImageView		view			= nullptr;
		vk::ImageLayout			currentLayout	= vk::ImageLayout::eUndefined;
		vk::Extent2D			extent			= { 0, 0 };

		utils::ETextureFormat format;
		utils::ESampleCount samples;

		uint32_t FindMemoryType(const Device& _device, uint32_t _typeFilter, vk::MemoryPropertyFlags _properties);

		explicit Impl(const Device* _device, const ImageCreateInfo& _info);
		explicit Impl(const Device* _device, const PredefinedImageCreateInfo& _info);

		~Impl() noexcept;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_IMAGE_H
