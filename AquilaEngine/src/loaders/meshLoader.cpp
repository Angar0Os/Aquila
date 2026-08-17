#include <loaders/meshLoader.h>

#include <graphics/render/mesh.h>
#include <graphics/render/material.h>
#include <graphics/render/materialLibrary.h>
#include <graphics/render/textureLibrary.h>

#include <core/gpu/device.h>

#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

using namespace loaders;
using namespace graphics::render;

TextureHandle s_LoadTexture(
    const tinygltf::Model& _model,
    int _textureIndex,
    TextureLibrary& _textureLibrary,
    ETextureSemantic _semantic,
    const std::string& _path)
{
    if (_textureIndex < 0)
        return INVALID_TEXTURE;

    if (_textureIndex >= static_cast<int>(_model.textures.size()))
    {
        std::cerr << "Invalid GLTF texture index: " << _textureIndex << "\n";
        return INVALID_TEXTURE;
    }

    const tinygltf::Texture& texture = _model.textures[_textureIndex];

    if (texture.source < 0)
        return INVALID_TEXTURE;

    if (texture.source >= static_cast<int>(_model.images.size()))
    {
        std::cerr << "Invalid GLTF image index: " << texture.source << "\n";
        return INVALID_TEXTURE;
    }

    const tinygltf::Image& image = _model.images[texture.source];

    if (image.image.empty())
    {
        std::cerr << "GLTF texture has no decoded pixel data: " << image.uri << "\n";
        return INVALID_TEXTURE;
    }

    const size_t pixelCount = static_cast<size_t>(image.width) * static_cast<size_t>(image.height);
    std::vector<uint8_t> rgba;

    if (image.component == 4)
    {
        rgba.assign(image.image.begin(), image.image.end());
    }
    else if (image.component == 3)
    {
        rgba.resize(pixelCount * 4);

        for (size_t p = 0; p < pixelCount; ++p)
        {
            rgba[p * 4 + 0] = image.image[p * 3 + 0];
            rgba[p * 4 + 1] = image.image[p * 3 + 1];
            rgba[p * 4 + 2] = image.image[p * 3 + 2];
            rgba[p * 4 + 3] = 255;
        }
    }
    else
    {
        std::cerr << "Unsupported GLTF image component count (" << image.component << ") for: " << image.uri << "\n";
        return INVALID_TEXTURE;
    }

    std::string textureName;

    if (!image.uri.empty())
    {
        textureName = _path + "::" + image.uri;
    }
    else
    {
        textureName = _path + "::gltf_image_" + std::to_string(texture.source);
    }

    if (auto existing = _textureLibrary.Find(textureName); existing != INVALID_TEXTURE)
        return existing;

    return _textureLibrary.AddFromPixels(
        textureName,
        rgba.data(),
        static_cast<uint32_t>(image.width),
        static_cast<uint32_t>(image.height),
        _semantic
    );
}

MaterialHandle s_LoadMaterial(
    const tinygltf::Model& _model,
    const tinygltf::Material& _gltfMat,
    TextureLibrary& _textureLibrary,
    MaterialLibrary& _materialLibrary,
    const std::string& _path,
    int _materialIndex)
{
    Material material;

    material.name = _path + "::" + (_gltfMat.name.empty()
        ? ("material_" + std::to_string(_materialIndex))
        : _gltfMat.name);

    const auto& pbr = _gltfMat.pbrMetallicRoughness;

    material.baseColor.value = glm::vec4(
        static_cast<float>(pbr.baseColorFactor[0]),
        static_cast<float>(pbr.baseColorFactor[1]),
        static_cast<float>(pbr.baseColorFactor[2]),
        static_cast<float>(pbr.baseColorFactor[3])
    );

    material.metallic.value = static_cast<float>(pbr.metallicFactor);
    material.roughness.value = static_cast<float>(pbr.roughnessFactor);

    if (pbr.baseColorTexture.index >= 0)
    {
        material.baseColor.texture = s_LoadTexture(
            _model,
            pbr.baseColorTexture.index,
            _textureLibrary,
            ETextureSemantic::BaseColor,
            _path
        );
    }

    if (_gltfMat.normalTexture.index >= 0)
    {
        material.normal.texture = s_LoadTexture(
            _model,
            _gltfMat.normalTexture.index,
            _textureLibrary,
            ETextureSemantic::Normal,
            _path
        );
    }

    if (pbr.metallicRoughnessTexture.index >= 0)
    {
        material.ormTexture = s_LoadTexture(
            _model,
            pbr.metallicRoughnessTexture.index,
            _textureLibrary,
            ETextureSemantic::ORM,
            _path
        );
    }

    if (_gltfMat.emissiveTexture.index >= 0)
    {
        material.emissive.texture = s_LoadTexture(
            _model,
            _gltfMat.emissiveTexture.index,
            _textureLibrary,
            ETextureSemantic::Emissive,
            _path
        );
    }

    material.emissive.value = glm::vec3(
        static_cast<float>(_gltfMat.emissiveFactor[0]),
        static_cast<float>(_gltfMat.emissiveFactor[1]),
        static_cast<float>(_gltfMat.emissiveFactor[2])
    );

    const std::string materialName = material.name;

    std::cout << "  material '" << material.name << "' baseColorFactor=("
        << pbr.baseColorFactor[0] << ", " << pbr.baseColorFactor[1] << ", "
        << pbr.baseColorFactor[2] << ", " << pbr.baseColorFactor[3] << ")\n";

    return _materialLibrary.Add(materialName, std::move(material));
}

std::unique_ptr<Mesh> s_BuildMesh(
    const core::gpu::Device& _device,
    const tinygltf::Model& _model,
    const tinygltf::Mesh& _gltfMesh,
    TextureLibrary& _textureLibrary,
    MaterialLibrary& _materialLibrary,
    const std::string& _path)
{

    MeshInstance instance;
    instance.path = _path;
    instance.name = _gltfMesh.name.empty() ? "mesh" : _gltfMesh.name;

    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    for (size_t primIdx = 0; primIdx < _gltfMesh.primitives.size(); ++primIdx)
    {
        const tinygltf::Primitive& prim = _gltfMesh.primitives[primIdx];

        const uint32_t submeshFirstIndex = static_cast<uint32_t>(instance.indices.size());

        auto getFloatVec = [&](const std::string& name, int elementSize) -> std::vector<float>
            {
                auto attrIt = prim.attributes.find(name);

                if (attrIt == prim.attributes.end())
                    return {};

                const tinygltf::Accessor& accessor = _model.accessors[attrIt->second];
                const tinygltf::BufferView& bufferView = _model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = _model.buffers[bufferView.buffer];

                const size_t offset = bufferView.byteOffset + accessor.byteOffset;

                const float* ptr = reinterpret_cast<const float*>(&buffer.data[offset]);

                return std::vector<float>(
                    ptr,
                    ptr + accessor.count * elementSize
                );
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

            const size_t offset = bufferView.byteOffset + accessor.byteOffset;

            primitiveIndices.resize(accessor.count);

            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                const uint8_t* src = reinterpret_cast<const uint8_t*>(&buffer.data[offset]);

                for (size_t i = 0; i < accessor.count; ++i)
                    primitiveIndices[i] = static_cast<uint32_t>(src[i]);
            }
            else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                const uint16_t* src = reinterpret_cast<const uint16_t*>(&buffer.data[offset]);

                for (size_t i = 0; i < accessor.count; ++i)
                    primitiveIndices[i] = static_cast<uint32_t>(src[i]);
            }
            else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                const uint32_t* src = reinterpret_cast<const uint32_t*>(&buffer.data[offset]);

                for (size_t i = 0; i < accessor.count; ++i)
                    primitiveIndices[i] = src[i];
            }
            else
            {
                throw std::runtime_error("Unsupported index component type in GLTF: " + _path);
            }
        }

        auto buildVertex = [&](size_t idx) -> Vertex
            {
                Vertex vertex{};

                vertex.position = {
                    positions[idx * 3 + 0],
                    positions[idx * 3 + 1],
                    positions[idx * 3 + 2]
                };

                if (!normals.empty())
                {
                    vertex.normal = {
                        normals[idx * 3 + 0],
                        normals[idx * 3 + 1],
                        normals[idx * 3 + 2]
                    };
                }

                if (!uvs.empty())
                {
                    vertex.uv = {
                        uvs[idx * 2 + 0],
                        uvs[idx * 2 + 1]
                    };
                }

                if (!tangents.empty())
                {
                    vertex.tangent = {
                        tangents[idx * 4 + 0],
                        tangents[idx * 4 + 1],
                        tangents[idx * 4 + 2],
                        tangents[idx * 4 + 3]
                    };
                }
                else
                {
                    vertex.tangent = {
                        1.0f,
                        0.0f,
                        0.0f,
                        1.0f
                    };
                }

                return vertex;
            };

        auto addVertex = [&](size_t idx)
            {
                Vertex vertex = buildVertex(idx);

                auto it = uniqueVertices.find(vertex);

                uint32_t vertexIndex;

                if (it == uniqueVertices.end())
                {
                    vertexIndex = static_cast<uint32_t>(instance.vertices.size());

                    uniqueVertices.emplace(vertex, vertexIndex);
                    instance.vertices.push_back(vertex);
                }
                else
                {
                    vertexIndex = it->second;
                }

                instance.indices.push_back(vertexIndex);
            };

        if (!primitiveIndices.empty())
        {
            for (uint32_t index : primitiveIndices)
                addVertex(index);
        }
        else
        {
            for (size_t i = 0; i < positions.size() / 3; ++i)
                addVertex(i);
        }

        SubMesh submesh;

        submesh.name = "primitive_" + std::to_string(primIdx);
        submesh.firstIndex = submeshFirstIndex;
        submesh.indexCount = static_cast<uint32_t>(instance.indices.size()) - submeshFirstIndex;
        submesh.vertexOffset = 0;

        if (prim.material >= 0 && prim.material < static_cast<int>(_model.materials.size()))
        {
            submesh.material = s_LoadMaterial(
                _model,
                _model.materials[prim.material],
                _textureLibrary,
                _materialLibrary,
                _path,
                prim.material
            );
        }
        else
        {
            submesh.material = _materialLibrary.GetDefaultMaterial();
        }

        instance.subMeshes.push_back(submesh);
        std::cout << "  submesh '" << submesh.name << "' -> materialHandle=" << submesh.material
            << (submesh.material == _materialLibrary.GetDefaultMaterial() ? " (DEFAULT/FALLBACK)" : " (assigned)")
            << "\n";
    }

    return std::make_unique<Mesh>(
        _device,
        std::move(instance)
    );
}

glm::mat4 s_NodeLocalTransform(const tinygltf::Node& _node)
{
    if (_node.matrix.size() == 16)
        return glm::make_mat4(_node.matrix.data());

    glm::mat4 t(1.0f);
    glm::mat4 r(1.0f);
    glm::mat4 s(1.0f);

    if (_node.translation.size() == 3)
    {
        t = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(
                static_cast<float>(_node.translation[0]),
                static_cast<float>(_node.translation[1]),
                static_cast<float>(_node.translation[2])
            )
        );
    }

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
    {
        s = glm::scale(
            glm::mat4(1.0f),
            glm::vec3(
                static_cast<float>(_node.scale[0]),
                static_cast<float>(_node.scale[1]),
                static_cast<float>(_node.scale[2])
            )
        );
    }

    return t * r * s;
}

struct MeshNode
{
    int meshIndex;
    glm::mat4 worldTransform;
};

void s_CollectMeshNodes(
    const tinygltf::Model& _model,
    int _nodeIndex,
    const glm::mat4& _parentTransform,
    std::vector<MeshNode>& _out)
{
    const tinygltf::Node& node = _model.nodes[_nodeIndex];

    const glm::mat4 worldTransform =
        _parentTransform * s_NodeLocalTransform(node);

    if (node.mesh >= 0)
        _out.push_back({ node.mesh, worldTransform });

    for (int child : node.children)
        s_CollectMeshNodes(
            _model,
            child,
            worldTransform,
            _out
        );
}

std::vector<std::unique_ptr<Mesh>> MeshLoader::LoadGLTF(
    const core::gpu::Device& _device,
    const std::string& _path,
    TextureLibrary& _textureLibrary,
    MaterialLibrary& _materialLibrary)
{
    std::string ext = std::filesystem::path(_path).extension().string();

    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext != ".gltf" && ext != ".glb")
        throw std::runtime_error("Unsupported mesh format: " + _path);

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;

    std::string err;
    std::string warn;

    bool ret = false;

    if (ext == ".gltf")
    {
        ret = loader.LoadASCIIFromFile(
            &model,
            &err,
            &warn,
            _path
        );
    }
    else
    {
        ret = loader.LoadBinaryFromFile(
            &model,
            &err,
            &warn,
            _path
        );
    }

    if (!warn.empty())
        std::cerr << "GLTF Warning: " << warn << "\n";

    if (!err.empty())
        std::cerr << "GLTF Error: " << err << "\n";

    if (!ret)
        throw std::runtime_error("Failed to load GLTF: " + _path);

    if (model.meshes.empty())
        throw std::runtime_error("GLTF contains no meshes");

    std::cout << "Model '" << _path << "' contains " << model.meshes.size() << " mesh(es)\n";

    std::vector<std::unique_ptr<Mesh>> result;
    result.reserve(model.meshes.size());

    for (const tinygltf::Mesh& gltfMesh : model.meshes)
    {
        if (gltfMesh.primitives.empty())
            continue;

        std::cout << "Loading GLTF mesh '" << gltfMesh.name << "' with " << gltfMesh.primitives.size() << " primitives\n";

        result.push_back(
            s_BuildMesh(
                _device,
                model,
                gltfMesh,
                _textureLibrary,
                _materialLibrary,
                _path
            )
        );
    }

    return result;
}