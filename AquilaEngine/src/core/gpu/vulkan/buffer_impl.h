#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_BUFFER_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_BUFFER_H
#pragma once

#include <core/gpu/buffer.h>

#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	struct Buffer::Impl
	{
		Buffer& parent;
		const core::gpu::Device* device;

		vk::raii::Buffer buffer;
		vk::raii::DeviceMemory memory;


		size_t bufferSize;
		void* mappedData;

		uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

		explicit Impl(Buffer& p, const Device* device,
			const BufferCreateInfo& info);
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_BUFFER_H
