#ifndef AQUILA_ENGINE_CORE_GPU_BUFFER_H
#define AQUILA_ENGINE_CORE_GPU_BUFFER_H
#pragma once

#include <core/gpu/utils/enums.h>
#include <memory>

namespace core::gpu
{
	class Device;

	struct BufferCreateInfo
	{
		size_t size = 0;
		utils::EBufferUsage usage = utils::EBufferUsage::None;
		utils::EMemoryProperty memoryProperties = utils::EMemoryProperty::None;
	};

	class Buffer
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	public:
		explicit Buffer(const Device* _device, const BufferCreateInfo& _info);
		~Buffer() noexcept;

		void Map(void** _data);
		void Unmap();

		void CopyFrom(const void* _data, size_t _size, size_t _offset = 0);

		Impl& GetImpl() const;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_BUFFER_H
