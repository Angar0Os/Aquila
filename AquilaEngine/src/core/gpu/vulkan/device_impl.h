#ifndef AQUILA_ENGINE_CORE_GPU_VULKAN_DEVICE_H
#define AQUILA_ENGINE_CORE_GPU_VULKAN_DEVICE_H
#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <core/gpu/device.h>
#include <core/gpu/utils/enums.h>

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

namespace core { class Window; }

namespace core::gpu
{
	struct Device::Impl
	{
		explicit Impl(const Window& _wnd);
		~Impl() noexcept;

		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapchain();

		vk::SurfaceFormatKHR	ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& _availableFormats, utils::ETextureFormat _preferredFormat);
		vk::PresentModeKHR		ChoosePresentMode(const std::vector<vk::PresentModeKHR>& _availableModes, utils::EPresentMode _preferredMode);
		vk::Extent2D			ChooseExtent(const vk::SurfaceCapabilitiesKHR& _capabilities, uint32_t _width, uint32_t _height);

		vk::raii::Context					context;
		vk::raii::Instance					instance		= nullptr;
		vk::raii::DebugUtilsMessengerEXT	debugMessenger	= nullptr;
		vk::raii::SurfaceKHR				surface			= nullptr;
		vk::raii::PhysicalDevice			physicalDevice	= nullptr;
		vk::raii::Device					device			= nullptr;
		vk::raii::Queue						graphicsQueue	= nullptr;
		uint32_t							queueIndex		= ~0;

		vk::raii::SwapchainKHR				swapchain = nullptr;
		std::vector<vk::raii::ImageView>    swapchainImageViews;
		vk::Extent2D						swapchainExtent;
		//std::vector<std::unique_ptr<Image>>	swapchainImages; // TODO : Ajouter la RHI image.
		vk::Format							swapchainImageFormat;

		const core::Window& window;

		std::vector<const char*> requiredDeviceExtension = {
			vk::KHRSwapchainExtensionName,
			vk::KHRSpirv14ExtensionName,
			vk::KHRSynchronization2ExtensionName,
			vk::KHRCreateRenderpass2ExtensionName,
			vk::KHRComputeShaderDerivativesExtensionName,

			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_KHR_RAY_QUERY_EXTENSION_NAME,
			VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
			VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
		};
	};
}

#endif //AQUILA_ENGINE_CORE_GPU_VULKAN_DEVICE_H
