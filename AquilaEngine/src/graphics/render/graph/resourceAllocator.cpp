#include <graphics/render/graph/resourceAllocator.h>

#include <core/gpu/device.h>
#include <core/gpu/buffer.h>
#include <core/gpu/image.h>
#include <core/gpu/texture.h>

#include <core/debug/debugNames.h>

using namespace graphics::render::graph;
using namespace core::gpu;

void ResourceAllocator::Allocate(PooledResource& _slot)
{
	Resource& res = _slot.resource;

	switch (res.desc.type)
	{
	case ResourceType::Buffer:
	{
		BufferCreateInfo info{};
		info.size = res.desc.size;
		info.usage = res.desc.bufferUsage;
		info.memoryProperties = res.desc.memoryProperties;

		_slot.ownedBuffer = std::make_unique<Buffer>(m_device, info);
		res.buffer = _slot.ownedBuffer.get();

		AQUILA_SET_DEBUG_NAME(m_device, *res.buffer, res.name);
		break;
	}
	case ResourceType::Texture:
	{
		ImageCreateInfo imageInfo{};
		imageInfo.width = res.desc.width;
		imageInfo.height = res.desc.height;
		imageInfo.mipLevels = res.desc.mipLevels;
		imageInfo.arrayLayers = res.desc.arrayLayers;
		imageInfo.format = res.desc.format;
		imageInfo.tiling = res.desc.tiling;
		imageInfo.usage = res.desc.imageUsage;
		imageInfo.memoryProperties = res.desc.memoryProperties;
		imageInfo.samples = res.desc.samples;

		_slot.ownedImage = std::make_unique<Image>(m_device, imageInfo);
		_slot.ownedTexture = std::make_unique<Texture>(m_device, *_slot.ownedImage, res.desc.filter);

		res.image = _slot.ownedImage.get();
		res.texture = _slot.ownedTexture.get();

		AQUILA_SET_DEBUG_NAME(m_device, *res.image, res.name);
		AQUILA_SET_DEBUG_NAME(m_device, *res.texture, res.name);
		break;
	}
	}
}

void ResourceAllocator::Free(PooledResource& _slot)
{
	_slot.ownedBuffer.reset();
	_slot.ownedTexture.reset();
	_slot.ownedImage.reset();

	_slot.resource.buffer = nullptr;
	_slot.resource.texture = nullptr;
	_slot.resource.image = nullptr;

	_slot.allocated = false;
}