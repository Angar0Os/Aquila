#ifndef AQUILA_ENGINE_LOADERS_MESH_LOADER_H
#define AQUILA_ENGINE_LOADERS_MESH_LOADER_H
#pragma once

#include <graphics/render/mesh.h>

#include <core/gpu/device.h>

#include <string>
#include <vector>
#include <memory>

namespace graphics::render
{
    class TextureLibrary;
    class MaterialLibrary;
}

namespace loaders
{
    class MeshLoader
    {
    public:
        static std::vector<std::unique_ptr<graphics::render::Mesh>> LoadGLTF(const core::gpu::Device& _device, const std::string& _path, graphics::render::TextureLibrary& _textureLibrary, graphics::render::MaterialLibrary& _materialLibrary);
    };
}

#endif // AQUILA_ENGINE_LOADERS_MESH_LOADER_H