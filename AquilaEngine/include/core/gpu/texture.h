#ifndef AQUILA_ENGINE_CORE_GPU_TEXTURE_H
#define AQUILA_ENGINE_CORE_GPU_TEXTURE_H
#pragma once

#include <memory>

namespace core::gpu
{
	class Device;
	class Image;

	class Texture
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;

	public:
		Texture(const Device& _device, const Image& _info);
		~Texture();

		Impl& GetImpl() const;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_TEXTURE_H
