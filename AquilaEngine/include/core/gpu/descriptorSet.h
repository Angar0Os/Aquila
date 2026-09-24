#ifndef AQUILA_ENGINE_CORE_GPU_DESCRIPTOR_SET_H
#define AQUILA_ENGINE_CORE_GPU_DESCRIPTOR_SET_H
#pragma once

#include <memory>

namespace core::gpu
{
	class AccelerationStructure;
	class Buffer;
	class Device;
	class DescriptorSetLayout;
	class Image;
	class Texture;

	class DescriptorSet
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	public:
		DescriptorSet(const Device& _device, const DescriptorSetLayout& _dsLayout);
		~DescriptorSet();

		template<typename T>
		void Bind(uint32_t _binding, const T& _input);

		void BindArray(uint32_t _binding, uint32_t _arrayElement, const Texture& _texture);


		void Update(const Device& _device);

		Impl& GetImpl() const;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_DESCRIPTOR_SET_H
