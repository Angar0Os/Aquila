#ifndef AQUILA_ENGINE_CORE_GPU_UTILS_ENUMS_H
#define AQUILA_ENGINE_CORE_GPU_UTILS_ENUMS_H
#pragma once

namespace core::gpu::utils
{
	enum class ETextureFormat

	{
		Undefined,
		R8_UNorm,
		RG8_UNorm,
		RGB8_UNorm,
		RGBA8_UNorm,
		RGBA8_SRGB,
		R16_Float,
		RG16_Float,
		RGBA16_Float,
		R32_Float,
		RG32_Float,
		RGB32_Float,
		RGBA32_Float,
		Depth16,
		Depth24,
		Depth32F,
		Depth24Stencil8,
		Depth32FStencil8,
		BC1_RGB_UNorm,
		BC3_RGBA_UNorm,
		BC7_RGBA_UNorm,
	};

	enum class EPresentMode
	{
		Immediate,
		Mailbox,
		Fifo,
		FifoRelaxed
	};

	enum class EBufferUsage : uint32_t
	{
		None = 0,
		TransferSrc = 1 << 0,
		TransferDst = 1 << 1,
		UniformBuffer = 1 << 2,
		StorageBuffer = 1 << 3,
		IndexBuffer = 1 << 4,
		VertexBuffer = 1 << 5,
		IndirectBuffer = 1 << 6,

		ShaderDeviceAddress = 1 << 17,
		AccelerationStructureStorage = 1 << 20,
		AccelerationStructureBuildInput = 1 << 19
	};

	inline EBufferUsage operator|(EBufferUsage a, EBufferUsage b)
	{
		return static_cast<EBufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline EBufferUsage operator&(EBufferUsage a, EBufferUsage b)
	{
		return static_cast<EBufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	enum class EMemoryProperty : uint32_t
	{
		None = 0,
		DeviceLocal = 1 << 0,
		HostVisible = 1 << 1,
		HostCoherent = 1 << 2,
		HostCached = 1 << 3
	};

	inline EMemoryProperty operator|(EMemoryProperty a, EMemoryProperty b)
	{
		return static_cast<EMemoryProperty>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline EMemoryProperty operator&(EMemoryProperty a, EMemoryProperty b)
	{
		return static_cast<EMemoryProperty>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}
}

#endif //AQUILA_ENGINE_CORE_GPU_UTILS_ENUMS_H
