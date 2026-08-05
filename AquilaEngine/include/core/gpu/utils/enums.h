#ifndef AQUILA_ENGINE_CORE_GPU_UTILS_ENUMS_H
#define AQUILA_ENGINE_CORE_GPU_UTILS_ENUMS_H
#pragma once

#include <cstdint>

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

	enum class EDescriptorType
	{
		UniformBuffer,
		CombinedImageSampler,
		StorageBuffer,
		StorageImage,
		AccelerationStructure
	};

	enum class EShaderStageFlags : uint32_t
	{
		None = 0,
		Vertex = 1 << 0,
		Fragment = 1 << 1,
		Compute = 1 << 2,
		Geometry = 1 << 3,
		TessellationControl = 1 << 4,
		TessellationEvaluation = 1 << 5,

		RayGen = 1 << 6,
		ClosestHit = 1 << 7,
		AnyHit = 1 << 8,
		Miss = 1 << 9,
		Intersection = 1 << 10,
		Callable = 1 << 11,

		AllGraphics = Vertex | Fragment | Geometry | TessellationControl | TessellationEvaluation,
		AllRayTracing = RayGen | ClosestHit | AnyHit | Miss | Intersection | Callable
	};

	inline EShaderStageFlags operator|(EShaderStageFlags a, EShaderStageFlags b)
	{
		return static_cast<EShaderStageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline EShaderStageFlags operator&(EShaderStageFlags a, EShaderStageFlags b)
	{
		return static_cast<EShaderStageFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	enum class EShaderStage : uint32_t
	{
		None = 0,
		Vertex = 1 << 0,
		Fragment = 1 << 1,
		Compute = 1 << 2,
		Geometry = 1 << 3,
		TessellationControl = 1 << 4,
		TessellationEvaluation = 1 << 5,

		RayGen = 1 << 6,
		ClosestHit = 1 << 7,
		AnyHit = 1 << 8,
		Miss = 1 << 9,
		Intersection = 1 << 10,
		Callable = 1 << 11,

		All = 0x7FFFFFFF
	};

	inline EShaderStage operator|(EShaderStage a, EShaderStage b)
	{
		return static_cast<EShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline EShaderStage operator&(EShaderStage a, EShaderStage b)
	{
		return static_cast<EShaderStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	enum class EImageTiling
	{
		Optimal,
		Linear
	};

	enum class EImageLayout
	{
		Undefined,
		General,
		ShaderReadOnly,
		ColorAttachment,
		DepthStencilAttachment,
		TransferSrc,
		TransferDst,
		Present
	};

	enum class EImageUsage : uint32_t
	{
		None = 0,
		TransferSrc = 1 << 0,
		TransferDst = 1 << 1,
		Sampled = 1 << 2,
		Storage = 1 << 3,
		ColorAttachment = 1 << 4,
		DepthStencilAttachment = 1 << 5,
		TransientAttachment = 1 << 6,
		InputAttachment = 1 << 7
	};

	inline EImageUsage operator|(EImageUsage a, EImageUsage b)
	{
		return static_cast<EImageUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline EImageUsage operator&(EImageUsage a, EImageUsage b)
	{
		return static_cast<EImageUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	enum class ESampleCount
	{
		e1 = 1,
		e2 = 2,
		e4 = 4,
		e8 = 8,
		e16 = 16,
		e32 = 32,
		e64 = 64
	};

	enum class ECommandPoolCreateFlags
		: uint32_t
	{
		None = 0,
		Transient = 1 << 0,
		ResetCommandBuffer = 1 << 1,
		Protected = 1 << 2
	};

	inline ECommandPoolCreateFlags operator|(ECommandPoolCreateFlags a, ECommandPoolCreateFlags b)
	{
		return static_cast<ECommandPoolCreateFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline ECommandPoolCreateFlags operator&(ECommandPoolCreateFlags a, ECommandPoolCreateFlags b)
	{
		return static_cast<ECommandPoolCreateFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	enum class EVertexInputRate
	{
		Vertex,
		Instance
	};

	enum class EPrimitiveTopology
	{
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
		TriangleFan
	};

	enum class EPolygonMode
	{
		Fill,
		Line,
		Point
	};

	enum class ECullMode : uint32_t
	{
		None = 0,
		Front = 1 << 0,
		Back = 1 << 1,
		FrontAndBack = Front | Back
	};

	inline ECullMode operator|(ECullMode a, ECullMode b)
	{
		return static_cast<ECullMode>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline ECullMode operator&(ECullMode a, ECullMode b)
	{
		return static_cast<ECullMode>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	enum class EFrontFace
	{
		CounterClockwise,
		Clockwise
	};

	enum class ECompareOp
	{
		Never,
		Less,
		Equal,
		LessOrEqual,
		Greater,
		NotEqual,
		GreaterOrEqual,
		Always
	};

	enum class EDynamicState
	{
		Viewport,
		Scissor,
		LineWidth,
		DepthBias,
		BlendConstants,
		DepthBounds,
		StencilCompareMask,
		StencilWriteMask,
		StencilReference
	};

	enum class ECommandBufferLevel
	{
		Primary,
		Secondary
	};

	enum class EAccelerationStructureType
	{
		BottomLevel,
		TopLevel
	};

	enum class EPipelineType
	{
		Compute, 
		Graphics
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_UTILS_ENUMS_H
