#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_TEXTURE_LIBRARY_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_TEXTURE_LIBRARY_H
#pragma once

#include <core/gpu/device.h>
#include <core/gpu/image.h>
#include <core/gpu/texture.h>

#include <core/gpu/utils/enums.h>

#include <graphics/render/material.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace graphics::render
{
    enum class ETextureSemantic
    {
        BaseColor,
        Normal,
        ORM,
        Emissive
    };

    class TextureLibrary
    {
    public:
        explicit TextureLibrary(const core::gpu::Device& device);

        TextureHandle Add(const std::string& name, std::unique_ptr<core::gpu::Image> image, std::unique_ptr<core::gpu::Texture> texture);
        TextureHandle Find(const std::string& name) const;
        TextureHandle Load(const std::string& path, ETextureSemantic semantic);

        TextureHandle AddFromPixels(const std::string& _name, const void* _pixels, uint32_t _width, uint32_t _height, ETextureSemantic _semantic);

        const core::gpu::Texture& Get(TextureHandle handle) const;

        bool Contains(const std::string& name) const;
        size_t Size() const;

        TextureHandle GetWhite() const;
        TextureHandle GetBlack() const;
        TextureHandle GetFlatNormal() const;

        static std::unique_ptr<core::gpu::Image> LoadHDRImage(const core::gpu::Device& _device, const std::string& _path);
    private:
        struct Entry
        {
            std::string name;

            std::unique_ptr<core::gpu::Image> image;
            std::unique_ptr<core::gpu::Texture> texture;
        };

        const core::gpu::Device& device;

        std::vector<Entry> textures;
        std::unordered_map<std::string, TextureHandle> lookup;

        std::pair<std::unique_ptr<core::gpu::Image>, std::unique_ptr<core::gpu::Texture>> CreateTexture(
            const core::gpu::Device& _device,
            const void* _pixels,
            uint32_t _width,
            uint32_t _height,
            core::gpu::utils::ETextureFormat _format);

        core::gpu::utils::ETextureFormat GetTextureFormat(ETextureSemantic semantic);

        TextureHandle whiteTexture = INVALID_TEXTURE;
        TextureHandle blackTexture = INVALID_TEXTURE;
        TextureHandle flatNormalTexture = INVALID_TEXTURE;
    };
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_TEXTURE_LIBRARY_H
