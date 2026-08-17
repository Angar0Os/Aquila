#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_H
#pragma once

#include <core/gpu/commandBuffer.h>
#include <core/gpu/device.h>
#include <core/gpu/buffer.h>
#include <core/gpu/image.h>
#include <core/gpu/texture.h>
#include <core/gpu/descriptorSet.h>
#include <core/gpu/utils/enums.h>

#include <glm/glm.hpp>
#include <stb_image.h>

#include <memory>
#include <iostream>
#include <string>

namespace graphics::render
{
	// TODO : This class will need a complete rework. Need to learn about PBR and how to manage it properly

	struct MaterialGPUData 
	{
		glm::vec4 baseColor;
		// rgb = albedoColor, a = unused

		glm::vec4 params;
		// x = metallic
		// y = roughness
		// z = hasAlbedoTexture
		// w = hasNormalTexture
	};

	struct Material
	{
		Material() = default; 

		Material(const core::gpu::Device& _device, const core::gpu::DescriptorSetLayout& _materialLayout)
		{
			name = "Fallback";

			gpuData.baseColor = glm::vec4(albedoColor, 1.0f);
			gpuData.params = glm::vec4(metalness, roughnessValue, 0.0f, 0.0f);

			core::gpu::BufferCreateInfo materialBufferInfo{};
			materialBufferInfo.size = sizeof(MaterialGPUData);
			materialBufferInfo.usage = core::gpu::utils::EBufferUsage::UniformBuffer;
			materialBufferInfo.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent;

			materialBuffer = std::make_unique<core::gpu::Buffer>(_device, materialBufferInfo);
			materialBuffer->CopyFrom(&gpuData, sizeof(MaterialGPUData));

			static constexpr uint32_t kMagenta = 0xFFFF00FFu;
			static constexpr uint32_t kFlatNormal = 0xFFFF8080u;
			static constexpr uint32_t kWhite = 0xFFFFFFFFu;

			auto makeSolidTexture = [&](uint32_t _pixel) -> std::pair<std::unique_ptr<core::gpu::Image>, std::unique_ptr<core::gpu::Texture>>
				{
					core::gpu::ImageCreateInfo imageInfo{};
					imageInfo.width = 1u;
					imageInfo.height = 1u;
					imageInfo.format = core::gpu::utils::ETextureFormat::RGBA8_UNorm;
					imageInfo.usage = core::gpu::utils::EImageUsage::Sampled | core::gpu::utils::EImageUsage::TransferDst;

					auto image = std::make_unique<core::gpu::Image>(_device, imageInfo);

					core::gpu::BufferCreateInfo stagingInfo{};
					stagingInfo.size = sizeof(uint32_t);
					stagingInfo.usage = core::gpu::utils::EBufferUsage::TransferSrc;
					stagingInfo.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent;

					auto staging = std::make_unique<core::gpu::Buffer>(_device, stagingInfo);
					staging->CopyFrom(&_pixel, sizeof(uint32_t));

					auto cmdBuf = _device.AcquireCommandBuffer();
					cmdBuf->Record([&]() {
						cmdBuf->TransitionImageLayout(
							*image,
							core::gpu::utils::EImageLayout::Undefined,
							core::gpu::utils::EImageLayout::TransferDst,
							false
						);

						cmdBuf->CopyBufferToImage(*staging, *image, 1u, 1u);

						cmdBuf->TransitionImageLayout(
							*image,
							core::gpu::utils::EImageLayout::TransferDst,
							core::gpu::utils::EImageLayout::ShaderReadOnly,
							false
						);
						});

					cmdBuf->Submit(_device, true);
					_device.ReleaseCommandBuffer(cmdBuf);

					auto texture = std::make_unique<core::gpu::Texture>(_device, *image);
					return { std::move(image), std::move(texture) };
				};

			std::tie(albedoImage, albedoTexture) = makeSolidTexture(kMagenta);
			std::tie(normalImage, normalTexture) = makeSolidTexture(kFlatNormal);
			std::tie(roughnessMetalImage, roughnessMetalTexture) = makeSolidTexture(kWhite);

			descriptorSet = std::make_unique<core::gpu::DescriptorSet>(_device, _materialLayout);
			descriptorSet->Bind(0, *materialBuffer);
			descriptorSet->Bind(1, *albedoTexture);
			descriptorSet->Bind(2, *normalTexture);
			descriptorSet->Bind(3, *roughnessMetalTexture);
			descriptorSet->Update(_device);
		}

		static std::pair<std::unique_ptr<core::gpu::Image>, std::unique_ptr<core::gpu::Texture>> CreateTexture(
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

		void SetAlbedo(const core::gpu::Device& _device, std::unique_ptr<core::gpu::Image> _image, std::unique_ptr<core::gpu::Texture> _texture)
		{
			albedoImage = std::move(_image);
			albedoTexture = std::move(_texture);
			descriptorSet->Bind(1, *albedoTexture);
			descriptorSet->Update(_device);
		}

		void SetNormal(const core::gpu::Device& _device, std::unique_ptr<core::gpu::Image> _image, std::unique_ptr<core::gpu::Texture> _texture)
		{
			normalImage = std::move(_image);
			normalTexture = std::move(_texture);
			descriptorSet->Bind(2, *normalTexture);
			descriptorSet->Update(_device);
		}

		void SetRoughnessMetal(const core::gpu::Device& _device, std::unique_ptr<core::gpu::Image> _image, std::unique_ptr<core::gpu::Texture> _texture)
		{
			roughnessMetalImage = std::move(_image);
			roughnessMetalTexture = std::move(_texture);
			descriptorSet->Bind(3, *roughnessMetalTexture);
			descriptorSet->Update(_device);
		}

		static std::unique_ptr<core::gpu::Image> UploadHDRTexture(const core::gpu::Device& _device, const std::string& _path)
		{
			int width, height, channels;
			float* pixels = stbi_loadf(_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

			if (!pixels)
			{
				std::cerr << "MaterialFactory: failed to load HDR " << _path << "\n";
				return nullptr;
			}

			size_t imageSize = static_cast<size_t>(width) * height * 4 * sizeof(float);

			core::gpu::BufferCreateInfo stagingInfo {
				.size = imageSize,
				.usage = core::gpu::utils::EBufferUsage::TransferSrc,
				.memoryProperties = core::gpu::utils::EMemoryProperty::HostVisible | core::gpu::utils::EMemoryProperty::HostCoherent
			};

			auto staging = std::make_unique<core::gpu::Buffer>(_device, stagingInfo);
			staging->CopyFrom(pixels, imageSize);
			stbi_image_free(pixels);

			core::gpu::ImageCreateInfo imageInfo {
				.width				= static_cast<uint32_t>(width),
				.height				= static_cast<uint32_t>(height),
				.mipLevels			= 1,
				.format				= core::gpu::utils::ETextureFormat::RGBA32_Float,
				.tiling				= core::gpu::utils::EImageTiling::Optimal,
				.usage				= core::gpu::utils::EImageUsage::TransferDst | core::gpu::utils::EImageUsage::Sampled,
				.memoryProperties	= core::gpu::utils::EMemoryProperty::DeviceLocal,
				.samples			= core::gpu::utils::ESampleCount::e1
			};
			auto image = std::make_unique<core::gpu::Image>(_device, imageInfo);

			auto commandBuffer = _device.AcquireCommandBuffer();

			commandBuffer->Record([&]() {
				commandBuffer->TransitionImageLayout(*image, core::gpu::utils::EImageLayout::Undefined, core::gpu::utils::EImageLayout::TransferDst, false);
				commandBuffer->CopyBufferToImage(*staging, *image, width, height);
				commandBuffer->TransitionImageLayout(*image, core::gpu::utils::EImageLayout::TransferDst, core::gpu::utils::EImageLayout::ShaderReadOnly, false);

			});
			commandBuffer->Submit(_device, true);
			_device.ReleaseCommandBuffer(commandBuffer);

			return image;
		}

		std::string name = "Default";

		std::unique_ptr<core::gpu::Image>   albedoImage;
		std::unique_ptr<core::gpu::Texture> albedoTexture;

		std::unique_ptr<core::gpu::Image>   normalImage;
		std::unique_ptr<core::gpu::Texture> normalTexture;

		std::unique_ptr<core::gpu::Image>   roughnessMetalImage;
		std::unique_ptr<core::gpu::Texture> roughnessMetalTexture;

		glm::vec3 albedoColor = { 1.0f, 1.0f, 1.0f };
		float     metalness = 0.0f;
		float     roughnessValue = 1.0f;

		bool hasAlbedoTexture = false;
		bool hasNormalTexture = false;
		bool hasRoughnessMetalTexture = false;

		std::unique_ptr<core::gpu::DescriptorSet> descriptorSet;
		std::unique_ptr<core::gpu::Buffer> materialBuffer;
		MaterialGPUData gpuData;

		size_t index; // used by materialInstanceManager to remove on destruction

		uint32_t albedoBindlessIndex = 0;         
		uint32_t normalBindlessIndex = 0;         
		uint32_t roughMetalBindlessIndex = 0;     
		uint32_t materialTableIndex = UINT32_MAX; 

		void SetBaseColor(const glm::vec4& color)
		{
			gpuData.baseColor = color;

			materialBuffer->CopyFrom(
				&gpuData,
				sizeof(MaterialGPUData)
			);
		}

		void SetRoughness(const float& _roughness)
		{
			gpuData.params.y = _roughness;

			materialBuffer->CopyFrom(
				&gpuData,
				sizeof(MaterialGPUData)
			);
		}

		void SetMetalness(const float& _metalness)
		{
			gpuData.params.x = _metalness;

			materialBuffer->CopyFrom(
				&gpuData,
				sizeof(MaterialGPUData)
			);
		}
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_H
