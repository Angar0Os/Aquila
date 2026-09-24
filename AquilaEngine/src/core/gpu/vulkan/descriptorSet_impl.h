#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_DESCRIPTOR_SET_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_DESCRIPTOR_SET_H
#pragma once

#include <core/gpu/descriptorSet.h>
#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace core::gpu
{
	struct DescriptorSet::Impl
	{
		vk::raii::DescriptorSet descriptorSet = nullptr;

		std::vector<vk::DescriptorBufferInfo>						bufferInfos;
		std::vector<vk::DescriptorImageInfo>						imageInfos;
		std::vector<vk::AccelerationStructureKHR>					asHandles;
		std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> asInfos;
		std::vector<vk::WriteDescriptorSet>							writes;

		struct BindingInfo
		{
			uint32_t			binding;
			vk::DescriptorType	type;
			size_t				infoIndex;
			uint32_t			arrayElement = 0;
		};

		std::vector<BindingInfo> bindingInfos;
	};

}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_DESCRIPTOR_SET_H
