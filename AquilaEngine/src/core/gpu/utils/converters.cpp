#include <core/gpu/utils/converters.h>

using namespace core::gpu::utils;

vk::Format ToVulkan(TextureFormat format)
{
	switch (format)
	{
		case TextureFormat::Undefined:				return vk::Format::eUndefined;
		case TextureFormat::R8_UNorm:				return vk::Format::eR8Unorm;
		case TextureFormat::RG8_UNorm:				return vk::Format::eR8G8Unorm;
		case TextureFormat::RGB8_UNorm:				return vk::Format::eR8G8B8Unorm;
		case TextureFormat::RGBA8_UNorm:			return vk::Format::eR8G8B8A8Unorm;
		case TextureFormat::RGBA8_SRGB:				return vk::Format::eR8G8B8A8Srgb;
		case TextureFormat::R16_Float:				return vk::Format::eR16Sfloat;
		case TextureFormat::RG16_Float:				return vk::Format::eR16G16Sfloat;
		case TextureFormat::RGBA16_Float:			return vk::Format::eR16G16B16A16Sfloat;
		case TextureFormat::R32_Float:				return vk::Format::eR32Sfloat;
		case TextureFormat::RG32_Float:				return vk::Format::eR32G32Sfloat;
		case TextureFormat::RGB32_Float:			return vk::Format::eR32G32B32Sfloat;
		case TextureFormat::RGBA32_Float:			return vk::Format::eR32G32B32A32Sfloat;
		case TextureFormat::Depth16:				return vk::Format::eD16Unorm;
		case TextureFormat::Depth24:				return vk::Format::eX8D24UnormPack32;
		case TextureFormat::Depth32F:				return vk::Format::eD32Sfloat;
		case TextureFormat::Depth24Stencil8:		return vk::Format::eD24UnormS8Uint;
		case TextureFormat::Depth32FStencil8:		return vk::Format::eD32SfloatS8Uint;
		case TextureFormat::BC1_RGB_UNorm:			return vk::Format::eBc1RgbUnormBlock;
		case TextureFormat::BC3_RGBA_UNorm:			return vk::Format::eBc3UnormBlock;
		case TextureFormat::BC7_RGBA_UNorm:			return vk::Format::eBc7UnormBlock;
		default:									return vk::Format::eR8G8B8A8Srgb;
	}
}

vk::ImageAspectFlags ToVulkanAspestMask(TextureFormat format)
{
	switch (format)
	{
		case TextureFormat::Undefined:
		case TextureFormat::R8_UNorm:
		case TextureFormat::RG8_UNorm:
		case TextureFormat::RGB8_UNorm:
		case TextureFormat::RGBA8_UNorm:
		case TextureFormat::RGBA8_SRGB:
		case TextureFormat::R16_Float:
		case TextureFormat::RG16_Float:
		case TextureFormat::RGBA16_Float:
		case TextureFormat::R32_Float:
		case TextureFormat::RG32_Float:
		case TextureFormat::RGB32_Float:
		case TextureFormat::RGBA32_Float:
		case TextureFormat::BC1_RGB_UNorm:
		case TextureFormat::BC3_RGBA_UNorm:
		case TextureFormat::BC7_RGBA_UNorm:		return vk::ImageAspectFlagBits::eColor;
		case TextureFormat::Depth16:
		case TextureFormat::Depth24:
		case TextureFormat::Depth32F:
		case TextureFormat::Depth24Stencil8:
		case TextureFormat::Depth32FStencil8:	return vk::ImageAspectFlagBits::eDepth;
		default:								return vk::ImageAspectFlagBits(0);
	}
}

vk::PresentModeKHR ToVulkan(PresentMode mode)
{
	switch (mode)
	{
		case PresentMode::Immediate:	return vk::PresentModeKHR::eImmediate;
		case PresentMode::Mailbox:		return vk::PresentModeKHR::eMailbox;
		case PresentMode::Fifo:			return vk::PresentModeKHR::eFifo;
		case PresentMode::FifoRelaxed:	return vk::PresentModeKHR::eFifoRelaxed;
		default:						return vk::PresentModeKHR::eFifo;
	}
}