#ifndef AQUILA_ENGINE_CORE_GPU_UTILS_CONVERTERS_H
#define AQUILA_ENGINE_CORE_GPU_UTILS_CONVERTERS_H
#pragma once

#include <core/gpu/utils/enums.h>
#include <vulkan/vulkan_raii.hpp>

namespace core::gpu::utils
{
	vk::PresentModeKHR          ToVulkan(PresentMode mode);
	vk::Format                  ToVulkan(TextureFormat format);
	vk::ImageAspectFlags        ToVulkanAspestMask(TextureFormat format);
}

#endif //AQUILA_ENGINE_CORE_GPU_UTILS_CONVERTERS_H
