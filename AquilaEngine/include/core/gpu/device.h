#ifndef AQUILA_ENGINE_CORE_GPU_DEVICE_H
#define AQUILA_ENGINE_CORE_GPU_DEVICE_H
#pragma once

#include <memory>

namespace core { class Window; }

namespace core::gpu
{
	class Device
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	public:
		explicit Device(const Window& _wnd);
		~Device() noexcept;

		static constexpr uint32_t FRAMES_IN_FLIGHT = 2;
		
		Impl& GetImpl() const;
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_DEVICE_H
