#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H
#pragma once

#include <core/gpu/device.h>
#include <core/gpu/utils/enums.h>

#include <graphics/render/materialLibrary.h>
#include <graphics/render/textureLibrary.h>


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

	struct GPULight // TODO : We will remove this and put this into UBO after Evoke.
	{
		glm::vec3 position;
		float     radius;
		glm::vec3 color;
		float     intensity;
	};

	struct GBufferPushConstants
	{
		glm::mat4 model;
		glm::mat4 prevModel;
		uint32_t  materialId;
		uint32_t  _pad[3] = { 0, 0, 0 };
	};

	struct ShadowPushConstants
	{
		glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));
		float     maxDistance = 1000.0f;
	};

	struct ResolvePushConstants
	{
		uint32_t numLights = 0;
		float    _pad[3] = { 0.0f, 0.0f, 0.0f };
	};

	struct FXAAPushConstants
	{
		glm::vec2 texelSize;
		float     _pad[2] = { 0.0f, 0.0f };
	};

	struct MeshTableEntry
	{
		uintptr_t indices;
		uintptr_t positions;
		uintptr_t normals;
		uintptr_t uvs;
		uintptr_t tangents;
		uint32_t  materialId;
		uint32_t  vertexStride;
	};

	struct GIPushConstants
	{
		uint32_t  sampleCount = 8;
		float     maxDistance = 50.0f;
		float     _pad0[2] = { 0.0f, 0.0f };
		glm::vec3 sunDirection = glm::vec3(0.0f, 1.0f, 0.0f);
		float     _pad1 = 0.0f;
		glm::vec3 sunColor = glm::vec3(1.0f);
		float     _pad2 = 0.0f;
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
		void RegisterMesh(graphics::render::Mesh* _mesh);

		void SetSunDirection(const glm::vec3& _direction) { m_sunDirection = glm::normalize(_direction); } // TODO : This is only to debug GI.

		void PushLight(const glm::vec3& _position, const glm::vec3& _color, float _intensity, float _radius)
		{
			m_pointLights.push_back({ _position, _radius, _color, _intensity });
		}

		TextureLibrary& GetTextureLibrary() { return *m_textureLibrary; }
		MaterialLibrary& GetMaterialLibrary() { return *m_materialLibrary; }

		std::vector<std::unique_ptr<core::gpu::DescriptorSetLayout>>	gBufferDsLayouts;
		std::vector<std::unique_ptr<core::gpu::DescriptorSetLayout>>	shadowDsLayouts;
		std::vector<std::unique_ptr<core::gpu::DescriptorSetLayout>>	resolveDsLayouts;
		std::vector<std::unique_ptr<core::gpu::DescriptorSetLayout>>	giDsLayouts;
		std::vector<std::unique_ptr<core::gpu::DescriptorSetLayout>>	fxaaDsLayouts;

		std::unique_ptr<core::gpu::DescriptorSetLayout>					materialLayout;

		std::vector<std::unique_ptr<core::gpu::DescriptorSet>>			gBufferDescriptorSets;
		std::vector<std::unique_ptr<core::gpu::DescriptorSet>>			resolveDescriptorSets;
		std::vector<std::unique_ptr<core::gpu::DescriptorSet>>			shadowDescriptorSets;
		std::vector<std::unique_ptr<core::gpu::DescriptorSet>>			giDescriptorSets;
		std::vector<std::unique_ptr<core::gpu::DescriptorSet>>			fxaaDescriptorSets;

		std::vector<std::unique_ptr<core::gpu::Buffer>>					uniformBuffers;

		std::vector<PassAttachment>										gBufferColorAttachments;
		PassAttachment													gBufferDepthAttachment;
		std::vector<PassAttachment>										giColorAttachments;
		std::vector<PassAttachment>										resolveColorAttachments;
		std::vector<PassAttachment>										aaColorAttachments;



		PassAttachment													shadowMaskAttachment;

		core::gpu::AccelerationStructure* tlas = nullptr;

	private:
		void CreateDescriptorSets();
		void UpdateDescriptorSets();

		void CreateUniformBuffers();
		void CreateAttachments();
		void CreateDescriptorSetLayout();
		void CreateMaterialLayout();
		void LoadEnvironmentMaps();


		std::unique_ptr<core::gpu::Pipeline> BuildGBufferPipeline();
		std::unique_ptr<core::gpu::Pipeline> BuildShadowPipeline();
		std::unique_ptr<core::gpu::Pipeline> BuildGIPipeline();
		std::unique_ptr<core::gpu::Pipeline> BuildResolvePipeline();
		std::unique_ptr<core::gpu::Pipeline> BuildFXAAPipeline();

		void DrawGBuffer(core::gpu::CommandBuffer& _cmdBuf);
		void DrawShadow(core::gpu::CommandBuffer& _cmdBuf);
		void DrawGI(core::gpu::CommandBuffer& _cmdBuf);
		void DrawResolve(core::gpu::CommandBuffer& _cmdBuf);
		void DrawFXAA(core::gpu::CommandBuffer& _cmdBuf);

		void UpdateUniformBuffers();
		void BuildTLAS();
		void RebuildAccelerationStructures();

		void SyncMaterialsAndTextures();

		std::unique_ptr<TextureLibrary>              m_textureLibrary;
		std::unique_ptr<MaterialLibrary>             m_materialLibrary;
		std::unique_ptr<core::gpu::DescriptorSet>    materialDescriptorSet;

		uint32_t RegisterBindlessTexture(const core::gpu::Texture& _texture);

		const core::gpu::Device& m_device;

		std::unique_ptr<core::gpu::Pipeline>		m_gBufferPipeline;
		std::unique_ptr<core::gpu::Pipeline>		m_shadowPipeline;
		std::unique_ptr<core::gpu::Pipeline>		m_giPipeline;
		std::unique_ptr<core::gpu::Pipeline>		m_resolvePipeline;
		std::unique_ptr<core::gpu::Pipeline>		m_fxaaPipeline;

		PassAttachment								m_envMap;
		PassAttachment								m_irradianceMap;

		std::vector<std::pair<Mesh*, glm::mat4>>	m_meshInstances;
		std::unordered_map<Mesh*, glm::mat4>		m_prevMeshInstances[core::gpu::Device::FRAMES_IN_FLIGHT];

		std::unique_ptr<core::gpu::AccelerationStructure> m_fallbackTlas;
		void CreateFallbackTLAS();

		std::vector<std::unique_ptr<core::gpu::AccelerationStructure>>	m_tlasPerFrame;
		std::vector<LightData>											m_lights;

		glm::mat4														m_prevViewProj = glm::mat4(1.0f);
		glm::mat4														m_viewMatrix;
		glm::mat4														m_projMatrix;
		glm::vec3														m_cameraPosition;

		glm::vec3														m_sunDirection = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));

		uint32_t														m_giParity = 0;

		std::vector<std::unique_ptr<core::gpu::Buffer>> lightBuffers;
		static constexpr uint32_t						MAX_LIGHTS = 32;
		std::vector<GPULight>							m_pointLights;

		size_t											m_maxMeshesInTableBuffer = 0;
		std::unique_ptr<core::gpu::Buffer>				m_meshTableBuffer = nullptr;

		static constexpr uint32_t						MAX_MESHES = 256;
		static constexpr uint32_t						MAX_MATERIALS = 512;
		static constexpr uint32_t						MAX_BINDLESS_TEXTURES = 4096;

		uint32_t										m_nextMeshTableIndex = 0;
		uint32_t										m_nextMaterialTableIndex = 0;
		uint32_t										m_nextBindlessTextureIndex = 0;

	};
}


#endif //AQUILA_ENGINE_GRAPHICS_RENDER_RENDERER_H