#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_LIGHT_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_LIGHT_H
#pragma once

#include <core/gpu/utils/enums.h>
#include <glm/glm.hpp>

namespace graphics::render
{
	struct LightData
	{
		core::gpu::utils::ELightType	type;
		glm::vec3						color;
		float							intensity;
		glm::vec2						spotAngles = { glm::radians(15.0f), glm::radians(30.0f) };
		float							radius = 0.0f;
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_LIGHT_H
