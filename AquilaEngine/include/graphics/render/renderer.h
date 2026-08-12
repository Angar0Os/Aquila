#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
#pragma once

#include <core/gpu/device.h>
#include <core/gpu/utils/enums.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <unordered_map>

namespace core::gpu { class AccelerationStructure; class Buffer; class CommandBuffer; class DescriptorSet;  class DescriptorSetLayout; class Image; class Pipeline; class Texture; }

namespace graphics::render
{
	struct Mesh;
	struct Material;

	struct UniformBufferObject
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewProjInverse;
		glm::mat4 prevViewProj;
		glm::mat4 prevViewProjInverse;
		glm::vec4 viewPos;
		uint32_t  frameCount;
	};

	struct LightData
	{
		core::gpu::utils::ELightType	type;
		glm::vec3						color;
		float							intensity;
		glm::vec2						spotAngles = { glm::radians(15.0f), glm::radians(30.0f) };
		float							radius = 0.0f;
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

		void Render(core::gpu::CommandBuffer* _cmdBuf, core::gpu::Image& _outputImage);
		void PushMesh(graphics::render::Mesh* _mesh, glm::mat4& _transform);

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
		void CreateDescriptorSets();
		void UpdateDescriptorSets();

		void CreateUniformBuffers();
		void CreateAttachments();
		void CreateDescriptorSetLayout();
		void CreateMaterialLayout();

		std::unique_ptr<core::gpu::Pipeline> BuildGBufferPipeline();
		std::unique_ptr<core::gpu::Pipeline> BuildLightingPipeline();

		void DrawGBuffer(core::gpu::CommandBuffer& _cmdBuf);
		void DrawLighting(core::gpu::CommandBuffer& _cmdBuf);

		void UpdateUniformBuffers();
		void BuildTLAS();
		void RebuildAccelerationStructures();

		const core::gpu::Device& m_device;

		std::unique_ptr<core::gpu::Pipeline>		m_gBufferPipeline;
		std::unique_ptr<core::gpu::Pipeline>		m_lightingPipeline;
		std::unique_ptr<core::gpu::Pipeline>		m_shadowPipeline;
		std::unique_ptr<core::gpu::Pipeline>		m_giPipeline;

		std::unique_ptr<graphics::render::Material> m_fallbackMaterial;
		std::vector<std::pair<Mesh*, glm::mat4>>	m_meshInstances;
		std::unordered_map<Mesh*, glm::mat4>		m_prevMeshInstances[core::gpu::Device::FRAMES_IN_FLIGHT];

		// TODO : We need to find a better way of doing this.
		std::unique_ptr<core::gpu::AccelerationStructure> m_fallbackTlas;
		void CreateFallbackTLAS();

		std::vector<std::unique_ptr<core::gpu::AccelerationStructure>>	m_tlasPerFrame;
		std::vector<LightData>											m_lights;
		
		glm::mat4														m_prevViewProj = glm::mat4(1.0f);
		glm::mat4														m_viewMatrix;
		glm::mat4														m_projMatrix;
		glm::vec3														m_cameraPosition;
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H