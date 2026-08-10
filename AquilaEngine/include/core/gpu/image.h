#ifndef AQUILA_ENGINE_CORE_GPU_IMAGE_H
#define AQUILA_ENGINE_CORE_GPU_IMAGE_H
#pragma once

#include <cstdint>
#include <memory>

#include <core/gpu/utils/enums.h>

namespace core::gpu
{
	class Buffer;
	class Device;

	struct ImageCreateInfo
	{
		uint32_t width							= 0;
		uint32_t height							= 0;
		uint32_t mipLevels						= 1;
		uint32_t arrayLayers					= 1;

		utils::ETextureFormat format			= utils::ETextureFormat::RGBA8_SRGB;
		utils::EImageTiling tiling				= utils::EImageTiling::Optimal;
		utils::EImageUsage usage				= utils::EImageUsage::None;
		utils::EMemoryProperty memoryProperties = utils::EMemoryProperty::DeviceLocal;
		utils::ESampleCount samples				= utils::ESampleCount::e1;
	};

	struct ImageViewCreateInfo
	{
		utils::ETextureFormat format	= utils::ETextureFormat::RGBA8_SRGB;

		uint32_t baseMipLevel			= 0;
		uint32_t levelCount				= 1;
		uint32_t baseArrayLayer			= 0;
		uint32_t layerCount				= 1;

		bool isDepth					= false;
	};

	struct PredefinedImageCreateInfo;

	class Image
	{
		private:
			struct Impl;
			std::unique_ptr<Impl> m_impl;

		public:
			Image(const Device& _device, const ImageCreateInfo& _info);
			Image(const Device& _device, const PredefinedImageCreateInfo& _info);

			~Image();

			Impl& GetImpl() const;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_IMAGE_H
