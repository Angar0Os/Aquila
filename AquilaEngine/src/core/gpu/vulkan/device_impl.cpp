#define NOMINMAX

#include "device_impl.h"
#include "commandPool_impl.h"
#include "commandBuffer_impl.h"
#include "descriptorPool_impl.h"
#include "image_impl.h"

#include <core/window.h>
#include <core/gpu/utils/converters.h>

#include <GLFW/glfw3.h>

#include <ranges>
#include <iostream>

using namespace core::gpu;

const std::vector<char const*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

std::vector<const char*> GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (enableValidationLayers)
	{
		extensions.push_back(vk::EXTDebugUtilsExtensionName);
	}

	return extensions;
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT _severity,
	vk::DebugUtilsMessageTypeFlagsEXT _type,
	const vk::DebugUtilsMessengerCallbackDataEXT*
	_pCallbackData, void*)
{
	std::cerr << "validation layer: type " << to_string(_type) << " msg: " << _pCallbackData->pMessage << std::endl;
	return vk::False;
}

vk::SurfaceFormatKHR Device::Impl::ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& _availableFormats, utils::ETextureFormat _preferredFormat)
{
	vk::Format vkPreferredFormat = utils::ToVulkan(_preferredFormat);

	for (const auto& format : _availableFormats)
	{
		if (format.format == vkPreferredFormat &&
			format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return format;
		}
	}

	for (const auto& format : _availableFormats)
	{
		if (format.format == vk::Format::eB8G8R8A8Srgb &&
			format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return format;
		}
	}

	return _availableFormats[0];
}

vk::PresentModeKHR Device::Impl::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& _availableModes, utils::EPresentMode _preferredMode)
{
	vk::PresentModeKHR vkPreferredMode = utils::ToVulkan(_preferredMode);

	for (const auto& mode : _availableModes)
	{
		if (mode == vkPreferredMode)
		{
			return mode;
		}
	}

	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Device::Impl::ChooseExtent(const vk::SurfaceCapabilitiesKHR& _capabilities, uint32_t _width, uint32_t _height)
{
	if (_capabilities.currentExtent.width != 0xFFFFFFFF)
	{
		return _capabilities.currentExtent;
	}

	vk::Extent2D actualExtent = { _width, _height };

	actualExtent.width = std::clamp(actualExtent.width,
		_capabilities.minImageExtent.width,
		_capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height,
		_capabilities.minImageExtent.height,
		_capabilities.maxImageExtent.height);

	return actualExtent;
}

Device::Device(const Window& _wnd)
{
	m_impl = std::make_unique<Impl>(_wnd, *this);

	m_impl->CreateInstance();
	m_impl->SetupDebugMessenger();
	m_impl->CreateSurface();
	m_impl->PickPhysicalDevice();
	m_impl->CreateLogicalDevice();

	m_impl->CreateSwapchain();

	m_impl->CreateDescriptorPool();
	m_impl->CreateCommandPool();

	m_impl->CreateSyncObjects();
}

Device::~Device()
{
	m_impl->device.waitIdle();
}

void Device::Impl::CreateInstance()
{
	vk::ApplicationInfo appInfo
	{
		.pApplicationName = window.GetAppName().c_str(),
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = vk::ApiVersion14
	};

	std::vector<char const*> requiredLayers;
	if (enableValidationLayers)
	{
		requiredLayers.assign(validationLayers.begin(), validationLayers.end());
	}

	auto layerProperties = context.enumerateInstanceLayerProperties();
	if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const& requiredLayer) {
		return std::ranges::none_of(layerProperties,
			[requiredLayer](auto const& layerProperty)
			{ return strcmp(layerProperty.layerName, requiredLayer) == 0; });
		}))
	{
		throw std::runtime_error("One or more required layers are not supported!");
	}

	auto requiredExtensions = GetRequiredExtensions();

	auto extensionProperties = context.enumerateInstanceExtensionProperties();
	for (auto const& requiredExtension : requiredExtensions)
	{
		if (std::ranges::none_of(extensionProperties,
			[requiredExtension](auto const& extensionProperty)
			{ return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
		{
			throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
		}
	}

	vk::InstanceCreateInfo createInfo
	{
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
		.ppEnabledLayerNames = requiredLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
		.ppEnabledExtensionNames = requiredExtensions.data()
	};

	instance = vk::raii::Instance(context, createInfo);
}

void Device::Impl::SetupDebugMessenger()
{
	if (!enableValidationLayers) return;

	vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

	vk::DebugUtilsMessageTypeFlagsEXT    messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

	vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT
	{
		.messageSeverity = severityFlags,
		.messageType = messageTypeFlags,
		.pfnUserCallback = &DebugCallback
	};

	debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

void Device::Impl::CreateSurface()
{
	GLFWwindow* glfwWindow = window.GetHandle();
	if (!glfwWindow)
	{
		throw std::runtime_error("Invalid GLFW window handle!");
	}

	VkSurfaceKHR _surface;

	VkResult result = glfwCreateWindowSurface(*instance, glfwWindow, nullptr, &_surface);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface. Error code: " + std::to_string(result));
	}

	surface = vk::raii::SurfaceKHR(instance, _surface);
}

void Device::Impl::PickPhysicalDevice()
{
	std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

	vk::raii::PhysicalDevice* bestDevice = nullptr;
	int bestScore = -1;

	for (auto& device : devices)
	{
		auto props = device.getProperties();
		int score = 0;

		auto queueFamilies = device.getQueueFamilyProperties();
		bool supportsGraphics = std::ranges::any_of(queueFamilies,
			[](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

		auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
		bool supportsAllRequiredExtensions = true;

		std::vector<const char*> requiredExtensions = {
			vk::KHRSwapchainExtensionName,
			vk::KHRSpirv14ExtensionName,
			vk::KHRSynchronization2ExtensionName,
			vk::KHRCreateRenderpass2ExtensionName
		};

		for (const auto& requiredExt : requiredExtensions)
		{
			bool found = std::ranges::any_of(availableDeviceExtensions,
				[requiredExt](auto const& availableExt)
				{ return strcmp(availableExt.extensionName, requiredExt) == 0; });

			if (!found)
			{
				supportsAllRequiredExtensions = false;
				break;
			}
		}

		if (!supportsAllRequiredExtensions) continue;

		auto basicFeatures = device.template getFeatures2
			<vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

		bool samplerAniso = basicFeatures.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy;
		bool dynRender = basicFeatures.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering;
		bool extDynState = basicFeatures.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

		std::vector<const char*> rtExtensions = {
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_KHR_RAY_QUERY_EXTENSION_NAME,
			VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
			VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
		};

		bool supportsAllRTExtensions = true;
		for (const auto& rtExt : rtExtensions)
		{
			bool found = std::ranges::any_of(availableDeviceExtensions,
				[rtExt](auto const& availableExt)
				{ return strcmp(availableExt.extensionName, rtExt) == 0; });

			if (!found)
			{
				supportsAllRTExtensions = false;
				break;
			}
		}

		if (!supportsAllRTExtensions) continue;

		auto rtFeatures = device.template getFeatures2
			<vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceBufferDeviceAddressFeatures,
			vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
			vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
			vk::PhysicalDeviceRayQueryFeaturesKHR>();

		bool bufferAddr = rtFeatures.template get<vk::PhysicalDeviceBufferDeviceAddressFeatures>().bufferDeviceAddress;
		bool accelStruct = rtFeatures.template get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>().accelerationStructure;
		bool rtPipeline = rtFeatures.template get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>().rayTracingPipeline;
		bool rayQuery = rtFeatures.template get<vk::PhysicalDeviceRayQueryFeaturesKHR>().rayQuery;

		if (!bufferAddr || !accelStruct || !rayQuery)
			continue;

		if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			score += 1000;
		else if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
			score += 100;

		score += props.limits.maxImageDimension2D / 1000;

		if (score > bestScore)
		{
			bestScore = score;
			bestDevice = &device;
		}
	}

	if (bestDevice)
	{
		auto props = bestDevice->getProperties();
		physicalDevice = *bestDevice;
	}
	else
	{
		throw std::runtime_error("Failed to find a suitable GPU with raytracing support!");
	}
}

void Device::Impl::CreateLogicalDevice()
{
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

	for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); ++qfpIndex)
	{
		if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
			physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
		{
			queueIndex = qfpIndex;
			break;
		}
	}

	if (queueIndex == ~0)
	{
		throw std::runtime_error("Could not find a queue for graphics and present");
	}

	vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
	rayQueryFeatures.rayQuery = VK_TRUE;

	vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{};
	rtPipelineFeatures.rayTracingPipeline = VK_TRUE;
	rtPipelineFeatures.pNext = &rayQueryFeatures;

	vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures{};
	accelFeatures.accelerationStructure = VK_TRUE;
	accelFeatures.pNext = &rtPipelineFeatures;

	vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
	bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
	bufferDeviceAddressFeatures.pNext = &accelFeatures;

	vk::StructureChain featureChain = {
		vk::PhysicalDeviceFeatures2 {.features = {.samplerAnisotropy = true} },
		vk::PhysicalDeviceVulkan11Features {.shaderDrawParameters = true},
		vk::PhysicalDeviceVulkan13Features {.synchronization2 = true, .dynamicRendering = true},
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT {.extendedDynamicState = true},
		vk::PhysicalDeviceComputeShaderDerivativesFeaturesKHR {.computeDerivativeGroupQuads = true }
	};

	featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().pNext = &bufferDeviceAddressFeatures;

	float queuePriority = 1.0f;
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
		.queueFamilyIndex = queueIndex,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	};

	vk::DeviceCreateInfo deviceCreateInfo{
		.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &deviceQueueCreateInfo,
		.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
		.ppEnabledExtensionNames = requiredDeviceExtension.data()
	};

	device = vk::raii::Device(physicalDevice, deviceCreateInfo);
	graphicsQueue = vk::raii::Queue(device, queueIndex, 0);

	auto rtProps = physicalDevice.getProperties2
		<vk::PhysicalDeviceProperties2,
		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

	const auto& rtPipelineProps = rtProps.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
}

void Device::Impl::CreateSwapchain()
{
	vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
	auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
	auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

	vk::SurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(surfaceFormats, utils::ETextureFormat::RGBA8_SRGB);
	vk::PresentModeKHR presentMode = ChoosePresentMode(presentModes, utils::EPresentMode::Fifo);
	vk::Extent2D extent = ChooseExtent(capabilities, 0, 0);

	uint32_t imageCount = std::max(2u, capabilities.minImageCount);
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
	{
		imageCount = capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo{
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = presentMode,
		.clipped = vk::True,
		.oldSwapchain = nullptr
	};

	swapchain = vk::raii::SwapchainKHR(device, createInfo);
	swapchainExtent = extent;

	std::vector<vk::Image> vkImages = swapchain.getImages();
	swapchainImageViews.clear();
	swapchainImageViews.reserve(vkImages.size());

	for (vk::Image image : vkImages)
	{
		vk::ImageViewCreateInfo viewInfo{
			.image = image,
			.viewType = vk::ImageViewType::e2D,
			.format = surfaceFormat.format,
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
		swapchainImageViews.emplace_back(device, viewInfo);

		PredefinedImageCreateInfo imageInfo{};
		imageInfo.image = image;
		imageInfo.extent = extent;
		imageInfo.aspectFlags = vk::ImageAspectFlagBits::eColor;
		imageInfo.format = surfaceFormat.format;

		swapchainImages.emplace_back(std::make_unique<Image>(parent, imageInfo));
	}

	swapchainImageFormat = surfaceFormat.format;
}

void Device::Impl::CreateDescriptorPool()
{
	descriptorPool = std::make_unique<DescriptorPool>(parent);
}

void Device::Impl::CreateCommandPool()
{
	CommandPoolCreateInfo poolInfo{
		.queueFamilyIndex = queueIndex,
		.flags = utils::ECommandPoolCreateFlags::ResetCommandBuffer
	};

	commandPool = std::make_unique<CommandPool>(parent, poolInfo);
}

void Device::Impl::CreateSyncObjects()
{
	frameSyncObjects.clear();
	tempCmdBufs.clear();

	vk::SemaphoreCreateInfo semInfo{};
	vk::FenceCreateInfo fenceInfo{ .flags = vk::FenceCreateFlagBits::eSignaled };

	frameSyncObjects.reserve(Device::FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < Device::FRAMES_IN_FLIGHT; ++i)
	{
		FrameSync frameSync{
			.inFlightFence = vk::raii::Fence(device, fenceInfo),
			.imageAvailable = vk::raii::Semaphore(device, semInfo),
			.renderFinished = vk::raii::Semaphore(device, semInfo)
		};

		frameSyncObjects.push_back(std::move(frameSync));
	}

	uint32_t swapchainImageCount = swapchain.getImages().size();
	tempCmdBufs.resize(::Device::FRAMES_IN_FLIGHT);
}

void Device::Impl::RecreateSwapchain()
{
	needsResize = false;
	int width = 0, height = 0;
	glfwGetFramebufferSize(window.GetHandle(), &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window.GetHandle(), &width, &height);
		glfwWaitEvents();
	}

	device.waitIdle();

	swapchainImageViews.clear();
	swapchainImages.clear();

	vk::SwapchainKHR oldSwapchain = *swapchain;

	vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
	auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
	auto presentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

	vk::SurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(surfaceFormats, utils::ETextureFormat::RGBA8_SRGB);
	vk::PresentModeKHR   presentMode = ChoosePresentMode(presentModes, utils::EPresentMode::Mailbox);
	vk::Extent2D         extent = ChooseExtent(capabilities, width, height);

	uint32_t imageCount = std::max(3u, capabilities.minImageCount);
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		imageCount = capabilities.maxImageCount;

	vk::SwapchainCreateInfoKHR createInfo{
		.surface = *surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment
						  | vk::ImageUsageFlagBits::eTransferDst,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = presentMode,
		.clipped = vk::True,
		.oldSwapchain = oldSwapchain
	};

	swapchain = vk::raii::SwapchainKHR(device, createInfo);
	swapchainExtent = extent;
	swapchainImageFormat = surfaceFormat.format;

	std::vector<vk::Image> vkImages = swapchain.getImages();
	swapchainImageViews.reserve(vkImages.size());

	for (vk::Image image : vkImages)
	{
		vk::ImageViewCreateInfo viewInfo{
			.image = image,
			.viewType = vk::ImageViewType::e2D,
			.format = surfaceFormat.format,
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
		swapchainImageViews.emplace_back(device, viewInfo);

		PredefinedImageCreateInfo imageInfo{};
		imageInfo.image = image;
		imageInfo.extent = extent;
		imageInfo.aspectFlags = vk::ImageAspectFlagBits::eColor;
		imageInfo.format = surfaceFormat.format;
		swapchainImages.emplace_back(std::make_unique<Image>(parent, imageInfo));
	}

	CreateSyncObjects();
}

Image* Device::AcquireNextImage()
{
	m_impl->device.waitForFences(*m_impl->frameSyncObjects[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);
	m_impl->device.resetFences(*m_impl->frameSyncObjects[currentFrame].inFlightFence);

	if (currentFrame >= m_impl->frameSyncObjects.size())
	{
		return nullptr;
	}

	auto& frameSync = m_impl->frameSyncObjects[currentFrame];

	vk::ResultValue<uint32_t> result = m_impl->device.acquireNextImage2KHR(
		vk::AcquireNextImageInfoKHR{
			.swapchain = m_impl->swapchain,
			.timeout = UINT64_MAX,
			.semaphore = *frameSync.imageAvailable,
			.fence = m_impl->frameSyncObjects[currentFrame].inFlightFence,
			.deviceMask = 1
		}
	);

	if (result.result == vk::Result::eErrorOutOfDateKHR)
	{
		return nullptr;
	}

	if (result.result != vk::Result::eSuccess &&
		result.result != vk::Result::eSuboptimalKHR)
	{
		return nullptr;
	}

	m_impl->currentImageIndex = result.value;
	return m_impl->swapchainImages[m_impl->currentImageIndex].get();
}

CommandBuffer* Device::AcquireCommandBuffer() const
{
	for (auto& [_, cb] : m_impl->commandBuffers)
	{
		bool isFree = cb->GetImpl().isCpuFree && (cb->GetImpl().isGpuFree.getStatus() == vk::Result::eSuccess);

		if (isFree)
		{
			cb->GetImpl().isCpuFree = false;
			return cb.get();
		}
	}

	auto cb = std::make_unique<CommandBuffer>(*this);
	cb->GetImpl().isCpuFree = false;

	auto cbPtr = cb.get();

	m_impl->commandBuffers[cbPtr] = std::move(cb);
	return cbPtr;
}

void Device::ReleaseCommandBuffer(CommandBuffer*& commandBuffer) const
{
	if (m_impl->commandBuffers.contains(commandBuffer))
	{
		commandBuffer->GetImpl().isCpuFree = true;
	}

	commandBuffer = nullptr;
}

std::pair<uint32_t, uint32_t> Device::GetSwapchainExtent() const
{
	return { m_impl->swapchainExtent.width, m_impl->swapchainExtent.height };
}

void Device::Present()
{
	if (currentFrame >= m_impl->frameSyncObjects.size()) return;

	auto& frameSync = m_impl->frameSyncObjects[currentFrame];
	vk::Semaphore presentWait = *frameSync.renderFinished;

	vk::PresentInfoKHR presentInfo{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &presentWait,
		.swapchainCount = 1,
		.pSwapchains = &*m_impl->swapchain,
		.pImageIndices = &m_impl->currentImageIndex
	};

	try
	{
		vk::Result result = m_impl->graphicsQueue.presentKHR(presentInfo);
		if (result == vk::Result::eSuboptimalKHR)
		{
			m_impl->needsResize = true;
		}
	}
	catch (const vk::OutOfDateKHRError&)
	{
		m_impl->needsResize = true;
	}

	++currentFrame %= m_impl->frameSyncObjects.size();
}

Device::Impl::Impl(const Window& _wnd, const Device& _device)
	: window(_wnd), parent(_device)
{
}

Device::Impl::~Impl()
{
	descriptorPool.reset();
}

Device::Impl& Device::GetImpl() const
{
	return *m_impl;
}