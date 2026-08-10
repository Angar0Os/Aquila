#include "descriptorSetLayout_impl.h"
#include "device_impl.h"

#include <core/gpu/utils/converters.h>

using namespace core::gpu;

DescriptorSetLayout::DescriptorSetLayout(const Device& _device, const DescriptorSetLayoutCreateInfo& _info)
	: m_impl(new Impl)
{
	std::vector<vk::DescriptorSetLayoutBinding> vkBindings;
	vkBindings.reserve(_info.bindings.size());

	for (const auto& binding : _info.bindings)
	{
		vk::DescriptorSetLayoutBinding bindings{};
		bindings.binding = binding.binding;
		bindings.descriptorType = utils::ToVulkan(binding.descriptorType);
		bindings.descriptorCount = binding.descriptorCount;
		bindings.stageFlags = utils::ToVulkan(binding.stageFlags);
		bindings.pImmutableSamplers = nullptr;

		vkBindings.push_back(bindings);
	}

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
	layoutInfo.pBindings = vkBindings.data();

	m_impl->layout = vk::raii::DescriptorSetLayout(_device.GetImpl().device, layoutInfo);
}

DescriptorSetLayout::~DescriptorSetLayout() = default;

DescriptorSetLayout::Impl& DescriptorSetLayout::GetImpl() const
{
	return *m_impl;
}