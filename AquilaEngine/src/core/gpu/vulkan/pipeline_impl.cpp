#include "pipeline_impl.h"
#include "descriptorSetLayout_impl.h"
#include "device_impl.h"

#include <core/gpu/utils/converters.h>

using namespace core::gpu;

Pipeline::Impl::Impl(const Device* device, const PipelineCreateInfo& info)
	: pipelineLayout(nullptr), pipeline(nullptr)
{
	std::vector<vk::raii::ShaderModule> shaderModules;
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos;
	type = info.pipelineType;

	for (const auto& stage : info.shaderStages)
	{
		vk::ShaderModuleCreateInfo moduleInfo{};
		moduleInfo.codeSize = stage.code.size();
		moduleInfo.pCode = reinterpret_cast<const uint32_t*>(stage.code.data());

		shaderModules.emplace_back(device->GetImpl().device, moduleInfo);

		vk::PipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.stage = utils::ToVulkan(stage.stage);
		shaderStageInfo.module = *shaderModules.back();
		shaderStageInfo.pName = stage.entryPoint.c_str();

		shaderStageInfos.push_back(shaderStageInfo);
	}

	std::vector<vk::DescriptorSetLayout> vkLayouts;
	for (auto* layout : info.descriptorSetLayouts)
	{
		vkLayouts.push_back(vk::DescriptorSetLayout(layout->GetImpl().layout));
	}

	std::vector<vk::PushConstantRange> vkPushConstants;
	for (const auto& range : info.pushConstantRanges)
	{
		vk::ShaderStageFlags stageFlags{};

		if (range.stageFlags & static_cast<uint32_t>(utils::EShaderStageFlags::Vertex))
			stageFlags |= vk::ShaderStageFlagBits::eVertex;

		if (range.stageFlags & static_cast<uint32_t>(utils::EShaderStageFlags::Fragment))
			stageFlags |= vk::ShaderStageFlagBits::eFragment;

		if (range.stageFlags & static_cast<uint32_t>(utils::EShaderStageFlags::Compute))
			stageFlags |= vk::ShaderStageFlagBits::eCompute;

		vk::PushConstantRange constRange{};
		constRange.stageFlags = stageFlags;
		constRange.offset = range.offset;
		constRange.size = range.size;

		vkPushConstants.push_back(constRange);
	}

	vk::PipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.setLayoutCount = static_cast<uint32_t>(vkLayouts.size());
	layoutInfo.pSetLayouts = vkLayouts.data();
	layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(vkPushConstants.size());
	layoutInfo.pPushConstantRanges = vkPushConstants.data();

	pipelineLayout = vk::raii::PipelineLayout(device->GetImpl().device, layoutInfo);

	if (std::find_if(info.shaderStages.begin(), info.shaderStages.end(), [](const auto& s) { return s.stage == utils::EShaderStageFlags::Compute; }) != info.shaderStages.end())
	{
		if (info.shaderStages.size() > 1)
		{
			throw std::runtime_error("Compute pipeline must have exactly one shader stage");
		}

		vk::ComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.stage = shaderStageInfos[0];
		pipelineInfo.layout = *pipelineLayout;

		pipeline = vk::raii::Pipeline(device->GetImpl().device, nullptr, pipelineInfo);
	}
	else
	{

		std::vector<vk::VertexInputBindingDescription> vkBindings;
		for (const auto& binding : info.vertexBindings)
		{
			vkBindings.push_back({
				binding.binding,
				binding.stride,
				utils::ToVulkan(binding.inputRate)
				});
		}

		std::vector<vk::VertexInputAttributeDescription> vkAttributes;
		for (const auto& attr : info.vertexAttributes)
		{
			vkAttributes.push_back({
				attr.location,
				attr.binding,
				utils::ToVulkan(attr.format),
				attr.offset
				});
		}

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vkBindings.size());
		vertexInputInfo.pVertexBindingDescriptions = vkBindings.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vkAttributes.size());
		vertexInputInfo.pVertexAttributeDescriptions = vkAttributes.data();

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.topology = utils::ToVulkan(info.topology);
		inputAssembly.primitiveRestartEnable = vk::False;

		vk::PipelineViewportStateCreateInfo viewportState{};
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		vk::PipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.depthClampEnable = vk::False;
		rasterizer.rasterizerDiscardEnable = vk::False;
		rasterizer.polygonMode = utils::ToVulkan(info.polygonMode);
		rasterizer.cullMode = utils::ToVulkan(info.cullMode);
		rasterizer.frontFace = utils::ToVulkan(info.frontFace);
		rasterizer.depthBiasEnable = vk::False;
		rasterizer.lineWidth = 1.0f;

		vk::PipelineMultisampleStateCreateInfo multisampling{};
		multisampling.rasterizationSamples = utils::ToVulkan(info.samples);
		multisampling.sampleShadingEnable = vk::False;

		vk::PipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.depthTestEnable = info.depthTestEnable ? vk::True : vk::False;
		depthStencil.depthWriteEnable = info.depthWriteEnable ? vk::True : vk::False;
		depthStencil.depthCompareOp = utils::ToVulkan(info.depthCompareOp);
		depthStencil.depthBoundsTestEnable = vk::False;
		depthStencil.stencilTestEnable = vk::False;

		vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = info.blendEnable ? vk::True : vk::False;
		colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

		uint32_t colorAttachmentCount = static_cast<uint32_t>(
			info.colorAttachmentFormats.empty() ? 1 : info.colorAttachmentFormats.size());

		std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments(
			colorAttachmentCount, colorBlendAttachment);

		vk::PipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.logicOpEnable = vk::False;
		colorBlending.attachmentCount = colorAttachmentCount;
		colorBlending.pAttachments = colorBlendAttachments.data();

		std::vector<vk::DynamicState> vkDynamicStates;
		for (const auto& state : info.dynamicStates)
		{
			vkDynamicStates.push_back(utils::ToVulkan(state));
		}

		vk::PipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.dynamicStateCount = static_cast<uint32_t>(vkDynamicStates.size());
		dynamicState.pDynamicStates = vkDynamicStates.data();

		std::vector<vk::Format> vkColorFormats;
		for (const auto& format : info.colorAttachmentFormats)
		{
			vkColorFormats.push_back(utils::ToVulkan(format));
		}

		vk::PipelineRenderingCreateInfo renderingInfo{};
		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(vkColorFormats.size());
		renderingInfo.pColorAttachmentFormats = vkColorFormats.data();
		renderingInfo.depthAttachmentFormat = utils::ToVulkan(info.depthAttachmentFormat);


		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.pNext = &renderingInfo;
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStageInfos.size());
		pipelineInfo.pStages = shaderStageInfos.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = *pipelineLayout;
		pipelineInfo.renderPass = nullptr;

		pipeline = vk::raii::Pipeline(device->GetImpl().device, nullptr, pipelineInfo);
	}
}

Pipeline::Impl::~Impl() = default;

Pipeline::Pipeline(const Device* device, const PipelineCreateInfo& info)
{
	m_impl = std::make_unique<Impl>(device, info);
}

Pipeline::~Pipeline() = default;

Pipeline::Impl& core::gpu::Pipeline::GetImpl() const
{
	return *m_impl;
}