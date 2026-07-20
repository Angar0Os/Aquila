#ifndef AQUILA_ENGINE_CORE_GPU_UTILS_ENUMS_H
#define AQUILA_ENGINE_CORE_GPU_UTILS_ENUMS_H
#pragma once

namespace core::gpu::utils
{
	enum class TextureFormat
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

	enum class PresentMode
	{
		Immediate,
		Mailbox,
		Fifo,
		FifoRelaxed
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_UTILS_ENUMS_H
