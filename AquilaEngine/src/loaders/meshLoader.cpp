#include <loaders/meshLoader.h>

#include <graphics/render/mesh.h>

#include <core/gpu/device.h>

#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <stdexcept>
#include <filesystem>
#include <iostream>

using namespace loaders;

std::pair<std::unique_ptr<core::gpu::Image>, std::unique_ptr<core::gpu::Texture>> s_LoadTexture(
	const core::gpu::Device& _device,
	const tinygltf::Model& _model,
	int _textureIndex,
	core::gpu::utils::ETextureFormat _format)
{
	if (_textureIndex < 0)
		return { nullptr, nullptr };

	const tinygltf::Texture& tex = _model.textures[_textureIndex];
	if (tex.source < 0)
		return { nullptr, nullptr };

	const tinygltf::Image& img = _model.images[tex.source];

	if (img.image.empty())
	{
		std::cerr << "GLTF texture has no decoded pixel data: " << img.uri << "\n";
		return { nullptr, nullptr };
	}

	const size_t pixelCount = static_cast<size_t>(img.width) * img.height;
	std::vector<uint8_t> rgba;

	if (img.component == 4)
	{
		rgba.assign(img.image.begin(), img.image.end());
	}
	else if (img.component == 3)
	{
		rgba.resize(pixelCount * 4);
		for (size_t p = 0; p < pixelCount; ++p)
		{
			rgba[p * 4 + 0] = img.image[p * 3 + 0];
			rgba[p * 4 + 1] = img.image[p * 3 + 1];
			rgba[p * 4 + 2] = img.image[p * 3 + 2];
			rgba[p * 4 + 3] = 255;
		}
	}
	else
	{
		std::cerr << "Unsupported GLTF image component count (" << img.component << ") for: " << img.uri << "\n";
		return { nullptr, nullptr };
	}

	return graphics::render::Material::CreateTexture(
		_device, rgba.data(),
		static_cast<uint32_t>(img.width),
		static_cast<uint32_t>(img.height),
		_format
	);
}

std::unique_ptr<graphics::render::Material> s_LoadMaterial(
	const core::gpu::Device& _device,
	const core::gpu::DescriptorSetLayout& _materialLayout,
	const tinygltf::Model& _model,
	const tinygltf::Material& _gltfMat)
{
	auto mat = std::make_unique<graphics::render::Material>(_device, _materialLayout);
	mat->name = _gltfMat.name.empty() ? "material" : _gltfMat.name;

	const auto& pbr = _gltfMat.pbrMetallicRoughness;

	mat->albedoColor = glm::vec3(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2]);
	mat->metalness = static_cast<float>(pbr.metallicFactor);
	mat->roughnessValue = static_cast<float>(pbr.roughnessFactor);

	mat->SetBaseColor(glm::vec4(mat->albedoColor, static_cast<float>(pbr.baseColorFactor[3])));
	mat->SetMetalness(mat->metalness);
	mat->SetRoughness(mat->roughnessValue);

	if (auto [img, tex] = s_LoadTexture(_device, _model, pbr.baseColorTexture.index, core::gpu::utils::ETextureFormat::RGBA8_SRGB); tex)
	{
		mat->hasAlbedoTexture = true;
		mat->SetAlbedo(_device, std::move(img), std::move(tex));
	}
	else
	{
		uint8_t pixel[4] = { 255, 255, 255, 255 };

		auto [img2, tex2] = graphics::render::Material::CreateTexture(_device, pixel, 1u, 1u, core::gpu::utils::ETextureFormat::RGBA8_SRGB);

		mat->hasAlbedoTexture = false;
		mat->SetAlbedo(_device, std::move(img2), std::move(tex2));
	}

	if (auto [img, tex] = s_LoadTexture(_device, _model, _gltfMat.normalTexture.index, core::gpu::utils::ETextureFormat::RGBA8_UNorm); tex)
	{
		mat->hasNormalTexture = true;
		mat->SetNormal(_device, std::move(img), std::move(tex));
	}

	if (auto [img, tex] = s_LoadTexture(_device, _model, pbr.metallicRoughnessTexture.index, core::gpu::utils::ETextureFormat::RGBA8_UNorm); tex)
	{
		mat->hasRoughnessMetalTexture = true;
		mat->SetRoughnessMetal(_device, std::move(img), std::move(tex));
	}

	return mat;
}

std::unique_ptr<graphics::render::Mesh> loaders::MeshLoader::LoadGLTF(
	const core::gpu::Device& _device,
	const std::string& _path,
	const core::gpu::DescriptorSetLayout& _materialLayout)
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

	std::cout << "Model has " << model.meshes.size() << " meshes total\n";

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

		auto addVertex = [&](size_t idx)
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
			};

		if (!primitiveIndices.empty())
		{
			for (auto idx : primitiveIndices)
				addVertex(idx);
		}
		else
		{
			for (size_t i = 0; i < positions.size() / 3; i++)
				addVertex(i);
		}

		graphics::render::SubMesh submesh;
		submesh.firstIndex = submeshFirstIndex;
		submesh.indexCount = static_cast<uint32_t>(instance.indices.size() - submeshFirstIndex);
		submesh.vertexOffset = 0;
		submesh.materialIndex = (prim.material >= 0) ? static_cast<uint32_t>(prim.material) : 0;
		submesh.name = "primitive_" + std::to_string(primIdx);

		instance.subMeshes.push_back(submesh);
	}

	auto mesh = std::make_unique<graphics::render::Mesh>(_device, instance);

	mesh->ownedMaterials.reserve(model.materials.size());
	for (const auto& gltfMat : model.materials)
		mesh->ownedMaterials.push_back(s_LoadMaterial(_device, _materialLayout, model, gltfMat));

	mesh->materials.reserve(mesh->ownedMaterials.size());
	for (auto& m : mesh->ownedMaterials)
		mesh->materials.push_back(m.get());

	return mesh;
}

glm::mat4 s_NodeLocalTransform(const tinygltf::Node& _node)
{
	if (_node.matrix.size() == 16)
		return glm::make_mat4(_node.matrix.data());

	glm::mat4 t(1.0f), r(1.0f), s(1.0f);

	if (_node.translation.size() == 3)
		t = glm::translate(glm::mat4(1.0f), glm::vec3(
			_node.translation[0], _node.translation[1], _node.translation[2]));

	if (_node.rotation.size() == 4)
	{
		glm::quat q(
			static_cast<float>(_node.rotation[3]),
			static_cast<float>(_node.rotation[0]),
			static_cast<float>(_node.rotation[1]),
			static_cast<float>(_node.rotation[2]) 
		);
		r = glm::mat4_cast(q);
	}

	if (_node.scale.size() == 3)
		s = glm::scale(glm::mat4(1.0f), glm::vec3(
			_node.scale[0], _node.scale[1], _node.scale[2]));

	return t * r * s;
}

struct MeshNode
{
	int       meshIndex;
	glm::mat4 worldTransform;
};

void s_CollectMeshNodes(
	const tinygltf::Model& _model,
	int _nodeIndex,
	const glm::mat4& _parentTransform,
	std::vector<MeshNode>& _out)
{
	const tinygltf::Node& node = _model.nodes[_nodeIndex];
	glm::mat4 worldTransform = _parentTransform * s_NodeLocalTransform(node);

	if (node.mesh >= 0)
		_out.push_back({ node.mesh, worldTransform });

	for (int child : node.children)
		s_CollectMeshNodes(_model, child, worldTransform, _out);
}

std::unique_ptr<graphics::render::Mesh> s_BuildMesh(
	const core::gpu::Device& _device,
	const tinygltf::Model& _model,
	const tinygltf::Mesh& _gltfMesh,
	const core::gpu::DescriptorSetLayout& _materialLayout,
	const std::string& _path)
{
	graphics::render::MeshInstance instance;
	instance.path = _path;
	instance.name = _gltfMesh.name.empty() ? "mesh" : _gltfMesh.name;

	std::unordered_map<graphics::render::Vertex, uint32_t> uniqueVertices{};
	bool needsFallbackMaterial = false;

	for (size_t primIdx = 0; primIdx < _gltfMesh.primitives.size(); ++primIdx)
	{
		const tinygltf::Primitive& prim = _gltfMesh.primitives[primIdx];

		uint32_t submeshFirstIndex = static_cast<uint32_t>(instance.indices.size());

		auto getFloatVec = [&](const std::string& name, int elementSize) -> std::vector<float>
			{
				auto attrIt = prim.attributes.find(name);
				if (attrIt == prim.attributes.end())
					return {};

				const tinygltf::Accessor& accessor = _model.accessors[attrIt->second];
				const tinygltf::BufferView& bufferView = _model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = _model.buffers[bufferView.buffer];

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
			const tinygltf::Accessor& accessor = _model.accessors[prim.indices];
			const tinygltf::BufferView& bufferView = _model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = _model.buffers[bufferView.buffer];

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

		auto addVertex = [&](size_t idx)
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
			};

		if (!primitiveIndices.empty())
		{
			for (auto idx : primitiveIndices)
				addVertex(idx);
		}
		else
		{
			for (size_t i = 0; i < positions.size() / 3; i++)
				addVertex(i);
		}

		graphics::render::SubMesh submesh;
		submesh.firstIndex = submeshFirstIndex;
		submesh.indexCount = static_cast<uint32_t>(instance.indices.size() - submeshFirstIndex);
		submesh.vertexOffset = 0;
		submesh.name = "primitive_" + std::to_string(primIdx);

		if (prim.material >= 0)
		{
			submesh.materialIndex = static_cast<uint32_t>(prim.material);
		}
		else
		{
			submesh.materialIndex = static_cast<uint32_t>(_model.materials.size());
			needsFallbackMaterial = true;
		}

		instance.subMeshes.push_back(submesh);
	}

	auto mesh = std::make_unique<graphics::render::Mesh>(_device, instance);

	mesh->ownedMaterials.reserve(_model.materials.size() + (needsFallbackMaterial ? 1 : 0));
	for (const auto& gltfMat : _model.materials)
		mesh->ownedMaterials.push_back(s_LoadMaterial(_device, _materialLayout, _model, gltfMat));

	if (needsFallbackMaterial)
		mesh->ownedMaterials.push_back(std::make_unique<graphics::render::Material>(_device, _materialLayout));

	mesh->materials.reserve(mesh->ownedMaterials.size());
	for (auto& m : mesh->ownedMaterials)
		mesh->materials.push_back(m.get());

	return mesh;
}


std::vector<graphics::render::SceneMeshInstance> loaders::MeshLoader::LoadGLTFScene(
	const core::gpu::Device& _device,
	const std::string& _path,
	const core::gpu::DescriptorSetLayout& _materialLayout)
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

	std::vector<MeshNode> meshNodes;

	if (!model.scenes.empty())
	{
		int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;
		for (int rootNode : model.scenes[sceneIndex].nodes)
			s_CollectMeshNodes(model, rootNode, glm::mat4(1.0f), meshNodes);
	}
	else
	{
		for (size_t i = 0; i < model.nodes.size(); ++i)
			s_CollectMeshNodes(model, static_cast<int>(i), glm::mat4(1.0f), meshNodes);
	}

	std::cout << "GLTF scene '" << _path << "' contains " << meshNodes.size() << " mesh node(s)\n";

	std::vector<graphics::render::SceneMeshInstance> result;
	result.reserve(meshNodes.size());

	for (const auto& node : meshNodes)
	{
		const tinygltf::Mesh& gltfMesh = model.meshes[node.meshIndex];
		auto mesh = s_BuildMesh(_device, model, gltfMesh, _materialLayout, _path);

		result.push_back({ std::move(mesh), node.worldTransform });
	}

	return result;
}