#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_ACCELERATION_STRUCTURE_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_ACCELERATION_STRUCTURE_H
#pragma once

#include <core/gpu/accelerationStructure.h>
#include <core/gpu/utils/enums.h>

#include <vulkan/vulkan_raii.hpp>

namespace core::gpu
{
	struct AccelerationStructure::Impl
	{
		std::optional<vk::raii::AccelerationStructureKHR>	accelerationStructure;
		std::unique_ptr<Buffer>								buffer;
		std::unique_ptr<Buffer>								scratchBuffer;
		std::unique_ptr<Buffer>								instanceBuffer;

		utils::EAccelerationStructureType					type;
		std::vector<AccelerationStructureGeometry>			geometries;
		std::vector<AccelerationStructureInstance>			instances;

		vk::BuildAccelerationStructureFlagsKHR				buildFlags;
		vk::AccelerationStructureBuildSizesInfoKHR			buildSizes;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_ACCELERATION_STRUCTURE_H
