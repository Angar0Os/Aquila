#include "descriptorSetLayout_impl.h"
#include "device_impl.h"

#include <core/gpu/utils/converters.h>

using namespace core::gpu;

DescriptorSetLayout::Impl::Impl(const Device* _device, const DescriptorSetLayoutCreateInfo& _info)
	: layout(nullptr)
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

	layout = vk::raii::DescriptorSetLayout(_device->GetImpl().device, layoutInfo);
}

DescriptorSetLayout::Impl::~Impl() = default;

DescriptorSetLayout::DescriptorSetLayout(const Device* _device, const DescriptorSetLayoutCreateInfo& _info)
{
	// TODO : Removing parent in impl initialization may causes bugs. 
	m_impl = std::make_unique<Impl>(_device, _info);
}

DescriptorSetLayout::Impl& DescriptorSetLayout::GetImpl() const
{
	return *m_impl;
}