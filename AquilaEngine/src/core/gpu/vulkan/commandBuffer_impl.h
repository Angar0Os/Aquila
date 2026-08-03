#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_COMMAND_BUFFER_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_COMMAND_BUFFER_H
#pragma once

#include <core/gpu/commandBuffer.h>
#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	struct CommandBuffer::Impl
	{
		vk::raii::CommandBuffer commandBuffer	= nullptr;
		vk::raii::Semaphore		semaphore		= nullptr;
		vk::raii::Fence			isGpuFree		= nullptr;
		bool					isCpuFree		= true;
		bool					isSingleTime	= false;
	};

}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_COMMAND_BUFFER_H
