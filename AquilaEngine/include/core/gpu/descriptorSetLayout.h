#ifndef AQUILA_ENGINE_CORE_GPU_DESCRIPTOR_SET_LAYOUT_H
#define AQUILA_ENGINE_CORE_GPU_DESCRIPTOR_SET_LAYOUT_H
#pragma once

#include <memory>
#include <vector>

#include <core/gpu/utils/enums.h>

namespace core::gpu
{
	class Device;

	struct DescriptorSetLayoutBinding
	{
		uint32_t binding;
		uint32_t descriptorCount = 1;
		utils::EDescriptorType descriptorType;
		utils::EShaderStage stageFlags;
	};

	struct DescriptorSetLayoutCreateInfo
	{
		std::vector<DescriptorSetLayoutBinding> bindings;
	};

	class DescriptorSetLayout
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;

	public:
		DescriptorSetLayout(const Device& _device, const DescriptorSetLayoutCreateInfo& _info);
		~DescriptorSetLayout();

		Impl& GetImpl() const;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_DESCRIPTOR_SET_LAYOUT_H
