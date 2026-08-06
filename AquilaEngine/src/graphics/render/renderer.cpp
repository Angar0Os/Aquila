#include <core/gpu/commandBuffer.h>

#include <graphics/render/renderer.h>
#include <graphics/deferred/renderGraph.h>
#include <graphics/deferred/pass.h>

using namespace graphics::render;

Renderer::Renderer()
{
    deferred::RenderGraph renderGraph; // TODO : Need to move this into a private renderGraph var so it's unique.
    renderGraph.Compile();
}

Renderer::~Renderer() = default;

void Renderer::Render(core::gpu::CommandBuffer& _cmdBuf)
{
	deferred::RenderGraph renderGraph;

	BuildGBufferPass(renderGraph);
	BuildLightingPass(renderGraph);

	renderGraph.Compile();
	renderGraph.Execute(_cmdBuf);
}

void Renderer::BuildGBufferPass(graphics::deferred::RenderGraph& _graph)
{
	_graph.AddPass(
		"Gbuffer",

        [&](deferred::PassBuilder& builder)
        {
            builder.CreateColorAttachment(
                "Albedo",
                core::gpu::utils::ETextureFormat::RGBA8_SRGB);

            builder.CreateColorAttachment(
                "Normal",
                core::gpu::utils::ETextureFormat::RGBA16_Float);

            builder.CreateDepthAttachment(
                "Depth",
                core::gpu::utils::ETextureFormat::Depth32F);
        },

        [&](core::gpu::CommandBuffer& _cmdBuf)
        {

        });
}

void Renderer::BuildLightingPass(graphics::deferred::RenderGraph& _graph)
{
    _graph.AddPass(
        "Lighting",

        [&](deferred::PassBuilder& builder)
        {
            builder.Read(*m_albedo);
            builder.Read(*m_normal);
            builder.Read(*m_depth);

            builder.CreateColorAttachment(
                "HDR",
                core::gpu::utils::ETextureFormat::RGBA16_Float
            );
        },
        [&](core::gpu::CommandBuffer& _cmdBuf)
        {

        }
    );
}