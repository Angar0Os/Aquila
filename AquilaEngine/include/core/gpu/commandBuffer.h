#ifndef AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
#define AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
#pragma once

#include <memory>
#include <vector>
#include <functional>

#include <core/gpu/utils/enums.h>

namespace core::gpu
{
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
			const Image*			image	= nullptr;
			bool                    clear	= true;
			float                   clearR	= 0.0f;
			float                   clearG	= 0.0f;
			float                   clearB	= 0.0f;
			float                   clearA	= 1.0f;
		};

		struct DepthAttachmentInfo
		{
			const Image*			image		= nullptr;
			bool                    clear		= true;
			float                   clearDepth	= 1.0f;
		};

		CommandBuffer(const Device& _device);
		~CommandBuffer();

		bool IsCpuFree() const;
		bool IsGpuFree() const;

		void Record(std::function<void()> _content);

		void Submit(const Device& _device, uint32_t _frameIndex, bool isImmediate = false);

		template<typename T>
		void Bind(T& _input);
		
		template<typename T>
		void Bind(const DescriptorSet& _dsSet, T& _input, uint32_t _firstSet);

		void SetViewport(float _x, float _y, float _width, float _height, float _minDepth, float _maxDepth);
		void SetScissor(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height);

		void Dispatch(uint32_t _x, uint32_t _y, uint32_t _z);

		void BeginRendering(const Device& _device, const std::vector<RenderingAttachmentInfo>& colorAttachments, const DepthAttachmentInfo& depthAttachment);
		void EndRendering();

		void TransitionImageLayout(const Image& _image, utils::EImageLayout _oldLayout, utils::EImageLayout _newLayout, bool _isDepth = false);
		void BlitImage(const Image& _srcImage, const Image& _dstImage);

		void CopyBuffer(const Buffer& _srcBuffer, const Buffer& _dstBuffer, uint32_t _size);
		void CopyBufferToImage(const Buffer& _srcBuffer, const Image& _dstImage, uint32_t _width, uint32_t _height);

		void PushConstants(const Pipeline& _pipeline, uint32_t _stageFlags, uint32_t _offset, uint32_t _size, const void* _pValues);

		void BuildAccelerationStructure(const AccelerationStructure& _accelerationStucture);
		void AccelerationStructureBarrier();

		void TraceRays();

		Impl& GetImpl() const;
}

#endif //AQUILA_ENGINE_CORE_GPU_COMMAND_BUFFER_H
