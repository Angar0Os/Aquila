#ifndef AQUILA_ENGINE_CORE_IMGUI_CONTEXT_IMPL_H
#define AQUILA_ENGINE_CORE_IMGUI_CONTEXT_IMPL_H
#pragma once

#include <core/imgui/context.h>
#include <vulkan/vulkan_raii.hpp>

namespace core::imgui
{
	struct ViewportInfo
	{
		std::unique_ptr<core::gpu::Image>	colorImage	= nullptr;
		VkDescriptorSet						dsSet		= nullptr;
		std::pair<uint32_t, uint32_t>		desiredSize = { 0, 0 };
		bool								isUsable	= false;
	};

	struct Context::Impl
	{
		ViewportInfo* currentViewportState = nullptr;
	};
}

#endif //AQUILA_ENGINE_CORE_IMGUI_CONTEXT_IMPL_H
