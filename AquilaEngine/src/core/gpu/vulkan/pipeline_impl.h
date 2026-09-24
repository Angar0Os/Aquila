#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_PIPELINE_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_PIPELINE_H
#pragma once

#include <core/gpu/pipeline.h>
#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	struct Pipeline::Impl
	{
		vk::raii::PipelineLayout	pipelineLayout	= nullptr;
		vk::raii::Pipeline			pipeline		= nullptr;
		utils::EPipelineType		type			= utils::EPipelineType::Graphics;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_PIPELINE_H
