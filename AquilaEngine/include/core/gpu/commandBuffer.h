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

		void Record(std::function<void()> _content);

		void Submit(const Device& _device, uint32_t _frameIndex, bool isImmediate = false);

		template<typename T>
		void Bind(T& _input);
		
		template<typename T>
		void Bind(const DescriptorSet& _dsSet, T& _input, uint32_t _firstSet);

		Impl& GetImpl() const;
}

#endif //AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
