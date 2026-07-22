#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_COMMAND_POOL_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_COMMAND_POOL_H
#pragma once

#include <core/gpu/commandPool.h>
#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	struct CommandPool::Impl
	{
		const Device*			device;
		vk::raii::CommandPool	pool;
		uint32_t				queueFamilyIndex;

		explicit Impl(const core::gpu::Device* _device, const CommandPoolCreateInfo& _info);
		~Impl();

		void Reset(bool _releaseResources);
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_COMMAND_POOL_H
