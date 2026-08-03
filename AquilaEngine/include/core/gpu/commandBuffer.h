#ifndef AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
#define AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
#pragma once

#include <memory>
#include <vector>
#include <functional>

#include <core/gpu/utils/enums.h>

namespace core::gpu
{
	class Buffer;
	class DescriptorSet;
	class Device;
	class Image;
	class Pipeline; 

	class CommandBuffer
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	public:
		CommandBuffer(const Device& _device);
		~CommandBuffer();

		bool IsCpuFree() const;
		bool IsGpuFree() const;

		void WaitForCompletion();

		void Record(std::function<void()> content);

		Impl& GetImpl() const;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
