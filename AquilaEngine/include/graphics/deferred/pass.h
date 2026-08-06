#ifndef AQUILA_ENGINE_GRAPHICS_DEFERRED_PASS_H
#define AQUILA_ENGINE_GRAPHICS_DEFERRED_PASS_H
#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include <core/gpu/utils/enums.h>

namespace core::gpu
{
    class CommandBuffer;
    class Image;
    class Texture;
}

namespace graphics::deferred
{
    class RenderGraph;

    enum class EAttachmentType
    {
        Color,
        Depth
    };

    struct PassAttachment
    {
        std::string name;

        EAttachmentType type;

        core::gpu::utils::ETextureFormat format;
        core::gpu::utils::EImageUsage usage;

        uint32_t width = 0;
        uint32_t height = 0;

        core::gpu::Image* image = nullptr;
        core::gpu::Texture* texture = nullptr;
    };

    struct Pass
    {
        std::string name;

        std::vector<PassAttachment*> inputs;
        std::vector<PassAttachment*> outputs;

        std::function<void(core::gpu::CommandBuffer&)> execute;
    };
}

#endif //AQUILA_ENGINE_GRAPHICS_DEFERRED_PASS_H