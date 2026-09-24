#include "descriptorSetLayout_impl.h"
#include "device_impl.h"

#include <core/gpu/utils/converters.h>

using namespace core::gpu;

DescriptorSetLayout::DescriptorSetLayout(const Device& _device, const DescriptorSetLayoutCreateInfo& _info)
	: m_impl(new Impl)
{
	std::vector<vk::DescriptorSetLayoutBinding> vkBindings;
	vkBindings.reserve(_info.bindings.size());

	std::vector<vk::DescriptorBindingFlags> bindingFlags;
	bindingFlags.reserve(_info.bindings.size());

	bool anyUpdateAfterBind = false;
	bool anySpecialFlags = false;

	for (const auto& binding : _info.bindings)
	{
		vk::DescriptorSetLayoutBinding bindings{};
		bindings.binding = binding.binding;
		bindings.descriptorType = utils::ToVulkan(binding.descriptorType);
		bindings.descriptorCount = binding.descriptorCount;
		bindings.stageFlags = utils::ToVulkan(binding.stageFlags);
		bindings.pImmutableSamplers = nullptr;

		vkBindings.push_back(bindings);

		vk::DescriptorBindingFlags flags{};
		if (binding.partiallyBound)
		{
			flags |= vk::DescriptorBindingFlagBits::ePartiallyBound;
			anySpecialFlags = true;
		}
		if (binding.updateAfterBind)
		{
			flags |= vk::DescriptorBindingFlagBits::eUpdateAfterBind;
			anySpecialFlags = true;
			anyUpdateAfterBind = true;
		}

		bindingFlags.push_back(flags);
	}

	vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
	bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
	bindingFlagsInfo.pBindingFlags = bindingFlags.data();

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
	layoutInfo.pBindings = vkBindings.data();

	if (anyUpdateAfterBind)
	{
		layoutInfo.flags |= vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
	}

	if (anySpecialFlags)
	{
		layoutInfo.pNext = &bindingFlagsInfo;
	}

	m_impl->layout = vk::raii::DescriptorSetLayout(_device.GetImpl().device, layoutInfo);
}

DescriptorSetLayout::~DescriptorSetLayout() = default;

DescriptorSetLayout::Impl& DescriptorSetLayout::GetImpl() const
{
	return *m_impl;
}