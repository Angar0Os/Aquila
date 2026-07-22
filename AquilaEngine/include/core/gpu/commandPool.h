#ifndef AQUILA_ENGINE_CORE_GPU_COMMAND_POOL_H
#define AQUILA_ENGINE_CORE_GPU_COMMAND_POOL_H
#pragma once

#include <memory>

#include <core/gpu/utils/enums.h>

namespace core::gpu
{
	class Device;

	struct CommandPoolCreateInfo
	{
		uint32_t queueFamilyIndex;
		utils::ECommandPoolCreateFlags flags = utils::ECommandPoolCreateFlags::None;
	};

	class CommandPool
	{
    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;

    public:
        CommandPool(const Device* _device, const CommandPoolCreateInfo& _info);
        ~CommandPool();

        void Reset(bool _releaseResources = false);

        Impl& GetImpl() const;
    };
}

#endif //AQUILA_ENGINE_CORE_GPU_COMMAND_POOL_H
