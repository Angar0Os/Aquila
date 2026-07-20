#include <core/gpu/utils/converters.h>

vk::Format core::gpu::utils::ToVulkan(core::gpu::utils::TextureFormat format)
{
	switch (format)
	{
		case core::gpu::utils::TextureFormat::Undefined:				return vk::Format::eUndefined;
		case core::gpu::utils::TextureFormat::R8_UNorm:					return vk::Format::eR8Unorm;
		case core::gpu::utils::TextureFormat::RG8_UNorm:				return vk::Format::eR8G8Unorm;
		case core::gpu::utils::TextureFormat::RGB8_UNorm:				return vk::Format::eR8G8B8Unorm;
		case core::gpu::utils::TextureFormat::RGBA8_UNorm:				return vk::Format::eR8G8B8A8Unorm;
		case core::gpu::utils::TextureFormat::RGBA8_SRGB:				return vk::Format::eR8G8B8A8Srgb;
		case core::gpu::utils::TextureFormat::R16_Float:				return vk::Format::eR16Sfloat;
		case core::gpu::utils::TextureFormat::RG16_Float:				return vk::Format::eR16G16Sfloat;
		case core::gpu::utils::TextureFormat::RGBA16_Float:				return vk::Format::eR16G16B16A16Sfloat;
		case core::gpu::utils::TextureFormat::R32_Float:				return vk::Format::eR32Sfloat;
		case core::gpu::utils::TextureFormat::RG32_Float:				return vk::Format::eR32G32Sfloat;
		case core::gpu::utils::TextureFormat::RGB32_Float:				return vk::Format::eR32G32B32Sfloat;
		case core::gpu::utils::TextureFormat::RGBA32_Float:				return vk::Format::eR32G32B32A32Sfloat;
		case core::gpu::utils::TextureFormat::Depth16:					return vk::Format::eD16Unorm;
		case core::gpu::utils::TextureFormat::Depth24:					return vk::Format::eX8D24UnormPack32;
		case core::gpu::utils::TextureFormat::Depth32F:					return vk::Format::eD32Sfloat;
		case core::gpu::utils::TextureFormat::Depth24Stencil8:			return vk::Format::eD24UnormS8Uint;
		case core::gpu::utils::TextureFormat::Depth32FStencil8:			return vk::Format::eD32SfloatS8Uint;
		case core::gpu::utils::TextureFormat::BC1_RGB_UNorm:			return vk::Format::eBc1RgbUnormBlock;
		case core::gpu::utils::TextureFormat::BC3_RGBA_UNorm:			return vk::Format::eBc3UnormBlock;
		case core::gpu::utils::TextureFormat::BC7_RGBA_UNorm:			return vk::Format::eBc7UnormBlock;
		default:														return vk::Format::eR8G8B8A8Srgb;
	}
}

vk::ImageAspectFlags core::gpu::utils::ToVulkanAspestMask(core::gpu::utils::TextureFormat format)
{
	switch (format)
	{
		case core::gpu::utils::TextureFormat::Undefined:
		case core::gpu::utils::TextureFormat::R8_UNorm:
		case core::gpu::utils::TextureFormat::RG8_UNorm:
		case core::gpu::utils::TextureFormat::RGB8_UNorm:
		case core::gpu::utils::TextureFormat::RGBA8_UNorm:
		case core::gpu::utils::TextureFormat::RGBA8_SRGB:
		case core::gpu::utils::TextureFormat::R16_Float:
		case core::gpu::utils::TextureFormat::RG16_Float:
		case core::gpu::utils::TextureFormat::RGBA16_Float:
		case core::gpu::utils::TextureFormat::R32_Float:
		case core::gpu::utils::TextureFormat::RG32_Float:
		case core::gpu::utils::TextureFormat::RGB32_Float:
		case core::gpu::utils::TextureFormat::RGBA32_Float:
		case core::gpu::utils::TextureFormat::BC1_RGB_UNorm:
		case core::gpu::utils::TextureFormat::BC3_RGBA_UNorm:
		case core::gpu::utils::TextureFormat::BC7_RGBA_UNorm:		return vk::ImageAspectFlagBits::eColor;
		case core::gpu::utils::TextureFormat::Depth16:
		case core::gpu::utils::TextureFormat::Depth24:
		case core::gpu::utils::TextureFormat::Depth32F:
		case core::gpu::utils::TextureFormat::Depth24Stencil8:
		case core::gpu::utils::TextureFormat::Depth32FStencil8:		return vk::ImageAspectFlagBits::eDepth;
		default:													return vk::ImageAspectFlagBits(0);
	}
}

vk::PresentModeKHR core::gpu::utils::ToVulkan(core::gpu::utils::PresentMode mode)
{
	switch (mode)
	{
		case core::gpu::utils::PresentMode::Immediate:		return vk::PresentModeKHR::eImmediate;
		case core::gpu::utils::PresentMode::Mailbox:		return vk::PresentModeKHR::eMailbox;
		case core::gpu::utils::PresentMode::Fifo:			return vk::PresentModeKHR::eFifo;
		case core::gpu::utils::PresentMode::FifoRelaxed:	return vk::PresentModeKHR::eFifoRelaxed;
		default:											return vk::PresentModeKHR::eFifo;
	}
}