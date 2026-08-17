#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_MESH_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_MESH_H
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include <graphics/render/material.h>

namespace core::gpu { class AccelerationStructure; class Buffer; class CommandBuffer; class Device; }

namespace graphics::render
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec4 tangent;

		bool operator==(const Vertex& other) const
		{
			return position == other.position &&
				normal == other.normal &&
				uv == other.uv &&
				tangent == other.tangent;
		}
	};

	struct SubMesh
	{
		std::string name;
		uint32_t	firstIndex = 0;
		uint32_t	indexCount = 0;
		uint32_t	vertexOffset = 0;
		uint32_t	materialIndex = 0;
	};

	struct MeshInstance
	{
		std::string				name = "mesh";
		std::string				path = "undefined";

		std::vector<Vertex>		vertices;
		std::vector<uint32_t>	indices;
		std::vector<SubMesh>	subMeshes;
	};

	struct Mesh
	{
		const core::gpu::Device&							device;
		MeshInstance										instance;

		std::unique_ptr<core::gpu::Buffer>					vertexBuffer;
		std::unique_ptr<core::gpu::Buffer>					indexBuffer;
		std::unique_ptr<core::gpu::Buffer>					rtVertexBuffer;
		std::unique_ptr<core::gpu::Buffer>					rtIndexBuffer;
		std::unique_ptr<core::gpu::AccelerationStructure>	blas;

		std::vector<SubMesh>								subMeshes;
		std::vector<Material*>								materials;
		std::vector<std::unique_ptr<Material>>				ownedMaterials;

		uint32_t											meshTableIndex = UINT32_MAX;

		explicit Mesh(const core::gpu::Device& _device, MeshInstance _instance);
		~Mesh() noexcept;

		void CreateBuffers();
		void CreateBLAS();

		void EnsureDefaultSubmesh(); // TODO : Need to know if this is rly important.

		Material* GetMaterial(uint32_t index)
		{
			if (index < materials.size())
				return materials[index];

			return nullptr;
		}
	};
}

namespace std
{
	template<>
	struct hash<graphics::render::Vertex>
	{
		size_t operator()(const graphics::render::Vertex& vertex) const
		{
			size_t h1 = hash<float>()(vertex.position.x);
			size_t h2 = hash<float>()(vertex.position.y);
			size_t h3 = hash<float>()(vertex.position.z);
			size_t h4 = hash<float>()(vertex.normal.x);
			size_t h5 = hash<float>()(vertex.uv.x);
			size_t h6 = hash<float>()(vertex.uv.y);

			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5);
		}
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_MESH_H