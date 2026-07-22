#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_DESCRIPTOR_POOL_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_DESCRIPTOR_POOL_H
#pragma once

#include <core/gpu/descriptorPool.h>
#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	struct DescriptorPool::Impl
	{
		const Device* device;
		vk::raii::DescriptorPool pool;

		explicit Impl(DescriptorPool& _pool, const core::gpu::Device* _device);
		~Impl();
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_DESCRIPTOR_POOL_H
