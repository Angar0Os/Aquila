#ifndef AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
#define AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
#pragma once

#include <memory>
#include <vector>

#include <core/gpu/utils/enums.h>

namespace core::gpu
{
	class Buffer;
	class DescriptorSet;
	class Device;
	class Image;
	class Pipeline; 

	struct CommandBufferCreateInfo
	{
		const Device* device;
		utils::ECommandBufferLevel level = utils::ECommandBufferLevel::Primary;
		uint32_t count = 1;
		bool singleTime = false;
	};

	class CommandBuffer
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	public:
		CommandBuffer(const core::gpu::Device* _device, const CommandBufferCreateInfo& _info);
		~CommandBuffer();


	};
}

#endif //AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
