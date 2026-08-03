#include "descriptorPool_impl.h"
#include "device_impl.h"

using namespace core::gpu;

core::gpu::DescriptorPool::Impl::Impl(DescriptorPool& _pool,
	const Device* _device)
	: device(_device), pool(nullptr)
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

	pool = vk::raii::DescriptorPool(device->GetImpl().device, createInfo);
}

DescriptorPool::Impl::~Impl() = default;

DescriptorPool::DescriptorPool(const Device* device)
{
	m_impl = std::make_unique<Impl>(*this, device);
}

DescriptorPool::~DescriptorPool() = default;

DescriptorPool::Impl& DescriptorPool::GetImpl() const
{
	return *m_impl;
}