#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_GPU_DATA_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_GPU_DATA_H
#pragma once

#include <glm/glm.hpp>

namespace graphics::render
{
	struct MaterialGPUData
	{
		glm::vec4 baseColor;
		glm::vec4 emissive;

		glm::vec4 properties;
		glm::uvec4 textures;
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_MATERIAL_GPU_DATA_H
