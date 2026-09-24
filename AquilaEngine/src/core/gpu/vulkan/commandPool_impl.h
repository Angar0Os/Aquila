#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_COMMAND_POOL_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_COMMAND_POOL_H
#pragma once

#include <core/gpu/commandPool.h>
#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	struct CommandPool::Impl
	{
		vk::raii::CommandPool	pool				= nullptr;
		uint32_t				queueFamilyIndex	= ~0;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_COMMAND_POOL_H
