#ifndef AQUILA_ENGINE_LOADERS_MESH_LOADER_H
#define AQUILA_ENGINE_LOADERS_MESH_LOADER_H
#pragma once

#include <string>
#include <memory>
#include <vector>

namespace graphics::render { struct Mesh; struct MeshInstance; struct SceneMeshInstance; }
namespace core::gpu { class Device; class DescriptorSetLayout; }

namespace loaders
{
	class MeshLoader
	{
	public:
		static std::unique_ptr<graphics::render::Mesh> LoadGLTF(const core::gpu::Device& _device, const std::string& _path, const core::gpu::DescriptorSetLayout& _materialLayout);
		static std::vector<graphics::render::SceneMeshInstance> LoadGLTFScene(const core::gpu::Device& _device, const std::string& _path, const core::gpu::DescriptorSetLayout& _materialLayout);
	};
}

#endif //AQUILA_ENGINE_LOADERS_MESH_LOADER_H
