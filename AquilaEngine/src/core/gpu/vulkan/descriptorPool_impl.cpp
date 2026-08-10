#include "descriptorPool_impl.h"
#include "device_impl.h"

using namespace core::gpu;

DescriptorPool::DescriptorPool(const Device& device)
	: m_impl(new Impl)
{
	std::vector<vk::DescriptorPoolSize> poolSizes = {
		{ vk::DescriptorType::eUniformBuffer,            1024 },
		{ vk::DescriptorType::eStorageBuffer,            1024 },
		{ vk::DescriptorType::eCombinedImageSampler,     2048 },
		{ vk::DescriptorType::eSampledImage,             1024 },
		{ vk::DescriptorType::eStorageImage,             512  },
		{ vk::DescriptorType::eUniformTexelBuffer,       256  },
		{ vk::DescriptorType::eStorageTexelBuffer,       256  },
		{ vk::DescriptorType::eSampler,                  512  },
		{ vk::DescriptorType::eInputAttachment,          256  },
		{ vk::DescriptorType::eAccelerationStructureKHR, 32   }
	};

	auto createInfo = vk::DescriptorPoolCreateInfo{};

	createInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	createInfo.maxSets = 2048;
	createInfo.poolSizeCount = uint32_t(poolSizes.size());
	createInfo.pPoolSizes = poolSizes.data();

	m_impl->pool = vk::raii::DescriptorPool(device.GetImpl().device, createInfo);
}

DescriptorPool::~DescriptorPool() = default;

DescriptorPool::Impl& DescriptorPool::GetImpl() const
{
	return *m_impl;
}