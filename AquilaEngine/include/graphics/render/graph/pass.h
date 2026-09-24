#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_PASS_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_PASS_H
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <core/gpu/buffer.h>
#include <core/gpu/image.h>
#include <core/gpu/texture.h>

#include <core/gpu/utils/enums.h>

namespace graphics::render::graph
{
	using ResourceHandle = size_t;
	inline constexpr ResourceHandle kInvalidResourceHandle = SIZE_MAX;

	enum class ResourceType
	{
		Buffer,
		Texture
	};

	enum class ResourceState
	{
		Undefined,
		RenderTarget,
		DepthWrite,
		DepthRead,
		ShaderRead,
		UnorderedAccess,
		TransferSrc,
		TransferDst,
		Present
	};

	inline core::gpu::utils::EImageLayout ToImageLayout(ResourceState _state)
	{
		switch (_state)
		{
		case ResourceState::Undefined:       return core::gpu::utils::EImageLayout::Undefined;
		case ResourceState::RenderTarget:    return core::gpu::utils::EImageLayout::ColorAttachment;
		case ResourceState::DepthWrite:      return core::gpu::utils::EImageLayout::DepthStencilAttachment;
		case ResourceState::DepthRead:       return core::gpu::utils::EImageLayout::DepthStencilAttachment;
		case ResourceState::ShaderRead:      return core::gpu::utils::EImageLayout::ShaderReadOnly;
		case ResourceState::UnorderedAccess: return core::gpu::utils::EImageLayout::General;
		case ResourceState::TransferSrc:     return core::gpu::utils::EImageLayout::TransferSrc;
		case ResourceState::TransferDst:     return core::gpu::utils::EImageLayout::TransferDst;
		case ResourceState::Present:         return core::gpu::utils::EImageLayout::Present;
		}

		return core::gpu::utils::EImageLayout::Undefined;
	}

	struct ResourceDesc
	{
		std::string  name;
		ResourceType type = ResourceType::Texture;

		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t mipLevels = 1;
		uint32_t arrayLayers = 1;

		core::gpu::utils::ETextureFormat format = core::gpu::utils::ETextureFormat::RGBA8_SRGB;
		core::gpu::utils::EImageTiling   tiling = core::gpu::utils::EImageTiling::Optimal;
		core::gpu::utils::EImageUsage    imageUsage = core::gpu::utils::EImageUsage::None;
		core::gpu::utils::ESampleCount   samples = core::gpu::utils::ESampleCount::e1;
		core::gpu::utils::ETextureFilter filter = core::gpu::utils::ETextureFilter::Linear;

		bool isDepthFormat = false;

		size_t size = 0;
		core::gpu::utils::EBufferUsage bufferUsage = core::gpu::utils::EBufferUsage::None;
		core::gpu::utils::EMemoryProperty memoryProperties = core::gpu::utils::EMemoryProperty::None;

		bool CanSatisfy(const ResourceDesc& _requested) const
		{
			if (type != _requested.type)
			{
				return false;
			}

			if (type == ResourceType::Buffer)
			{
				return size >= _requested.size &&
					(bufferUsage & _requested.bufferUsage) == _requested.bufferUsage &&
					memoryProperties == _requested.memoryProperties;
			}

			return width == _requested.width &&
				height == _requested.height &&
				mipLevels == _requested.mipLevels &&
				arrayLayers == _requested.arrayLayers &&
				format == _requested.format &&
				tiling == _requested.tiling &&
				(imageUsage & _requested.imageUsage) == _requested.imageUsage &&
				samples == _requested.samples &&
				filter == _requested.filter;
		}
	};

	struct Resource
	{
		std::string  name;
		ResourceDesc desc;

		core::gpu::Buffer* buffer = nullptr;
		core::gpu::Image* image = nullptr;
		core::gpu::Texture* texture = nullptr;

		ResourceState currentState = ResourceState::Undefined;
	};

	struct PassInput
	{
		ResourceHandle handle = kInvalidResourceHandle;
		ResourceState  requiredState = ResourceState::ShaderRead;
	};

	struct PassOutput
	{
		ResourceHandle handle = kInvalidResourceHandle;
		ResourceState  requiredState = ResourceState::RenderTarget;
	};

	struct PassCreateInfo
	{
		std::string name;

		std::vector<PassInput>  inputs;
		std::vector<PassOutput> outputs;
	};

	struct ResourceBarrier
	{
		Resource* resource = nullptr;
		ResourceState from = ResourceState::Undefined;
		ResourceState to = ResourceState::Undefined;
	};

	class Pass
	{
	private:
		std::string m_name;

		std::vector<PassInput>  m_inputs;
		std::vector<PassOutput> m_outputs;
	public:
		explicit Pass(PassCreateInfo createInfo)
			: m_name(std::move(createInfo.name))
			, m_inputs(std::move(createInfo.inputs))
			, m_outputs(std::move(createInfo.outputs))
		{
		}

		virtual ~Pass() = default;

		const std::string& GetName() const { return m_name; }
		const std::vector<PassInput>& GetInputs() const { return m_inputs; }
		const std::vector<PassOutput>& GetOutputs() const { return m_outputs; }

		virtual void Execute() = 0;
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_PASS_H