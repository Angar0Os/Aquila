#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_DESCRIPTOR_SET_LAYOUT_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_DESCRIPTOR_SET_LAYOUT_H
#pragma once

#include <core/gpu/descriptorSetLayout.h>
#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	class Device;

	struct DescriptorSetLayout::Impl
	{
		vk::raii::DescriptorSetLayout layout;

		explicit Impl(const Device* _device, const DescriptorSetLayoutCreateInfo& _info);
		~Impl();
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_DESCRIPTOR_SET_LAYOUT_H
