#include <core/gpu/utils/converters.h>

vk::Format core::gpu::utils::ToVulkan(core::gpu::utils::ETextureFormat format)
{
	switch (format)
	{
		case core::gpu::utils::ETextureFormat::Undefined:				return vk::Format::eUndefined;
		case core::gpu::utils::ETextureFormat::R8_UNorm:					return vk::Format::eR8Unorm;
		case core::gpu::utils::ETextureFormat::RG8_UNorm:				return vk::Format::eR8G8Unorm;
		case core::gpu::utils::ETextureFormat::RGB8_UNorm:				return vk::Format::eR8G8B8Unorm;
		case core::gpu::utils::ETextureFormat::RGBA8_UNorm:				return vk::Format::eR8G8B8A8Unorm;
		case core::gpu::utils::ETextureFormat::RGBA8_SRGB:				return vk::Format::eR8G8B8A8Srgb;
		case core::gpu::utils::ETextureFormat::R16_Float:				return vk::Format::eR16Sfloat;
		case core::gpu::utils::ETextureFormat::RG16_Float:				return vk::Format::eR16G16Sfloat;
		case core::gpu::utils::ETextureFormat::RGBA16_Float:				return vk::Format::eR16G16B16A16Sfloat;
		case core::gpu::utils::ETextureFormat::R32_Float:				return vk::Format::eR32Sfloat;
		case core::gpu::utils::ETextureFormat::RG32_Float:				return vk::Format::eR32G32Sfloat;
		case core::gpu::utils::ETextureFormat::RGB32_Float:				return vk::Format::eR32G32B32Sfloat;
		case core::gpu::utils::ETextureFormat::RGBA32_Float:				return vk::Format::eR32G32B32A32Sfloat;
		case core::gpu::utils::ETextureFormat::Depth16:					return vk::Format::eD16Unorm;
		case core::gpu::utils::ETextureFormat::Depth24:					return vk::Format::eX8D24UnormPack32;
		case core::gpu::utils::ETextureFormat::Depth32F:					return vk::Format::eD32Sfloat;
		case core::gpu::utils::ETextureFormat::Depth24Stencil8:			return vk::Format::eD24UnormS8Uint;
		case core::gpu::utils::ETextureFormat::Depth32FStencil8:			return vk::Format::eD32SfloatS8Uint;
		case core::gpu::utils::ETextureFormat::BC1_RGB_UNorm:			return vk::Format::eBc1RgbUnormBlock;
		case core::gpu::utils::ETextureFormat::BC3_RGBA_UNorm:			return vk::Format::eBc3UnormBlock;
		case core::gpu::utils::ETextureFormat::BC7_RGBA_UNorm:			return vk::Format::eBc7UnormBlock;
		default:														return vk::Format::eR8G8B8A8Srgb;
	}
}

vk::ImageAspectFlags core::gpu::utils::ToVulkanAspestMask(core::gpu::utils::ETextureFormat format)
{
	switch (format)
	{
		case core::gpu::utils::ETextureFormat::Undefined:
		case core::gpu::utils::ETextureFormat::R8_UNorm:
		case core::gpu::utils::ETextureFormat::RG8_UNorm:
		case core::gpu::utils::ETextureFormat::RGB8_UNorm:
		case core::gpu::utils::ETextureFormat::RGBA8_UNorm:
		case core::gpu::utils::ETextureFormat::RGBA8_SRGB:
		case core::gpu::utils::ETextureFormat::R16_Float:
		case core::gpu::utils::ETextureFormat::RG16_Float:
		case core::gpu::utils::ETextureFormat::RGBA16_Float:
		case core::gpu::utils::ETextureFormat::R32_Float:
		case core::gpu::utils::ETextureFormat::RG32_Float:
		case core::gpu::utils::ETextureFormat::RGB32_Float:
		case core::gpu::utils::ETextureFormat::RGBA32_Float:
		case core::gpu::utils::ETextureFormat::BC1_RGB_UNorm:
		case core::gpu::utils::ETextureFormat::BC3_RGBA_UNorm:
		case core::gpu::utils::ETextureFormat::BC7_RGBA_UNorm:		return vk::ImageAspectFlagBits::eColor;
		case core::gpu::utils::ETextureFormat::Depth16:
		case core::gpu::utils::ETextureFormat::Depth24:
		case core::gpu::utils::ETextureFormat::Depth32F:
		case core::gpu::utils::ETextureFormat::Depth24Stencil8:
		case core::gpu::utils::ETextureFormat::Depth32FStencil8:		return vk::ImageAspectFlagBits::eDepth;
		default:													return vk::ImageAspectFlagBits(0);
	}
}

vk::PresentModeKHR core::gpu::utils::ToVulkan(core::gpu::utils::EPresentMode mode)
{
	switch (mode)
	{
		case core::gpu::utils::EPresentMode::Immediate:		return vk::PresentModeKHR::eImmediate;
		case core::gpu::utils::EPresentMode::Mailbox:		return vk::PresentModeKHR::eMailbox;
		case core::gpu::utils::EPresentMode::Fifo:			return vk::PresentModeKHR::eFifo;
		case core::gpu::utils::EPresentMode::FifoRelaxed:	return vk::PresentModeKHR::eFifoRelaxed;
		default:											return vk::PresentModeKHR::eFifo;
	}
}

vk::BufferUsageFlags core::gpu::utils::ToVulkan(core::gpu::utils::EBufferUsage usage)
{
	vk::BufferUsageFlags flags;

	if ((usage & core::gpu::utils::EBufferUsage::TransferSrc) != core::gpu::utils::EBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eTransferSrc;

	if ((usage & core::gpu::utils::EBufferUsage::TransferDst) != core::gpu::utils::EBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eTransferDst;

	if ((usage & core::gpu::utils::EBufferUsage::UniformBuffer) != core::gpu::utils::EBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eUniformBuffer;

	if ((usage & core::gpu::utils::EBufferUsage::StorageBuffer) != core::gpu::utils::EBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eStorageBuffer;

	if ((usage & core::gpu::utils::EBufferUsage::IndexBuffer) != core::gpu::utils::EBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eIndexBuffer;

	if ((usage & core::gpu::utils::EBufferUsage::VertexBuffer) != core::gpu::utils::EBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eVertexBuffer;

	if ((usage & core::gpu::utils::EBufferUsage::IndirectBuffer) != core::gpu::utils::EBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eIndirectBuffer;

	if ((usage & core::gpu::utils::EBufferUsage::ShaderDeviceAddress) != core::gpu::utils::EBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eShaderDeviceAddress;

	if ((usage & core::gpu::utils::EBufferUsage::AccelerationStructureStorage) != core::gpu::utils::EBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;

	if ((usage & core::gpu::utils::EBufferUsage::AccelerationStructureBuildInput) != core::gpu::utils::EBufferUsage::None)
		flags |= vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;

	return flags;
}

vk::MemoryPropertyFlags core::gpu::utils::ToVulkan(core::gpu::utils::EMemoryProperty properties)
{
	vk::MemoryPropertyFlags flags;

	if ((properties & core::gpu::utils::EMemoryProperty::DeviceLocal) != core::gpu::utils::EMemoryProperty::None)
		flags |= vk::MemoryPropertyFlagBits::eDeviceLocal;

	if ((properties & core::gpu::utils::EMemoryProperty::HostVisible) != core::gpu::utils::EMemoryProperty::None)
		flags |= vk::MemoryPropertyFlagBits::eHostVisible;

	if ((properties & core::gpu::utils::EMemoryProperty::HostCoherent) != core::gpu::utils::EMemoryProperty::None)
		flags |= vk::MemoryPropertyFlagBits::eHostCoherent;

	if ((properties & core::gpu::utils::EMemoryProperty::HostCached) != core::gpu::utils::EMemoryProperty::None)
		flags |= vk::MemoryPropertyFlagBits::eHostCached;

	return flags;
}

vk::DescriptorType core::gpu::utils::ToVulkan(core::gpu::utils::EDescriptorType type)
{
	switch (type)
	{
	case core::gpu::utils::EDescriptorType::UniformBuffer:
		return vk::DescriptorType::eUniformBuffer;
	case core::gpu::utils::EDescriptorType::CombinedImageSampler:
		return vk::DescriptorType::eCombinedImageSampler;
	case core::gpu::utils::EDescriptorType::StorageBuffer:
		return vk::DescriptorType::eStorageBuffer;
	case core::gpu::utils::EDescriptorType::StorageImage:
		return vk::DescriptorType::eStorageImage;
	case core::gpu::utils::EDescriptorType::AccelerationStructure:
		return vk::DescriptorType::eAccelerationStructureKHR;
	default:
		throw std::runtime_error("Unknown descriptor type");
	}
}

vk::ShaderStageFlagBits core::gpu::utils::ToVulkan(core::gpu::utils::EShaderStageFlags stage)
{
	if ((stage & core::gpu::utils::EShaderStageFlags::Vertex) != core::gpu::utils::EShaderStageFlags::None)
		return vk::ShaderStageFlagBits::eVertex;
	if ((stage & core::gpu::utils::EShaderStageFlags::Fragment) != core::gpu::utils::EShaderStageFlags::None)
		return vk::ShaderStageFlagBits::eFragment;
	if ((stage & core::gpu::utils::EShaderStageFlags::Compute) != core::gpu::utils::EShaderStageFlags::None)
		return vk::ShaderStageFlagBits::eCompute;
	if ((stage & core::gpu::utils::EShaderStageFlags::Geometry) != core::gpu::utils::EShaderStageFlags::None)
		return vk::ShaderStageFlagBits::eGeometry;
	if ((stage & core::gpu::utils::EShaderStageFlags::TessellationControl) != core::gpu::utils::EShaderStageFlags::None)
		return vk::ShaderStageFlagBits::eTessellationControl;
	if ((stage & core::gpu::utils::EShaderStageFlags::TessellationEvaluation) != core::gpu::utils::EShaderStageFlags::None)
		return vk::ShaderStageFlagBits::eTessellationEvaluation;

	return vk::ShaderStageFlagBits::eVertex;
}

vk::ShaderStageFlags core::gpu::utils::ToVulkan(core::gpu::utils::EShaderStage stages)
{
	vk::ShaderStageFlags result;

	if ((stages & core::gpu::utils::EShaderStage::Vertex) != core::gpu::utils::EShaderStage::None)
		result |= vk::ShaderStageFlagBits::eVertex;

	if ((stages & core::gpu::utils::EShaderStage::Fragment) != core::gpu::utils::EShaderStage::None)
		result |= vk::ShaderStageFlagBits::eFragment;

	if ((stages & core::gpu::utils::EShaderStage::Geometry) != core::gpu::utils::EShaderStage::None)
		result |= vk::ShaderStageFlagBits::eGeometry;

	if ((stages & core::gpu::utils::EShaderStage::Compute) != core::gpu::utils::EShaderStage::None)
		result |= vk::ShaderStageFlagBits::eCompute;

	if ((stages & core::gpu::utils::EShaderStage::TessellationControl) != core::gpu::utils::EShaderStage::None)
		result |= vk::ShaderStageFlagBits::eTessellationControl;

	if ((stages & core::gpu::utils::EShaderStage::TessellationEvaluation) != core::gpu::utils::EShaderStage::None)
		result |= vk::ShaderStageFlagBits::eTessellationEvaluation;

	return result;
}

vk::ImageTiling core::gpu::utils::ToVulkan(core::gpu::utils::EImageTiling tiling)
{
	switch (tiling)
	{
		case core::gpu::utils::EImageTiling::Optimal:	return vk::ImageTiling::eOptimal;
		case core::gpu::utils::EImageTiling::Linear:	return vk::ImageTiling::eLinear;
		default:										return vk::ImageTiling::eOptimal;
	}
}

vk::SampleCountFlagBits core::gpu::utils::ToVulkan(core::gpu::utils::ESampleCount samples)
{
	switch (samples)
	{
		case core::gpu::utils::ESampleCount::e1:	return vk::SampleCountFlagBits::e1;
		case core::gpu::utils::ESampleCount::e2:	return vk::SampleCountFlagBits::e2;
		case core::gpu::utils::ESampleCount::e4:	return vk::SampleCountFlagBits::e4;
		case core::gpu::utils::ESampleCount::e8:	return vk::SampleCountFlagBits::e8;
		case core::gpu::utils::ESampleCount::e16:	return vk::SampleCountFlagBits::e16;
		case core::gpu::utils::ESampleCount::e32:	return vk::SampleCountFlagBits::e32;
		case core::gpu::utils::ESampleCount::e64:	return vk::SampleCountFlagBits::e64;
		default:									return vk::SampleCountFlagBits::e1;
	}
}

vk::ImageUsageFlags core::gpu::utils::ToVulkan(core::gpu::utils::EImageUsage usage)
{
	vk::ImageUsageFlags flags;

	if ((usage & core::gpu::utils::EImageUsage::TransferSrc) != core::gpu::utils::EImageUsage::None)
		flags |= vk::ImageUsageFlagBits::eTransferSrc;

	if ((usage & core::gpu::utils::EImageUsage::TransferDst) != core::gpu::utils::EImageUsage::None)
		flags |= vk::ImageUsageFlagBits::eTransferDst;

	if ((usage & core::gpu::utils::EImageUsage::Sampled) != core::gpu::utils::EImageUsage::None)
		flags |= vk::ImageUsageFlagBits::eSampled;

	if ((usage & core::gpu::utils::EImageUsage::Storage) != core::gpu::utils::EImageUsage::None)
		flags |= vk::ImageUsageFlagBits::eStorage;

	if ((usage & core::gpu::utils::EImageUsage::ColorAttachment) != core::gpu::utils::EImageUsage::None)
		flags |= vk::ImageUsageFlagBits::eColorAttachment;

	if ((usage & core::gpu::utils::EImageUsage::DepthStencilAttachment) != core::gpu::utils::EImageUsage::None)
		flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

	if ((usage & core::gpu::utils::EImageUsage::InputAttachment) != core::gpu::utils::EImageUsage::None)
		flags |= vk::ImageUsageFlagBits::eInputAttachment;

	return flags;
}

vk::CommandPoolCreateFlags core::gpu::utils::ToVulkan(core::gpu::utils::ECommandPoolCreateFlags flags)
{
	vk::CommandPoolCreateFlags vkFlags;

	if (static_cast<int>(flags) & static_cast<int>(core::gpu::utils::ECommandPoolCreateFlags::Transient))
		vkFlags |= vk::CommandPoolCreateFlagBits::eTransient;

	if (static_cast<int>(flags) & static_cast<int>(core::gpu::utils::ECommandPoolCreateFlags::ResetCommandBuffer))
		vkFlags |= vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	if (static_cast<int>(flags) & static_cast<int>(core::gpu::utils::ECommandPoolCreateFlags::Protected))
		vkFlags |= vk::CommandPoolCreateFlagBits::eProtected;

	return vkFlags;
}