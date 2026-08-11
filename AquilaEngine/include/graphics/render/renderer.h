#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
#pragma once

#include <core/gpu/device.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <unordered_map>

namespace core::gpu { class AccelerationStructure; class Buffer; class CommandBuffer; class DescriptorSet;  class DescriptorSetLayout; class Image; class Pipeline; class Texture; }

namespace graphics::render
{
	struct Mesh;

	struct UniformBufferObject
	{

	};

	struct GBufferPushConstants {
		glm::mat4 model;
		glm::mat4 prevModel;
	};

	struct PassAttachment
	{
		std::unique_ptr<core::gpu::Image>   image = nullptr;
		std::unique_ptr<core::gpu::Texture> texture = nullptr;
	};

	struct ColorAttachmentDesc
	{
		const core::gpu::Image* image = nullptr;
		bool					clear = true;
		float					clearR = 0.0f;
		float					clearG = 0.0f;
		float					clearB = 0.0f;
		float					clearA = 1.0f;
	};

	struct DepthAttachmentDesc
	{
		const core::gpu::Image* image = nullptr;
		bool					clear = true;
		float					clearDepth = 1.0f;
	};


	class Renderer
	{
	public:
		explicit Renderer(const core::gpu::Device& _device);
		~Renderer() noexcept;

		void Render(core::gpu::CommandBuffer& _cmdBuf, core::gpu::Image& _outputImage);

		void CreateDescriptorSets();
		void UpdateDescriptorSets();

		void CreateUniformBuffers();
		void CreateAttachments();
		void CreateDescriptorSetLayout();
		void CreateMaterialLayout();

		std::unique_ptr<core::gpu::Pipeline> BuildGBufferPipeline();
		std::unique_ptr<core::gpu::Pipeline> BuildLightingPipeline();

		void DrawGBuffer(core::gpu::CommandBuffer& _cmdBuf);
		//void DrawLighting(core::gpu::CommandBuffer& _cmdBuf);

		std::vector<std::unique_ptr<core::gpu::DescriptorSetLayout>>	gBufferDsLayouts;
		std::vector<std::unique_ptr<core::gpu::DescriptorSetLayout>>	lightingDsLayouts;

		std::unique_ptr<core::gpu::DescriptorSetLayout>					materialLayout;

		std::vector<std::unique_ptr<core::gpu::DescriptorSet>>			gBufferDescriptorSets;
		std::vector<std::unique_ptr<core::gpu::DescriptorSet>>			lightingDescriptorSets;

		std::vector<std::unique_ptr<core::gpu::Buffer>>					uniformBuffers;

		std::vector<PassAttachment>										gBufferColorAttachments;
		PassAttachment													gBufferDepthAttachment;

		std::vector<PassAttachment>										lightingColorAttachments;

		core::gpu::AccelerationStructure* tlas = nullptr;
	private:
		const core::gpu::Device& m_device;

		std::unique_ptr<core::gpu::Pipeline>		m_gBufferPipeline;
		std::unique_ptr<core::gpu::Pipeline>		m_lightingPipeline;
		std::unique_ptr<core::gpu::Pipeline>		m_shadowPipeline;
		std::unique_ptr<core::gpu::Pipeline>		m_giPipeline;

		std::vector<std::pair<Mesh*, glm::mat4>>	m_meshInstances;
		std::unordered_map<Mesh*, glm::mat4>		m_prevMeshInstances[core::gpu::Device::FRAMES_IN_FLIGHT];
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H