#ifndef AQUILA_ENGINE_CORE_GPU_ACCELERATION_STRUCTURE_H
#define AQUILA_ENGINE_CORE_GPU_ACCELERATION_STRUCTURE_H
#pragma once

#include <core/gpu/utils/enums.h>

#include <vector>
#include <memory>

namespace core::gpu
{
    class AccelerationStructure;
    class Buffer;
    class CommandBuffer;
    class Device;

    struct AccelerationStructureGeometry
    {
        Buffer* vertexBuffer = nullptr;
        Buffer* indexBuffer = nullptr;
        Buffer* transformBuffer = nullptr;

        uint32_t    vertexCount = 0;
        uint32_t    vertexStride = 0;
        uint32_t    indexCount = 0;
        uint32_t    triangleCount = 0;

        bool        opaque = true;
    };

    struct AccelerationStructureInstance
    {
        float                   transform[12];
        uint32_t                instanceCustomIndex = 0;
        uint32_t                mask = 0xFF;
        uint32_t                instanceShaderBindingTableRecordOffset = 0;
        AccelerationStructure* blas = nullptr;
    };

    struct AccelerationStructureCreateInfo
    {
        utils::EAccelerationStructureType type;

        std::vector<AccelerationStructureGeometry> geometries;
        std::vector<AccelerationStructureInstance> instances;

        bool allowUpdate = false;
        bool preferFastTrace = true;
    };

    class AccelerationStructure
    {
    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;

    public:
        AccelerationStructure(const Device& _device, const AccelerationStructureCreateInfo& _info);
        ~AccelerationStructure();

        uint64_t GetDeviceAddress() const;

        void Build(const CommandBuffer* _cmdBuf);

        void CreateBottomLevel(const Device& _device, const AccelerationStructureCreateInfo& _info);
        void CreateTopLevel(const Device& _device, const AccelerationStructureCreateInfo& _info);
        void CreateAccelerationStructureBuffer(const Device& _device, uint32_t _size);
        void CreateScratchBuffer(const Device& _device, uint32_t size);

        Impl& GetImpl() const;
    };
}

#endif //AQUILA_ENGINE_CORE_GPU_ACCELERATION_STRUCTURE_H