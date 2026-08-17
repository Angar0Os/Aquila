#include <graphics/render/textureLibrary.h>

#include <core/gpu/buffer.h>
#include <core/gpu/commandBuffer.h>

#include <iostream>
#include <stdexcept>
#include <utility>

#include <stb_image.h>

using namespace graphics::render;


core::gpu::utils::ETextureFormat TextureLibrary::GetTextureFormat(ETextureSemantic semantic)
{
	switch (semantic)
	{
	case ETextureSemantic::BaseColor:
	case ETextureSemantic::Emissive:
		return core::gpu::utils::ETextureFormat::RGBA8_SRGB;

	case ETextureSemantic::Normal:
	case ETextureSemantic::ORM:
		return core::gpu::utils::ETextureFormat::RGBA8_UNorm;
	}

	return core::gpu::utils::ETextureFormat::RGBA8_UNorm;
}

std::pair<std::unique_ptr<core::gpu::Image>, std::unique_ptr<core::gpu::Texture>> TextureLibrary::CreateTexture(
	const core::gpu::Device& _device,
	const void* _pixels,
	uint32_t _width,
	uint32_t _height,
	core::gpu::utils::ETextureFormat _format)
{
	core::gpu::ImageCreateInfo imageInfo{};
	imageInfo.width = _width;
	imageInfo.height = _height;
	imageInfo.format = _format;
	imageInfo.usage = core::gpu::utils::EImageUsage::Sampled | core::gpu::utils::EImageUsage::TransferDst;

	auto image = std::make_unique<core::gpu::Image>(_device, imageInfo);

	const size_t byteSize = static_cast<size_t>(_width) * _height * 4;

	core::gpu::BufferCreateInfo stagingInfo{};
	stagingInfo.size = byteSize;
	stagingInfo.usage = core::gpu::utils::EBufferUsage::TransferSrc;
	stagingInfo.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent;

	auto staging = std::make_unique<core::gpu::Buffer>(_device, stagingInfo);
	staging->CopyFrom(_pixels, byteSize);

	auto cmdBuf = _device.AcquireCommandBuffer();
	cmdBuf->Record([&]() {
		cmdBuf->TransitionImageLayout(*image, core::gpu::utils::EImageLayout::Undefined, core::gpu::utils::EImageLayout::TransferDst, false);
		cmdBuf->CopyBufferToImage(*staging, *image, _width, _height);
		cmdBuf->TransitionImageLayout(*image, core::gpu::utils::EImageLayout::TransferDst, core::gpu::utils::EImageLayout::ShaderReadOnly, false);
		});
	cmdBuf->Submit(_device, true);
	_device.ReleaseCommandBuffer(cmdBuf);

	auto texture = std::make_unique<core::gpu::Texture>(_device, *image);
	return { std::move(image), std::move(texture) };
}

TextureLibrary::TextureLibrary(const core::gpu::Device& _device)
	: device(_device)
{
	auto createSolid = [&](const std::string& name, uint32_t pixel, ETextureSemantic semantic)
		{
			auto [image, texture] = CreateTexture(
				device,
				&pixel,
				1,
				1,
				GetTextureFormat(semantic)
			);

			return Add(name, std::move(image), std::move(texture));
		};

	whiteTexture = createSolid(
		"default_white",
		0xFFFFFFFFu,
		ETextureSemantic::BaseColor
	);

	blackTexture = createSolid(
		"default_black",
		0x000000FFu,
		ETextureSemantic::BaseColor
	);

	flatNormalTexture = createSolid(
		"default_flat_normal",
		0xFFFF8080u,
		ETextureSemantic::Normal
	);
}

std::unique_ptr<core::gpu::Image> TextureLibrary::LoadHDRImage(const core::gpu::Device& _device, const std::string& _path)
{
	int width = 0;
	int height = 0;
	int channels = 0;

	float* pixels = stbi_loadf(
		_path.c_str(),
		&width,
		&height,
		&channels,
		STBI_rgb_alpha
	);

	if (!pixels)
	{
		std::cerr << "TextureLibrary: failed to load HDR '" << _path << "'\n";
		return nullptr;
	}

	core::gpu::ImageCreateInfo imageInfo{};
	imageInfo.width = static_cast<uint32_t>(width);
	imageInfo.height = static_cast<uint32_t>(height);
	imageInfo.format = core::gpu::utils::ETextureFormat::RGBA32_Float;
	imageInfo.usage = core::gpu::utils::EImageUsage::Sampled | core::gpu::utils::EImageUsage::TransferDst;

	auto image = std::make_unique<core::gpu::Image>(_device, imageInfo);

	const size_t byteSize = static_cast<size_t>(width) * height * 4 * sizeof(float);

	core::gpu::BufferCreateInfo stagingInfo{};
	stagingInfo.size = byteSize;
	stagingInfo.usage = core::gpu::utils::EBufferUsage::TransferSrc;
	stagingInfo.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent;

	auto staging = std::make_unique<core::gpu::Buffer>(_device, stagingInfo);
	staging->CopyFrom(pixels, byteSize);

	stbi_image_free(pixels);

	auto cmdBuf = _device.AcquireCommandBuffer();
	cmdBuf->Record([&]() {
		cmdBuf->TransitionImageLayout(*image, core::gpu::utils::EImageLayout::Undefined, core::gpu::utils::EImageLayout::TransferDst, false);
		cmdBuf->CopyBufferToImage(*staging, *image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		cmdBuf->TransitionImageLayout(*image, core::gpu::utils::EImageLayout::TransferDst, core::gpu::utils::EImageLayout::ShaderReadOnly, false);
		});
	cmdBuf->Submit(_device, true);
	_device.ReleaseCommandBuffer(cmdBuf);

	return image;
}

TextureHandle TextureLibrary::AddFromPixels(const std::string& _name, const void* _pixels, uint32_t _width, uint32_t _height, ETextureSemantic _semantic)
{
	if (!_pixels || _width == 0 || _height == 0)
		return INVALID_TEXTURE;

	if (Contains(_name))
		return Find(_name);

	const auto format = GetTextureFormat(_semantic);

	auto [image, texture] = CreateTexture(
		device,
		_pixels,
		_width,
		_height,
		format
	);

	if (!image || !texture)
		return INVALID_TEXTURE;

	return Add(_name, std::move(image), std::move(texture));
}

TextureHandle TextureLibrary::Add(const std::string& _name, std::unique_ptr<core::gpu::Image> _image, std::unique_ptr<core::gpu::Texture> _texture)
{
    if (Contains(_name))
        return Find(_name);

    TextureHandle handle = static_cast<TextureHandle>(textures.size());

    textures.push_back({
        .name = _name,
        .image = std::move(_image),
        .texture = std::move(_texture)
    });

    lookup[_name] = handle;
    return handle;
}

TextureHandle TextureLibrary::Find(const std::string& _name) const
{
    auto it = lookup.find(_name);

    if (it == lookup.end())
        return INVALID_TEXTURE;

    return it->second;
}

const core::gpu::Texture& TextureLibrary::Get(TextureHandle _handle) const
{
    if (_handle == INVALID_TEXTURE ||
        _handle >= textures.size())
    {
        throw std::out_of_range("Invalid TextureHandle");
    }

    return *textures[_handle].texture;
}

bool TextureLibrary::Contains(const std::string& _name) const
{
    return lookup.contains(_name);
}

TextureHandle TextureLibrary::Load(const std::string& _path, ETextureSemantic _semantic)
{
	if (Contains(_path))
		return Find(_path);

	int width = 0;
	int height = 0;
	int channels = 0;

	stbi_uc* pixels = stbi_load(
		_path.c_str(),
		&width,
		&height,
		&channels,
		STBI_rgb_alpha
	);

	if (!pixels)
	{
		std::cerr << "TextureLibrary: failed to load '" << _path << "'\n";
		return INVALID_TEXTURE;
	}

	const auto format = GetTextureFormat(_semantic);

	auto [image, texture] = CreateTexture(
		device,
		pixels,
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		format
	);

	stbi_image_free(pixels);

	if (!image || !texture)
		return INVALID_TEXTURE;

	return Add(
		_path,
		std::move(image),
		std::move(texture)
	);
}

TextureHandle TextureLibrary::GetWhite() const
{
	return whiteTexture;
}

TextureHandle TextureLibrary::GetBlack() const
{
	return blackTexture;
}

TextureHandle TextureLibrary::GetFlatNormal() const
{
	return flatNormalTexture;
}

size_t TextureLibrary::Size() const
{
    return textures.size();
}