#ifndef AQUILA_ENGINE_CORE_GPU_DESCRIPTOR_POOL_H
#define AQUILA_ENGINE_CORE_GPU_DESCRIPTOR_POOL_H
#pragma once

#include <memory>
#include <vector>

namespace core::gpu
{
	class Device;

	class DescriptorPool
	{
    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;

    public:
        DescriptorPool(const Device* _device);
        ~DescriptorPool();

        Impl& GetImpl() const;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_DESCRIPTOR_POOL_H
