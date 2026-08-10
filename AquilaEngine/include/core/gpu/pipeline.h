#ifndef AQUILA_ENGINE_CORE_GPU_PIPELINE_H
#define AQUILA_ENGINE_CORE_GPU_PIPELINE_H
#pragma once

#include <memory>
#include <vector>
#include <string>

#include <core/gpu/utils/enums.h>

namespace core::gpu
{
	class Device;
	class DescriptorSetLayout;

	struct VertexInputBinding
	{
		uint32_t binding;
		uint32_t stride;
		utils::EVertexInputRate inputRate;
	};

	struct VertexInputAttribute
	{
		uint32_t location;
		uint32_t binding;
		utils::ETextureFormat format;
		uint32_t offset;
	};

	struct ShaderStage
	{
		utils::EShaderStageFlags stage;
		std::vector<char> code;
		std::string entryPoint = "main";
	};

	struct PushConstantRange
	{
		uint32_t stageFlags;
		uint32_t offset;
		uint32_t size;
	};

	struct PipelineCreateInfo
	{
		std::vector<ShaderStage> shaderStages;
		std::vector<VertexInputBinding> vertexBindings;
		std::vector<VertexInputAttribute> vertexAttributes;
		
		utils::EPipelineType		pipelineType;
		utils::EPrimitiveTopology	topology;

		utils::EPolygonMode polygonMode;
		utils::ECullMode cullMode;

		utils::EFrontFace frontFace;

		utils::ECompareOp depthCompareOp;

		bool depthTestEnable;
		bool depthWriteEnable;
		bool blendEnable;

		utils::ESampleCount samples;

		utils::ETextureFormat depthAttachmentFormat;

		std::vector<utils::ETextureFormat> colorAttachmentFormats;
		std::vector<DescriptorSetLayout*> descriptorSetLayouts;
		std::vector<PushConstantRange> pushConstantRanges;
		std::vector<utils::EDynamicState> dynamicStates;
	};

	class Pipeline
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	public:
		Pipeline(const core::gpu::Device& device, const PipelineCreateInfo& info);
		~Pipeline();

		Impl& GetImpl() const;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_PIPELINE_H
