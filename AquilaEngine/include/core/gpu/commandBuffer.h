#ifndef AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
#define AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
#pragma once

#include <memory>
#include <vector>
#include <functional>

#include <core/gpu/utils/enums.h>

namespace core::gpu
{
	class AccelerationStructure;
	class Buffer;
	class DescriptorSet;
	class Device;
	class Image;
	class Pipeline;

	class CommandBuffer
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	public:
		struct RenderingAttachmentInfo
		{
			const Image* image = nullptr;
			bool                    clear = true;
			float                   clearR = 0.0f;
			float                   clearG = 0.0f;
			float                   clearB = 0.0f;
			float                   clearA = 1.0f;
		};

		struct DepthAttachmentInfo
		{
			const Image* image = nullptr;
			bool                    clear = true;
			float                   clearDepth = 1.0f;
		};

		CommandBuffer(const Device& _device);
		~CommandBuffer();

		bool IsCpuFree() const;
		bool IsGpuFree() const;

		void Record(std::function<void()> _content) const;

		void Submit(const Device& _device, bool isImmediate = false) const;

		template<typename T>
		void Bind(T& _input);

		template<typename T>
		void Bind(const DescriptorSet& _dsSet, T& _input, uint32_t _firstSet) const;

		void SetViewport(float _x, float _y, float _width, float _height, float _minDepth, float _maxDepth);
		void SetScissor(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height);

		void Dispatch(uint32_t _x, uint32_t _y, uint32_t _z);

		void BeginRendering(const Device& _device, const std::vector<RenderingAttachmentInfo>& colorAttachments, const DepthAttachmentInfo& depthAttachment);
		void EndRendering();

		void TransitionImageLayout(const Image& _image, utils::EImageLayout _oldLayout, utils::EImageLayout _newLayout, bool _isDepth = false);
		void BlitImage(const Image& _srcImage, const Image& _dstImage);
		
		void CopyBuffer(const Buffer& _srcBuffer, const Buffer& _dstBuffer, uint32_t _size) const;
		void CopyBufferToImage(const Buffer& _srcBuffer, const Image& _dstImage, uint32_t _width, uint32_t _height);

		void PushConstants(const Pipeline& _pipeline, uint32_t _stageFlags, uint32_t _offset, uint32_t _size, const void* _pValues);

		void DrawIndexed(uint32_t _indexCount, uint32_t _instanceCount, uint32_t _firstIndex, uint32_t _vertexOffset, uint32_t _firstInstance) const;

		void AccelerationStructureBarrier();

		void TraceRays(const Device& _device,
			void* _raygenSBT, uint32_t _raygenOffset, uint32_t _raygenStride,
			void* _missSBT, uint32_t _missOffset, uint32_t _missStride, uint32_t _missCount,
			void* _hitSBT, uint32_t _hitOffset, uint32_t _hitStride, uint32_t _hitCount,
			void* _callableSBT, uint32_t _callableOffset, uint32_t _callableStride, uint32_t _callableCount,
			uint32_t _width, uint32_t _height, uint32_t _depth);

		Impl& GetImpl() const;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
