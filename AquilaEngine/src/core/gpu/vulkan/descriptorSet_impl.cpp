#include "descriptorSet_impl.h"
#include "accelerationStructure_impl.h"
#include "buffer_impl.h"
#include "descriptorSetLayout_impl.h"
#include "descriptorPool_impl.h"
#include "device_impl.h"
#include "image_impl.h"
#include "texture_impl.h"

using namespace core::gpu;

DescriptorSet::DescriptorSet(const Device& _device, const DescriptorSetLayout& _dsLayout)
	: m_impl(new Impl)
{
	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = _device.GetImpl().descriptorPool->GetImpl().pool;
	allocInfo.descriptorSetCount = 1;
	vk::DescriptorSetLayout vkLayout = _dsLayout.GetImpl().layout;
	allocInfo.pSetLayouts = &vkLayout;

	auto sets = vk::raii::DescriptorSets(_device.GetImpl().device, allocInfo);
	m_impl->descriptorSet = std::move(sets[0]);

	m_impl->bufferInfos.reserve(8);
	m_impl->imageInfos.reserve(8);
	m_impl->asInfos.reserve(8);
	m_impl->asHandles.reserve(8);
	m_impl->writes.reserve(8);
	m_impl->bindingInfos.reserve(8);
}

DescriptorSet::~DescriptorSet() = default;

template<>
void DescriptorSet::Bind<Texture>(uint32_t binding, const Texture& texture)
{
	size_t infoIndex = m_impl->imageInfos.size();
	m_impl->imageInfos.emplace_back(
		*texture.GetImpl().sampler,
		*texture.GetImpl().image->GetImpl().view,
		vk::ImageLayout::eShaderReadOnlyOptimal
	);

	m_impl->bindingInfos.push_back({
		binding,
		vk::DescriptorType::eCombinedImageSampler,
		infoIndex
		});
}

template<>
void DescriptorSet::Bind<Image>(uint32_t _binding, const Image& _image)
{
	size_t infoIndex = m_impl->imageInfos.size();
	m_impl->imageInfos.emplace_back(
		vk::Sampler{},
		*_image.GetImpl().view,
		vk::ImageLayout::eGeneral
	);

	m_impl->bindingInfos.push_back({
		_binding,
		vk::DescriptorType::eStorageImage,
		infoIndex
		});
}

void DescriptorSet::BindArray(uint32_t binding, uint32_t arrayElement, const Texture& texture)
{
	size_t infoIndex = m_impl->imageInfos.size();
	m_impl->imageInfos.emplace_back(
		*texture.GetImpl().sampler,
		*texture.GetImpl().image->GetImpl().view,
		vk::ImageLayout::eShaderReadOnlyOptimal
	);

	m_impl->bindingInfos.push_back({
		binding,
		vk::DescriptorType::eCombinedImageSampler,
		infoIndex,
		arrayElement 
		});
}

template<>
void core::gpu::DescriptorSet::Bind<core::gpu::Buffer>(uint32_t _binding, const Buffer& _buffer)
{
	size_t infoIndex = m_impl->bufferInfos.size();
	m_impl->bufferInfos.emplace_back(
		_buffer.GetImpl().buffer,
		0,
		_buffer.GetImpl().bufferSize
	);

	vk::DescriptorType descriptorType = vk::DescriptorType::eUniformBuffer;

	auto usage = _buffer.GetImpl().usage;
	if ((usage & utils::EBufferUsage::StorageBuffer) == utils::EBufferUsage::StorageBuffer)
	{
		descriptorType = vk::DescriptorType::eStorageBuffer;
	}
	else if ((usage & utils::EBufferUsage::UniformBuffer) == utils::EBufferUsage::UniformBuffer)
	{
		descriptorType = vk::DescriptorType::eUniformBuffer;
	}

	m_impl->bindingInfos.push_back({
		_binding,
		descriptorType,
		infoIndex
		});
}

template<>
void DescriptorSet::Bind<AccelerationStructure>(uint32_t _binding, const AccelerationStructure& _accelStructure)
{
	vk::AccelerationStructureKHR handle = static_cast<vk::AccelerationStructureKHR>(*_accelStructure.GetImpl().accelerationStructure);

	m_impl->asHandles.push_back(handle);

	vk::WriteDescriptorSetAccelerationStructureKHR asWrite{};
	asWrite.sType = vk::StructureType::eWriteDescriptorSetAccelerationStructureKHR;
	asWrite.accelerationStructureCount = 1;
	asWrite.pAccelerationStructures = &m_impl->asHandles.back();

	size_t infoIndex = m_impl->asInfos.size();
	m_impl->asInfos.push_back(asWrite);

	m_impl->bindingInfos.push_back({
		_binding,
		vk::DescriptorType::eAccelerationStructureKHR,
		infoIndex
		});
}

void DescriptorSet::Update(const Device& device)
{
	if (m_impl->bindingInfos.empty())
		return;

	std::vector<vk::WriteDescriptorSet> writes;
	writes.reserve(m_impl->bindingInfos.size());

	for (const auto& bindingInfo : m_impl->bindingInfos)
	{
		vk::WriteDescriptorSet write{};
		write.dstSet = m_impl->descriptorSet;
		write.dstBinding = bindingInfo.binding;
		write.dstArrayElement = bindingInfo.arrayElement;
		write.descriptorCount = 1;
		write.descriptorType = bindingInfo.type;

		switch (bindingInfo.type)
		{
		case vk::DescriptorType::eCombinedImageSampler:
		case vk::DescriptorType::eStorageImage:
			write.pImageInfo = &m_impl->imageInfos[bindingInfo.infoIndex];
			break;
		case vk::DescriptorType::eUniformBuffer:
		case vk::DescriptorType::eStorageBuffer:
			write.pBufferInfo = &m_impl->bufferInfos[bindingInfo.infoIndex];
			break;
		case vk::DescriptorType::eAccelerationStructureKHR:
			write.pNext = &m_impl->asInfos[bindingInfo.infoIndex];
			break;
		}

		writes.push_back(write);
	}

	device.GetImpl().device.updateDescriptorSets(writes, {});

	m_impl->imageInfos.clear();
	m_impl->bufferInfos.clear();
	m_impl->asInfos.clear();
	m_impl->asHandles.clear();
	m_impl->bindingInfos.clear();
}

DescriptorSet::Impl& DescriptorSet::GetImpl() const
{
	return *m_impl;
}