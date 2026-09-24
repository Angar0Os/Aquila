#ifndef AQUILA_ENGINE_CORE_GPU_TEXTURE_H
#define AQUILA_ENGINE_CORE_GPU_TEXTURE_H
#pragma once

#include <memory>
#include <core/gpu/utils/enums.h>
#include <string_view>

namespace core::gpu
{
	class Device;
	class Image;

	struct TextureCreateInfo
	{
		utils::ETextureFilter filter = utils::ETextureFilter::Linear;
	};

	class Texture
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;

	public:
		Texture(const Device& _device, const Image& _image, utils::ETextureFilter _filter = utils::ETextureFilter::Linear);
		~Texture();

		void SetDebugName(const Device& _device, std::string_view _name);

		Impl& GetImpl() const;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_TEXTURE_H
