#include <loaders/meshLoader.h>

#include <graphics/render/mesh.h>

#include <core/gpu/device.h>

#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <iostream>

using namespace loaders;

std::unique_ptr<graphics::render::Mesh> loaders::MeshLoader::LoadGLTF(const core::gpu::Device& _device, const std::string& _path)
{
	std::string ext = std::filesystem::path(_path).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext != ".gltf" && ext != ".glb")
		throw std::runtime_error("Unsupported mesh format: " + _path);

	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err, warn;
	bool               ret = false;

	if (ext == ".gltf")
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, _path);
	else
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, _path);

	if (!warn.empty()) std::cerr << "GLTF Warning: " << warn << "\n";
	if (!err.empty())  std::cerr << "GLTF Error: " << err << "\n";
	if (!ret)          throw std::runtime_error("Failed to load GLTF: " + _path);

	if (model.meshes.empty())
		throw std::runtime_error("GLTF contains no meshes");

	const tinygltf::Mesh& gltfMesh = model.meshes[0];
	if (gltfMesh.primitives.empty())
		throw std::runtime_error("GLTF mesh contains no primitives");

	std::cout << "Loading GLTF mesh '" << gltfMesh.name
		<< "' with " << gltfMesh.primitives.size() << " primitives\n";

	graphics::render::MeshInstance instance;
	instance.path = _path;
	instance.name = gltfMesh.name.empty() ? "mesh" : gltfMesh.name;

	std::unordered_map<graphics::render::Vertex, uint32_t> uniqueVertices{};

	for (size_t primIdx = 0; primIdx < gltfMesh.primitives.size(); ++primIdx)
	{
		const tinygltf::Primitive& prim = gltfMesh.primitives[primIdx];

		uint32_t submeshFirstIndex = static_cast<uint32_t>(instance.indices.size());
		uint32_t submeshVertexOffset = static_cast<uint32_t>(instance.vertices.size());

		auto getFloatVec = [&](const std::string& name, int elementSize) -> std::vector<float>
			{
				auto attrIt = prim.attributes.find(name);
				if (attrIt == prim.attributes.end())
					return {};

				const tinygltf::Accessor& accessor = model.accessors[attrIt->second];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

				const float* ptr = reinterpret_cast<const float*>(
					&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
				return std::vector<float>(ptr, ptr + accessor.count * elementSize);
			};

		std::vector<float> positions = getFloatVec("POSITION", 3);
		std::vector<float> normals = getFloatVec("NORMAL", 3);
		std::vector<float> uvs = getFloatVec("TEXCOORD_0", 2);
		std::vector<float> tangents = getFloatVec("TANGENT", 4);

		if (positions.empty())
			throw std::runtime_error("GLTF primitive has no POSITION attribute: " + _path);

		std::vector<uint32_t> primitiveIndices;
		if (prim.indices >= 0)
		{
			const tinygltf::Accessor& accessor = model.accessors[prim.indices];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			primitiveIndices.resize(accessor.count);

			if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			{
				const uint16_t* src = reinterpret_cast<const uint16_t*>(
					&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
				for (size_t i = 0; i < accessor.count; i++)
					primitiveIndices[i] = static_cast<uint32_t>(src[i]);
			}
			else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
			{
				const uint32_t* src = reinterpret_cast<const uint32_t*>(
					&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
				for (size_t i = 0; i < accessor.count; i++)
					primitiveIndices[i] = src[i];
			}
			else
			{
				throw std::runtime_error("Unsupported index component type in GLTF: " + _path);
			}
		}

		auto buildVertex = [&](size_t idx) -> graphics::render::Vertex
			{
				graphics::render::Vertex v{};
				v.position = { positions[idx * 3 + 0], positions[idx * 3 + 1], positions[idx * 3 + 2] };
				if (!normals.empty())
					v.normal = { normals[idx * 3 + 0], normals[idx * 3 + 1], normals[idx * 3 + 2] };
				if (!uvs.empty())
					v.uv = { uvs[idx * 2 + 0], uvs[idx * 2 + 1] };
				if (!tangents.empty())
					v.tangent = { tangents[idx * 4 + 0], tangents[idx * 4 + 1],
								  tangents[idx * 4 + 2], tangents[idx * 4 + 3] };
				else
					v.tangent = { 1.0f, 1.0f, 1.0f, 1.0f };
				return v;
			};

		if (!primitiveIndices.empty())
		{
			for (auto idx : primitiveIndices)
			{
				graphics::render::Vertex v = buildVertex(idx);

				auto it = uniqueVertices.find(v);
				uint32_t vertexIndex;
				if (it == uniqueVertices.end())
				{
					vertexIndex = static_cast<uint32_t>(instance.vertices.size());
					uniqueVertices[v] = vertexIndex;
					instance.vertices.push_back(v);
				}
				else
				{
					vertexIndex = it->second;
				}

				instance.indices.push_back(vertexIndex);
			}
		}
		else
		{
			for (size_t i = 0; i < positions.size() / 3; i++)
			{
				graphics::render::Vertex v = buildVertex(i);

				auto it = uniqueVertices.find(v);
				uint32_t vertexIndex;
				if (it == uniqueVertices.end())
				{
					vertexIndex = static_cast<uint32_t>(instance.vertices.size());
					uniqueVertices[v] = vertexIndex;
					instance.vertices.push_back(v);
				}
				else
				{
					vertexIndex = it->second;
				}

				instance.indices.push_back(vertexIndex);
			}
		}

		graphics::render::SubMesh submesh;
		submesh.firstIndex = submeshFirstIndex;
		submesh.indexCount = static_cast<uint32_t>(instance.indices.size() - submeshFirstIndex);
		submesh.vertexOffset = 0;
		submesh.materialIndex = (prim.material >= 0) ? static_cast<uint32_t>(prim.material) : 0;
		submesh.name = "primitive_" + std::to_string(primIdx);

		instance.subMeshes.push_back(submesh);
	}

	return std::make_unique<graphics::render::Mesh>(_device, instance);
}