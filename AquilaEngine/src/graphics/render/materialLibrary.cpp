#include <graphics/render/materialLibrary.h>

#include <stdexcept>
#include <vector>

using namespace graphics::render;

MaterialLibrary::MaterialLibrary(const core::gpu::Device& _device, TextureLibrary& _textureLibrary)
	: device(_device)
	, textureLibrary(_textureLibrary)
{
	Material defaultMat;

	defaultMat.name = "default_material";

	defaultMat.baseColor.value = glm::vec4(1.0f);
	defaultMat.baseColor.texture = INVALID_TEXTURE;
	defaultMat.metallic.value = 0.0f;
	defaultMat.roughness.value = 1.0f;
	defaultMat.normal.value = glm::vec3(0.0f, 0.0f, 1.0f);
	defaultMat.normal.texture = INVALID_TEXTURE;
	defaultMat.emissive.value = glm::vec3(0.0f);
	defaultMat.emissive.texture = INVALID_TEXTURE;
	defaultMat.ormTexture = INVALID_TEXTURE;

	defaultMaterial = Add("default_material", std::move(defaultMat));
}

MaterialHandle MaterialLibrary::Add(const std::string& _name, Material _material)
{
	if (Contains(_name))
		return Find(_name);

	const MaterialHandle handle = static_cast<MaterialHandle>(materials.size());

	materials.push_back({
		.name = _name,
		.material = std::move(_material)
		});

	lookup[_name] = handle;

	dirty = true;

	return handle;
}

bool MaterialLibrary::Contains(const std::string& _name) const
{
	return lookup.contains(_name);
}

MaterialHandle MaterialLibrary::Find(const std::string& _name) const
{
	auto it = lookup.find(_name);

	if (it == lookup.end())
		return INVALID_MATERIAL;

	return it->second;
}

const Material& MaterialLibrary::Get(MaterialHandle _handle) const
{
	if (_handle == INVALID_MATERIAL || _handle >= materials.size())
	{
		throw std::out_of_range(
			"Invalid MaterialHandle"
		);
	}

	return materials[_handle].material;
}

MaterialGPUData MaterialLibrary::BuildGPUData(MaterialHandle _handle) const
{
	const Material& material = Get(_handle);

	MaterialGPUData gpu{};

	gpu.baseColor = material.baseColor.value;

	gpu.emissive = glm::vec4(material.emissive.value, 1.0f);

	gpu.properties = glm::vec4(
		material.metallic.value,
		material.roughness.value,
		0.0f,
		0.0f
	);

	gpu.textures = glm::uvec4(
		material.baseColor.texture == INVALID_TEXTURE
		? textureLibrary.GetWhite()
		: material.baseColor.texture,

		material.normal.texture == INVALID_TEXTURE
		? textureLibrary.GetFlatNormal()
		: material.normal.texture,

		material.ormTexture == INVALID_TEXTURE
		? textureLibrary.GetWhite()
		: material.ormTexture,

		material.emissive.texture == INVALID_TEXTURE
		? textureLibrary.GetBlack()
		: material.emissive.texture
	);

	return gpu;
}

void MaterialLibrary::UploadGPUData()
{
	if (materials.empty())
		return;

	if (!gpuBuffer)
	{
		core::gpu::BufferCreateInfo bufferInfo{};

		bufferInfo.size = sizeof(MaterialGPUData) * MAX_MATERIALS_CAPACITY;
		bufferInfo.usage = core::gpu::utils::EBufferUsage::StorageBuffer;
		bufferInfo.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent;

		gpuBuffer = std::make_unique<core::gpu::Buffer>(device, bufferInfo);
	}

	if (!dirty)
		return;

	if (materials.size() > MAX_MATERIALS_CAPACITY)
	{
		throw std::runtime_error("MaterialLibrary: material count exceeds MAX_MATERIALS_CAPACITY");
	}

	const size_t byteSize = sizeof(MaterialGPUData) * materials.size();

	std::vector<MaterialGPUData> data;
	data.reserve(materials.size());

	for (MaterialHandle handle = 0; handle < materials.size(); ++handle)
	{
		data.push_back(
			BuildGPUData(handle)
		);
	}

	gpuBuffer->CopyFrom(data.data(), byteSize);

	dirty = false;
}

const core::gpu::Buffer& MaterialLibrary::GetGPUBuffer() const
{
	if (!gpuBuffer)
	{
		throw std::runtime_error("Material GPU buffer has not been created");
	}

	return *gpuBuffer;
}

size_t MaterialLibrary::Size() const
{
	return materials.size();
}

MaterialHandle MaterialLibrary::GetDefaultMaterial() const
{
	return defaultMaterial;
}