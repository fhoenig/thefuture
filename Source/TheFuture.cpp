#include <vulkan/vulkan.hpp>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>


int main(int argc, char* argv[])
{
	vk::Instance instance;

	std::vector<vk::PhysicalDevice> physicalDevices;
	// Physical device (GPU) that Vulkan will ise
	vk::PhysicalDevice physicalDevice;
	// Stores physical device properties (for e.g. checking device limits)
	vk::PhysicalDeviceProperties deviceProperties;
	// Stores phyiscal device features (for e.g. checking if a feature is available)
	vk::PhysicalDeviceFeatures deviceFeatures;
	// Stores all available memory (type) properties for the physical device
	vk::PhysicalDeviceMemoryProperties deviceMemoryProperties;

	// Vulkan instance
	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "TheFuture";
	appInfo.pEngineName = "TheFuture";
	appInfo.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

	// Enable surface extensions depending on os
	enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

	vk::InstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	if (enabledExtensions.size() > 0) 
	{
		instanceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	}
	instance = vk::createInstance(instanceCreateInfo);

	// Physical device
	physicalDevices = instance.enumeratePhysicalDevices();
	// Note :
	// This example will always use the first physical device reported,
	// change the vector index if you have multiple Vulkan devices installed
	// and want to use another one
	physicalDevice = physicalDevices[0];

	// Version information for Vulkan is stored in a single 32 bit integer
	// with individual bits representing the major, minor and patch versions.
	// The maximum possible major and minor version is 512 (look out nVidia)
	// while the maximum possible patch version is 2048

	// Store properties (including limits) and features of the phyiscal device
	// So examples can check against them and see if a feature is actually supported
	deviceProperties = physicalDevice.getProperties();
	uint32_t version = deviceProperties.apiVersion;
	uint32_t driverVersion = deviceProperties.driverVersion;
	deviceFeatures = physicalDevice.getFeatures();
	// Gather physical device memory properties
	deviceMemoryProperties = physicalDevice.getMemoryProperties();

	GLFWwindow* window;
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto monitor = glfwGetPrimaryMonitor();
	auto mode = glfwGetVideoMode(monitor);
	uint32_t screenWidth = mode->width;
	uint32_t screenHeight = mode->height;
	vk::Extent2D size(screenWidth / 2, screenHeight / 2);
	window = glfwCreateWindow(size.width, size.height, "The Future by Unbound", NULL, NULL);
	// Disable window resize
	glfwSetWindowSizeLimits(window, size.width, size.height, size.width, size.height);
	//glfwSetWindowUserPointer(window, this);
	//glfwSetWindowPos(window, 1280, 720);
	glfwShowWindow(window);


	// Create surface depending on OS
	vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo;
	surfaceCreateInfo.hinstance = GetModuleHandle(NULL);
	surfaceCreateInfo.hwnd = glfwGetWin32Window(window);
	vk::SurfaceKHR surface = instance.createWin32SurfaceKHR(surfaceCreateInfo);


	// Get list of supported surface formats
	std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
	auto formatCount = surfaceFormats.size();

	vk::Format colorFormat;
	vk::ColorSpaceKHR colorSpace;
	uint32_t queueNodeIndex = UINT32_MAX;

	// If the surface format list only includes one entry with  vk::Format::eUndefined,
	// there is no preferered format, so we assume  vk::Format::eB8G8R8A8Unorm
	if ((formatCount == 1) && (surfaceFormats[0].format == vk::Format::eUndefined)) {
		colorFormat = vk::Format::eB8G8R8A8Unorm;
	}
	else {
		// Always select the first available color format
		// If you need a specific format (e.g. SRGB) you'd need to
		// iterate over the list of available surface format and
		// check for it's presence
		colorFormat = surfaceFormats[0].format;
	}
	colorSpace = surfaceFormats[0].colorSpace;


	uint32_t queueIndex = 0;
	
	std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
	uint32_t queueCount = queueProps.size();

	for (queueIndex = 0; queueIndex < queueCount; queueIndex++) 
	{
		if (queueProps[queueIndex].queueFlags & vk::QueueFlagBits::eCompute)
		{
			break;
		}
	}
	assert(queueIndex < queueCount);

	{

		// Vulkan device
		vk::Device device;
		{
			std::array<float, 1> queuePriorities = { 0.0f };
			vk::DeviceQueueCreateInfo queueCreateInfo;
			queueCreateInfo.queueFamilyIndex = queueIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = queuePriorities.data();
			std::vector<const char*> enabledExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
			vk::DeviceCreateInfo deviceCreateInfo;
			deviceCreateInfo.queueCreateInfoCount = 1;
			deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
			deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
			if (enabledExtensions.size() > 0) 
			{
				deviceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
				deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
			}
			device = physicalDevice.createDevice(deviceCreateInfo);
		}
	
		vk::SwapchainKHR swapChain;
		vk::PresentInfoKHR presentInfo;

		// Get physical device surface properties and formats
		vk::SurfaceCapabilitiesKHR surfCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		// Get available present modes
		std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
		auto presentModeCount = presentModes.size();

		vk::Extent2D swapchainExtent;
		// width and height are either both -1, or both not -1.
		if (surfCaps.currentExtent.width == -1) {
			// If the surface size is undefined, the size is set to
			// the size of the images requested.
			swapchainExtent = size;
		}
		else {
			// If the surface size is defined, the swap chain size must match
			swapchainExtent = surfCaps.currentExtent;
			size = surfCaps.currentExtent;
		}

		// Prefer mailbox mode if present, it's the lowest latency non-tearing present  mode
		vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;
		{
			for (size_t i = 0; i < presentModeCount; i++) {
				if (presentModes[i] == vk::PresentModeKHR::eMailbox) {
					swapchainPresentMode = vk::PresentModeKHR::eMailbox;
					break;
				}
				if ((swapchainPresentMode != vk::PresentModeKHR::eMailbox) && (presentModes[i] == vk::PresentModeKHR::eImmediate)) {
					swapchainPresentMode = vk::PresentModeKHR::eImmediate;
				}
			}
		}

		// Determine the number of images
		uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
		if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount)) {
			desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
		}

		vk::SurfaceTransformFlagBitsKHR preTransform;
		if (surfCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
			preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		}
		else {
			preTransform = surfCaps.currentTransform;
		}

	
		vk::SwapchainCreateInfoKHR swapchainCI;
		swapchainCI.surface = surface;
		swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
		swapchainCI.imageFormat = colorFormat;
		swapchainCI.imageColorSpace = colorSpace;
		swapchainCI.imageExtent = vk::Extent2D{ swapchainExtent.width, swapchainExtent.height };
		swapchainCI.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
		swapchainCI.preTransform = preTransform;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageSharingMode = vk::SharingMode::eExclusive;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.pQueueFamilyIndices = NULL;
		swapchainCI.presentMode = swapchainPresentMode;
		swapchainCI.clipped = true;
		swapchainCI.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapChain = device.createSwapchainKHR(swapchainCI);


		while (glfwWindowShouldClose(window) == false)
		{
			glfwPollEvents();
			Sleep(10);
		}

		device.destroySwapchainKHR(swapChain);
		device.destroy();
	}
	
	instance.destroy();
	return 0;
}

