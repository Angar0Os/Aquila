#ifndef AQUILA_ENGINE_GRAPHICS_DEFERRED_PASS_BUILDER_H
#define AQUILA_ENGINE_GRAPHICS_DEFERRED_PASS_BUILDER_H
#pragma once

#include <graphics/deferred/pass.h>

namespace graphics::deferred
{
    class RenderGraph;

    class PassBuilder
    {
    public:

        PassBuilder(Pass& _pass, std::vector<std::unique_ptr<PassAttachment>>& _attachments) : m_pass(_pass) {};

        PassAttachment& CreateColorAttachment(
            const std::string& name,
            core::gpu::utils::ETextureFormat format);

        PassAttachment& CreateDepthAttachment(
            const std::string& name,
            core::gpu::utils::ETextureFormat format);

        void Read(PassAttachment& attachment);

        void Write(PassAttachment& attachment);

    private:
        RenderGraph& m_graph;
        Pass& m_pass;
    };
}

#endif //AQUILA_ENGINE_GRAPHICS_DEFERRED_PASS_BUILDER_H
