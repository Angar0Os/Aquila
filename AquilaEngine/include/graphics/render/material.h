#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_H
#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>

namespace graphics::render
{
	using TextureHandle = uint32_t;
	static constexpr TextureHandle INVALID_TEXTURE = UINT32_MAX;

	using MaterialHandle = uint32_t;
	static constexpr MaterialHandle INVALID_MATERIAL = UINT32_MAX;

	template<typename T>
	struct MaterialProperty
	{
		T value{};
		TextureHandle texture = INVALID_TEXTURE;
	};

	struct Material
	{
		std::string name;

		MaterialProperty<glm::vec4> baseColor {
			.value = glm::vec4(1.0f),
			.texture = INVALID_TEXTURE
		};

		MaterialProperty<float> metallic {
			.value = 0.0f
		};

		MaterialProperty<float> roughness {
			.value = 1.0f
		};

		MaterialProperty<glm::vec3> normal {
			.value = glm::vec3(0.0f, 0.0f, 1.0f),
			.texture = INVALID_TEXTURE
		};

		MaterialProperty<glm::vec3> emissive {
			.value = glm::vec3(0.0f),
			.texture = INVALID_TEXTURE
		};

		TextureHandle ormTexture = INVALID_TEXTURE;
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_H
