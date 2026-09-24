#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_LIBRARY_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_LIBRARY_H
#pragma once

#include <graphics/render/material.h>
#include <graphics/render/materialGPUData.h>
#include <graphics/render/textureLibrary.h>

#include <core/gpu/device.h>
#include <core/gpu/buffer.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace graphics::render
{
	class MaterialLibrary
	{
	public:
		explicit MaterialLibrary(const core::gpu::Device& _device, TextureLibrary& _textureLibrary);

		MaterialHandle Add(const std::string& _name, Material _material);
		MaterialHandle Find(const std::string& _name) const;

		bool Contains(const std::string& _name) const;

		const Material& Get(MaterialHandle _handle) const;

		size_t Size() const;

		MaterialGPUData BuildGPUData(MaterialHandle _handle) const;
		void UploadGPUData();

		const core::gpu::Buffer& GetGPUBuffer() const;
		MaterialHandle GetDefaultMaterial() const;

	private:
		struct Entry
		{
			std::string name;
			Material material;
		};

		const core::gpu::Device& device;
		TextureLibrary& textureLibrary;

		std::vector<Entry> materials;
		std::unordered_map<std::string, MaterialHandle> lookup;

		std::unique_ptr<core::gpu::Buffer> gpuBuffer;

		static constexpr size_t MAX_MATERIALS_CAPACITY = 512;
		bool dirty = true;

		MaterialHandle defaultMaterial = INVALID_MATERIAL;
	};
}

#endif // AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_LIBRARY_H