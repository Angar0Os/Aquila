#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_BUFFER_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_BUFFER_H
#pragma once

#include <core/gpu/buffer.h>

#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	struct Buffer::Impl
	{
		const Device& device;

		vk::raii::Buffer buffer;
		vk::raii::DeviceMemory memory;
		utils::EBufferUsage usage;


		size_t bufferSize;
		void* mappedData;

		uint32_t FindMemoryType(uint32_t _typeFilter, vk::MemoryPropertyFlags _properties);

		explicit Impl(const Device& _device, const BufferCreateInfo& _info);
		~Impl() noexcept;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_BUFFER_H
