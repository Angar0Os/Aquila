#ifndef AQUILA_ENGINE_CORE_GPU_UTILS_CONVERTERS_H
#define AQUILA_ENGINE_CORE_GPU_UTILS_CONVERTERS_H
#pragma once

#include <core/gpu/utils/enums.h>
#include <vulkan/vulkan_raii.hpp>

namespace core::gpu::utils
{
	vk::PresentModeKHR          ToVulkan(EPresentMode mode);
	vk::Format                  ToVulkan(ETextureFormat format);
	vk::BufferUsageFlags        ToVulkan(EBufferUsage usage);
	vk::MemoryPropertyFlags     ToVulkan(EMemoryProperty properties);

	vk::ImageAspectFlags        ToVulkanAspestMask(ETextureFormat format);
}

#endif //AQUILA_ENGINE_CORE_GPU_UTILS_CONVERTERS_H
