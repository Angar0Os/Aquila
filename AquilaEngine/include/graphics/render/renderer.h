#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace core::gpu { class AccelerationStructure;  class CommandBuffer; class Device; class DescriptorSet;  class DescriptorSetLayout; class Image; class Pipeline; class Texture;}

namespace graphics::render
{
	struct Vertex 
	{
	public:
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec4 tangent;
	};

	struct GBufferPushConstants {
		glm::mat4 model;
		glm::mat4 prevModel;
	};

	struct PassAttachment
	{
		std::unique_ptr<core::gpu::Image>   image	= nullptr;
		std::unique_ptr<core::gpu::Texture> texture = nullptr;
	};

	struct ColorAttachmentDesc
	{
		const core::gpu::Image* image	= nullptr;
		bool					clear	= true;
		float					clearR	= 0.0f;
		float					clearG	= 0.0f;
		float					clearB	= 0.0f;
		float					clearA	= 1.0f;
	};
		
	struct DepthAttachmentDesc
	{
		const core::gpu::Image* image		= nullptr;
		bool					clear		= true;
		float					clearDepth	= 1.0f;
	};


	class Renderer
	{
	public:
		explicit Renderer(core::gpu::Device _device, core::gpu::CommandBuffer _cmdBuf);
		~Renderer() noexcept;

		void Render(core::gpu::CommandBuffer& _cmdBuf);

		void CreateDescriptorSets(const core::gpu::Device& _device, const core::gpu::CommandBuffer& _cmdBuf);
		void UpdateDescriptorSets(const core::gpu::Device& _device);

		void CreateAttachments(core::gpu::Device& _device);
		void CreateDescriptorSetLayout(core::gpu::Device& _device);
		void CreateMaterialLayout(core::gpu::Device& _device);

		std::unique_ptr<core::gpu::Pipeline> BuildGBufferPipeline(const core::gpu::Device& _device);
		std::unique_ptr<core::gpu::Pipeline> BuildLightingPipeline(const core::gpu::Device& _device);


		std::vector<std::unique_ptr<core::gpu::DescriptorSetLayout>>	gBufferDsLayouts;
		std::vector<std::unique_ptr<core::gpu::DescriptorSetLayout>>	lightingDsLayouts;

		std::unique_ptr<core::gpu::DescriptorSetLayout>					materialLayout;

		std::vector<std::unique_ptr<core::gpu::DescriptorSet>>			gBufferDescriptorSets;
		std::vector<std::unique_ptr<core::gpu::DescriptorSet>>			lightingDescriptorSets;

		std::vector<std::unique_ptr<core::gpu::Buffer>>					gBufferUniformBuffers;
		std::vector<std::unique_ptr<core::gpu::Buffer>>					lightingUniformBuffers;

		std::vector<PassAttachment>										gBufferColorAttachments;
		PassAttachment													gBufferDepthAttachment;
		
		std::vector<PassAttachment>										lightingColorAttachments;
		PassAttachment													lightingDepthAttachment;

		core::gpu::AccelerationStructure* tlas = nullptr;
	private:
		std::unique_ptr<core::gpu::Pipeline> m_gBufferPipeline;
		std::unique_ptr<core::gpu::Pipeline> m_lightingPipeline;
		std::unique_ptr<core::gpu::Pipeline> m_shadowPipeline;
		std::unique_ptr<core::gpu::Pipeline> m_giPipeline;
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
