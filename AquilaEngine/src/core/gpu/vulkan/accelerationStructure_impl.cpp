#include "accelerationStructure_impl.h"
#include "buffer_impl.h"
#include "commandBuffer_impl.h"
#include "device_impl.h"


using namespace core::gpu;

AccelerationStructure::AccelerationStructure(const Device& _device, const AccelerationStructureCreateInfo& _info)
	: m_impl(new Impl)
{
	m_impl->type = _info.type;
	m_impl->geometries = _info.geometries;
	m_impl->instances = _info.instances;

	if (_info.preferFastTrace)
		m_impl->buildFlags |= vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
	if (_info.allowUpdate)
		m_impl->buildFlags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;

	if (_info.type == utils::EAccelerationStructureType::BottomLevel)
		CreateBottomLevel(_device, _info);
	else
		CreateTopLevel(_device, _info);

	Build(_device);
}

core::gpu::AccelerationStructure::~AccelerationStructure() = default;

void AccelerationStructure::CreateAccelerationStructureBuffer(const Device& _device, uint32_t _size)
{
	BufferCreateInfo bufferInfo
	{
		.size = _size,
		.usage = utils::EBufferUsage::AccelerationStructureStorage | utils::EBufferUsage::ShaderDeviceAddress,
		.memoryProperties = utils::EMemoryProperty::DeviceLocal
	};

	m_impl->buffer = std::make_unique<Buffer>(_device, bufferInfo);
}

void AccelerationStructure::CreateScratchBuffer(const Device& _device, uint32_t _size)
{
	auto chain = _device.GetImpl().physicalDevice
		.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();

	const auto& accelProps = chain.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();

	vk::DeviceSize alignment = accelProps.minAccelerationStructureScratchOffsetAlignment;
	if (alignment == 0) alignment = 256;

	vk::DeviceSize alignedSize = (_size + alignment - 1) & ~(alignment - 1);

	BufferCreateInfo bufferInfo{
		.size = static_cast<size_t>(alignedSize),
		.usage = utils::EBufferUsage::StorageBuffer | utils::EBufferUsage::ShaderDeviceAddress,
		.memoryProperties = utils::EMemoryProperty::DeviceLocal
	};

	m_impl->scratchBuffer = std::make_unique<Buffer>(_device, bufferInfo);

	VkDeviceAddress scratchAddr = m_impl->scratchBuffer->GetDeviceAddress();
	if (scratchAddr == 0) {
		throw std::runtime_error("Scratch buffer device address is 0");
	}
}

void AccelerationStructure::CreateBottomLevel(const Device& _device, const AccelerationStructureCreateInfo& _info)
{
	if (m_impl->geometries.empty())
		throw std::runtime_error("BLAS requires at least one geometry");

	std::vector<vk::AccelerationStructureGeometryKHR> vkGeometries;
	std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRanges;
	std::vector<uint32_t> maxPrimitiveCounts;

	vkGeometries.reserve(m_impl->geometries.size());
	buildRanges.reserve(m_impl->geometries.size());
	maxPrimitiveCounts.reserve(m_impl->geometries.size());

	std::vector<vk::AccelerationStructureGeometryTrianglesDataKHR> trianglesDatas;
	trianglesDatas.reserve(m_impl->geometries.size());

	for (const auto& geom : m_impl->geometries)
	{
		if (!geom.vertexBuffer)
			throw std::runtime_error("Geometry missing vertex buffer");

		vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData{};
		trianglesData.vertexFormat = vk::Format::eR32G32B32Sfloat;
		trianglesData.vertexData.deviceAddress = geom.vertexBuffer->GetDeviceAddress();
		trianglesData.vertexStride = geom.vertexStride ? geom.vertexStride : sizeof(float) * 3;
		trianglesData.maxVertex = (geom.vertexCount > 0) ? (geom.vertexCount - 1) : 0;

		if (geom.indexBuffer && geom.indexCount > 0)
		{
			trianglesData.indexType = vk::IndexType::eUint32;
			trianglesData.indexData.deviceAddress = geom.indexBuffer->GetDeviceAddress();
		}
		else
		{
			trianglesData.indexType = vk::IndexType::eNoneKHR;
		}

		if (geom.transformBuffer)
		{
			trianglesData.transformData.deviceAddress = geom.transformBuffer->GetDeviceAddress();
		}

		trianglesDatas.push_back(trianglesData);

		vk::AccelerationStructureGeometryKHR geometry{};
		geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
		geometry.geometry.setTriangles(trianglesDatas.back());
		geometry.flags = geom.opaque ? vk::GeometryFlagBitsKHR::eOpaque : vk::GeometryFlagsKHR{};

		vkGeometries.push_back(geometry);

		vk::AccelerationStructureBuildRangeInfoKHR rangeInfo{};
		rangeInfo.primitiveCount = geom.triangleCount;
		rangeInfo.primitiveOffset = 0;
		rangeInfo.firstVertex = 0;
		rangeInfo.transformOffset = 0;
		buildRanges.push_back(rangeInfo);

		maxPrimitiveCounts.push_back(geom.triangleCount);
	}

	vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
	buildInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
	buildInfo.setFlags(m_impl->buildFlags);
	buildInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
	buildInfo.setGeometryCount(static_cast<uint32_t>(vkGeometries.size()));
	buildInfo.setPGeometries(vkGeometries.data());

	m_impl->buildSizes = _device.GetImpl().device.getAccelerationStructureBuildSizesKHR(
		vk::AccelerationStructureBuildTypeKHR::eDevice,
		buildInfo,
		maxPrimitiveCounts
	);

	if (m_impl->buildSizes.accelerationStructureSize == 0)
		throw std::runtime_error("getAccelerationStructureBuildSizesKHR returned zero accelerationStructureSize");

	CreateAccelerationStructureBuffer(_device, m_impl->buildSizes.accelerationStructureSize);
	CreateScratchBuffer(_device, m_impl->buildSizes.buildScratchSize);

	vk::AccelerationStructureCreateInfoKHR createInfo{};
	createInfo.buffer = *m_impl->buffer->GetImpl().buffer;
	createInfo.size = m_impl->buildSizes.accelerationStructureSize;
	createInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

	m_impl->accelerationStructure.emplace(_device.GetImpl().device, createInfo);
}

void AccelerationStructure::CreateTopLevel(const Device& _device, const AccelerationStructureCreateInfo& _info)
{
	uint32_t instanceCount = static_cast<uint32_t>(m_impl->instances.size());
	size_t	bufferInstanceCount = std::max<size_t>(instanceCount, 1);

	BufferCreateInfo instanceBufferInfo{
		.size = sizeof(vk::AccelerationStructureInstanceKHR) * bufferInstanceCount,
		.usage = utils::EBufferUsage::AccelerationStructureBuildInput | utils::EBufferUsage::ShaderDeviceAddress,
		.memoryProperties = utils::EMemoryProperty::HostVisible | utils::EMemoryProperty::HostCoherent
	};

	m_impl->instanceBuffer = std::make_unique<Buffer>(_device, instanceBufferInfo);

	std::vector<vk::AccelerationStructureInstanceKHR> vkInstances;
	vkInstances.reserve(m_impl->instances.size());

	for (const auto& instance : m_impl->instances)
	{
		if (!instance.blas)
			throw std::runtime_error("TLAS instance has null BLAS pointer");

		vk::AccelerationStructureInstanceKHR vkInstance{};
		std::memcpy(&vkInstance.transform, &instance.transform, sizeof(vkInstance.transform));
		vkInstance.instanceCustomIndex = instance.instanceCustomIndex;
		vkInstance.mask = instance.mask;
		vkInstance.instanceShaderBindingTableRecordOffset = instance.instanceShaderBindingTableRecordOffset & 0x00FFFFFF;
		vkInstance.flags = static_cast<VkGeometryInstanceFlagsKHR>(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
		vkInstance.accelerationStructureReference = instance.blas->GetDeviceAddress(_device);

		vkInstances.push_back(vkInstance);
	}

	if (!vkInstances.empty())
	{
		m_impl->instanceBuffer->CopyFrom(vkInstances.data(), sizeof(vk::AccelerationStructureInstanceKHR) * vkInstances.size(), 0);
	}

	vk::AccelerationStructureGeometryInstancesDataKHR instancesData{};
	instancesData.setArrayOfPointers(VK_FALSE);
	instancesData.data.deviceAddress = m_impl->instanceBuffer->GetDeviceAddress();

	if (instancesData.data.deviceAddress == 0)
		throw std::runtime_error("Instance buffer device address is 0");

	vk::AccelerationStructureGeometryKHR geometry{};
	geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
	geometry.geometry.setInstances(instancesData);
	geometry.flags = vk::GeometryFlagsKHR{};

	vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
	buildInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
	buildInfo.setFlags(m_impl->buildFlags);
	buildInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
	buildInfo.setGeometryCount(1);
	buildInfo.setPGeometries(&geometry);

	m_impl->buildSizes = _device.GetImpl().device.getAccelerationStructureBuildSizesKHR(
		vk::AccelerationStructureBuildTypeKHR::eDevice,
		buildInfo,
		{ instanceCount } 
	);

	if (m_impl->buildSizes.accelerationStructureSize == 0)
		throw std::runtime_error("getAccelerationStructureBuildSizesKHR returned zero accelerationStructureSize (TLAS)");

	CreateAccelerationStructureBuffer(_device, m_impl->buildSizes.accelerationStructureSize);
	CreateScratchBuffer(_device, m_impl->buildSizes.buildScratchSize);

	vk::AccelerationStructureCreateInfoKHR createInfo{};
	createInfo.buffer = *m_impl->buffer->GetImpl().buffer;
	createInfo.size = m_impl->buildSizes.accelerationStructureSize;
	createInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;

	m_impl->accelerationStructure.emplace(_device.GetImpl().device, createInfo);
}

void AccelerationStructure::Build(const Device& _device) const
{
	if (!m_impl->accelerationStructure.has_value())
	{
		throw std::runtime_error("Acceleration structure not initialized before Build()");
	}

	if (m_impl->type == utils::EAccelerationStructureType::BottomLevel)
	{
		if (m_impl->geometries.empty())
		{
			throw std::runtime_error("Cannot build BLAS with no geometries");
		}

		std::vector<vk::AccelerationStructureGeometryKHR> vkGeometries;
		std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRanges;
		std::vector<vk::AccelerationStructureGeometryTrianglesDataKHR> trianglesDatas;

		vkGeometries.reserve(m_impl->geometries.size());
		buildRanges.reserve(m_impl->geometries.size());
		trianglesDatas.reserve(m_impl->geometries.size());

		for (const auto& geom : m_impl->geometries)
		{
			if (!geom.vertexBuffer)
			{
				throw std::runtime_error("Geometry has null vertex buffer");
			}

			vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData{};
			trianglesData.vertexFormat = vk::Format::eR32G32B32Sfloat;
			trianglesData.vertexData.deviceAddress = geom.vertexBuffer->GetDeviceAddress();
			trianglesData.vertexStride = geom.vertexStride ? geom.vertexStride : sizeof(float) * 3;
			trianglesData.maxVertex = (geom.vertexCount > 0) ? (geom.vertexCount - 1) : 0;

			if (geom.indexBuffer && geom.indexCount > 0)
			{
				trianglesData.indexType = vk::IndexType::eUint32;
				trianglesData.indexData.deviceAddress = geom.indexBuffer->GetDeviceAddress();
			}
			else
			{
				trianglesData.indexType = vk::IndexType::eNoneKHR;
			}

			if (geom.transformBuffer)
				trianglesData.transformData.deviceAddress = geom.transformBuffer->GetDeviceAddress();

			trianglesDatas.push_back(trianglesData);

			vk::AccelerationStructureGeometryKHR geometry{};
			geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
			geometry.geometry.setTriangles(trianglesDatas.back());
			geometry.flags = geom.opaque ? vk::GeometryFlagBitsKHR::eOpaque : vk::GeometryFlagsKHR{};

			vkGeometries.push_back(geometry);

			vk::AccelerationStructureBuildRangeInfoKHR rangeInfo{};
			rangeInfo.primitiveCount = geom.triangleCount;
			rangeInfo.primitiveOffset = 0;
			rangeInfo.firstVertex = 0;
			rangeInfo.transformOffset = 0;
			buildRanges.push_back(rangeInfo);
		}

		if (!m_impl->scratchBuffer)
		{
			throw std::runtime_error("Scratch buffer not created");
		}

		vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
		buildInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
		buildInfo.setFlags(m_impl->buildFlags);
		buildInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
		buildInfo.setSrcAccelerationStructure(VK_NULL_HANDLE);
		buildInfo.setDstAccelerationStructure(**m_impl->accelerationStructure);
		buildInfo.setGeometryCount(static_cast<uint32_t>(vkGeometries.size()));
		buildInfo.setPGeometries(vkGeometries.data());
		buildInfo.scratchData.deviceAddress = m_impl->scratchBuffer->GetDeviceAddress();

		if (buildInfo.scratchData.deviceAddress == 0)
		{
			throw std::runtime_error("Scratch buffer device address is 0");
		}

		std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> buildRangePtrs;
		buildRangePtrs.reserve(buildRanges.size());
		for (auto& range : buildRanges)
		{
			buildRangePtrs.push_back(&range);
		}

		auto* commandBuffer = _device.AcquireCommandBuffer();
		commandBuffer->Record([&]() {
			commandBuffer->GetImpl().commandBuffer.buildAccelerationStructuresKHR({ buildInfo }, buildRangePtrs);
			});
		commandBuffer->Submit(_device, true);
		_device.ReleaseCommandBuffer(commandBuffer);
	}
	else
	{
		if (!m_impl->instanceBuffer)
		{
			throw std::runtime_error("Instance buffer not created");
		}

		if (!m_impl->scratchBuffer)
		{
			throw std::runtime_error("Scratch buffer not created");
		}

		vk::AccelerationStructureGeometryInstancesDataKHR instancesData{};
		instancesData.setArrayOfPointers(VK_FALSE);
		instancesData.data.deviceAddress = m_impl->instanceBuffer->GetDeviceAddress();

		if (instancesData.data.deviceAddress == 0)
			throw std::runtime_error("Instance buffer device address is 0 (TLAS)");

		vk::AccelerationStructureGeometryKHR geometry{};
		geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
		geometry.geometry.setInstances(instancesData);
		geometry.flags = vk::GeometryFlagsKHR{};

		vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
		buildInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
		buildInfo.setFlags(m_impl->buildFlags);
		buildInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
		buildInfo.setSrcAccelerationStructure(VK_NULL_HANDLE);
		buildInfo.setDstAccelerationStructure(**m_impl->accelerationStructure);
		buildInfo.setGeometryCount(1);
		buildInfo.setPGeometries(&geometry);
		buildInfo.scratchData.deviceAddress = m_impl->scratchBuffer->GetDeviceAddress();

		if (buildInfo.scratchData.deviceAddress == 0)
		{
			throw std::runtime_error("Scratch buffer device address is 0 (TLAS)");
		}

		vk::AccelerationStructureBuildRangeInfoKHR rangeInfo{};
		rangeInfo.primitiveCount = static_cast<uint32_t>(m_impl->instances.size()); // 0 si vide, OK
		rangeInfo.primitiveOffset = 0;
		rangeInfo.firstVertex = 0;
		rangeInfo.transformOffset = 0;

		std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> buildRangePtrs = { &rangeInfo };

		auto* commandBuffer = _device.AcquireCommandBuffer();
		commandBuffer->Record([&]() {
			commandBuffer->GetImpl().commandBuffer.buildAccelerationStructuresKHR({ buildInfo }, buildRangePtrs);
			});
		commandBuffer->Submit(_device, true);
		_device.ReleaseCommandBuffer(commandBuffer);
	}
}

uint64_t AccelerationStructure::GetDeviceAddress(const Device& _device) const
{
	vk::AccelerationStructureDeviceAddressInfoKHR addressInfo{};
	addressInfo.accelerationStructure = **m_impl->accelerationStructure;
	return _device.GetImpl().device.getAccelerationStructureAddressKHR(addressInfo);
}

AccelerationStructure::Impl& core::gpu::AccelerationStructure::GetImpl() const
{
	return *m_impl;
}
