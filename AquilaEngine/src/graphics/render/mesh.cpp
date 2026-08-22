#include <graphics/render/mesh.h>

#include <core/gpu/accelerationStructure.h>
#include <core/gpu/buffer.h>
#include <core/gpu/commandBuffer.h>
#include <core/gpu/device.h>

#include <core/gpu/utils/enums.h>

#include <iostream>
#include <vector>

using namespace graphics::render;


Mesh::Mesh(const core::gpu::Device& _device, MeshInstance _instance)
	: device(_device)
	, instance(std::move(_instance))
{
	CreateBuffers();
	EnsureDefaultSubmesh();
	CreateBLAS();
}

Mesh::~Mesh() noexcept = default;

void Mesh::CreateBuffers()
{
	auto cmdBuf = device.AcquireCommandBuffer();

	if (instance.vertices.empty() || instance.indices.empty())
	{
		std::cerr << "ERROR : Invalid _instance infos. Vertices or Indices are empty ! " << std::endl;
	}

	auto vertexBufferSize = static_cast<uint32_t>(instance.vertices.size() * sizeof(Vertex));
	auto indicesBufferSize = static_cast<uint32_t>(instance.indices.size() * sizeof(uint32_t));

	core::gpu::BufferCreateInfo stagingVertexInfo{
		.size = vertexBufferSize,
		.usage = core::gpu::utils::EBufferUsage::TransferSrc,
		.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
	};

	auto stagingVertexBuffer = std::make_unique<core::gpu::Buffer>(device, stagingVertexInfo);

	core::gpu::BufferCreateInfo stagingIndexInfo{
		.size = indicesBufferSize,
		.usage = core::gpu::utils::EBufferUsage::TransferSrc,
		.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
	};

	auto stagingIndexBuffer = std::make_unique<core::gpu::Buffer>(device, stagingIndexInfo);

	stagingVertexBuffer->CopyFrom(instance.vertices.data(), vertexBufferSize);
	stagingIndexBuffer->CopyFrom(instance.indices.data(), indicesBufferSize);

	core::gpu::BufferCreateInfo vertexInfo{
		.size = vertexBufferSize,
		.usage = core::gpu::utils::EBufferUsage::VertexBuffer | core::gpu::utils::EBufferUsage::TransferDst | core::gpu::utils::EBufferUsage::ShaderDeviceAddress,
		.memoryProperties = core::gpu::utils::EMemoryProperty::DeviceLocal
	};
	vertexBuffer = std::make_unique<core::gpu::Buffer>(device, vertexInfo);

	core::gpu::BufferCreateInfo indexInfo{
		.size = indicesBufferSize,
		.usage = core::gpu::utils::EBufferUsage::IndexBuffer | core::gpu::utils::EBufferUsage::TransferDst | core::gpu::utils::EBufferUsage::ShaderDeviceAddress,
		.memoryProperties = core::gpu::utils::EMemoryProperty::DeviceLocal
	};
	indexBuffer = std::make_unique<core::gpu::Buffer>(device, indexInfo);

	core::gpu::BufferCreateInfo rtVertexInfo{
		.size = vertexBufferSize,
		.usage = core::gpu::utils::EBufferUsage::AccelerationStructureBuildInput |
								core::gpu::utils::EBufferUsage::ShaderDeviceAddress |
								core::gpu::utils::EBufferUsage::StorageBuffer,
		.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
	};
	rtVertexBuffer = std::make_unique<core::gpu::Buffer>(device, rtVertexInfo);
	rtVertexBuffer->CopyFrom(instance.vertices.data(), vertexBufferSize);

	core::gpu::BufferCreateInfo rtIndexInfo{
		.size = indicesBufferSize,
		.usage = core::gpu::utils::EBufferUsage::AccelerationStructureBuildInput |
								core::gpu::utils::EBufferUsage::ShaderDeviceAddress |
								core::gpu::utils::EBufferUsage::StorageBuffer,
		.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
	};
	rtIndexBuffer = std::make_unique<core::gpu::Buffer>(device, rtIndexInfo);
	rtIndexBuffer->CopyFrom(instance.indices.data(), indicesBufferSize);

	cmdBuf->Record([&]() {
		cmdBuf->CopyBuffer(*stagingVertexBuffer, *vertexBuffer, vertexBufferSize);
		cmdBuf->CopyBuffer(*stagingIndexBuffer, *indexBuffer, indicesBufferSize);
		});
	cmdBuf->Submit(device, true);
	device.ReleaseCommandBuffer(cmdBuf);
}

void Mesh::CreateBLAS()
{
	if (!rtVertexBuffer || !rtIndexBuffer)
	{
		std::cerr << "ERROR : RT buffers not found for BLAS creation !" << std::endl;
		return;
	}

	core::gpu::AccelerationStructureGeometry geometry
	{
		.vertexBuffer = rtVertexBuffer.get(),
		.indexBuffer = rtIndexBuffer.get(),
		.vertexCount = static_cast<uint32_t>(instance.vertices.size()),
		.vertexStride = sizeof(Vertex),
		.indexCount = static_cast<uint32_t>(instance.indices.size()),
		.triangleCount = static_cast<uint32_t>(instance.indices.size() / 3),
		.opaque = true
	};

	core::gpu::AccelerationStructureCreateInfo blasInfo
	{
		.type = core::gpu::utils::EAccelerationStructureType::BottomLevel,
		.geometries = { geometry },
		.allowUpdate = true,
		.preferFastTrace = true
	};

	blas = std::make_unique<core::gpu::AccelerationStructure>(device, blasInfo);

	auto buildCmdBuf = device.AcquireCommandBuffer();
	buildCmdBuf->Record([&]() {
		blas->Build(device);
		});
	buildCmdBuf->Submit(device, true);
	device.ReleaseCommandBuffer(buildCmdBuf);
}

void Mesh::EnsureDefaultSubmesh()
{
	if (!instance.subMeshes.empty())
		return;

	SubMesh defaultSubmesh
	{
		.name = "default",
		.firstIndex = 0,
		.indexCount = static_cast<uint32_t>(instance.indices.size()),
		.vertexOffset = 0,
		.material = INVALID_MATERIAL
	};

	instance.subMeshes.push_back(defaultSubmesh);
}