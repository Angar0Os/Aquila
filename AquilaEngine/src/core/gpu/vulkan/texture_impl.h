#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_TEXTURE_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_TEXTURE_H
#pragma once

#include <core/gpu/texture.h>
#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	struct Texture::Impl
	{
		const Image*		image	= nullptr;
		vk::raii::Sampler	sampler = nullptr;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_TEXTURE_H
