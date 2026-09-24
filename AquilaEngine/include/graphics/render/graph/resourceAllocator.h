#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_RESOURCE_ALLOCATOR_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_RESOURCE_ALLOCATOR_H
#pragma once

#include <memory>

#include <graphics/render/graph/pass.h>

#include <core/gpu/buffer.h>
#include <core/gpu/image.h>
#include <core/gpu/texture.h>

namespace core::gpu { class Device; }

namespace graphics::render::graph
{
	struct PooledResource
	{
		Resource resource;

		std::unique_ptr<core::gpu::Buffer>  ownedBuffer;
		std::unique_ptr<core::gpu::Image>   ownedImage;
		std::unique_ptr<core::gpu::Texture> ownedTexture;

		size_t freeAtPassIndex = 0;
		bool allocated = false;
	};

	class ResourceAllocator
	{
	public:
		explicit ResourceAllocator(const core::gpu::Device& _device)
			: m_device(_device)
		{
		}

		void Allocate(PooledResource& _slot);
		void Free(PooledResource& _slot);

	private:
		const core::gpu::Device& m_device;
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_RESOURCE_ALLOCATOR_H