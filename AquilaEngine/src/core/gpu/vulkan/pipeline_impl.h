#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_PIPELINE_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_PIPELINE_H
#pragma once

#include <core/gpu/pipeline.h>
#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	struct Pipeline::Impl
	{
		vk::raii::PipelineLayout pipelineLayout;
		vk::raii::Pipeline pipeline;
		utils::EPipelineType	type;

		explicit Impl(const core::gpu::Device* device, const PipelineCreateInfo& info);
		~Impl();
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_PIPELINE_H
