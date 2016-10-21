#include <vulkan/vulkan.hpp>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std::tr2;

VkBool32 messageCallback(VkDebugReportFlagsEXT flags,
						 VkDebugReportObjectTypeEXT objType,
						 uint64_t srcObject,
						 size_t location,
						 int32_t msgCode,
						 const char* pLayerPrefix,
						 const char* pMsg,
						 void* pUserData) 
{
	std::string message;
	{
		std::stringstream buf;
		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) 
		{
			buf << "ERROR: ";
		}
		else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) 
		{
			buf << "WARNING: ";
		}
		else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) 
		{
			buf << "PERF: ";
		}
		else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		{
			buf << "INFO: ";
		}
		else 
		{
			return false;
		}
		buf << "[" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg;
		message = buf.str();
	}

	std::cout << message << std::endl;
#ifdef _MSC_VER 
	OutputDebugStringA(message.c_str());
	OutputDebugStringA("\n");
#endif
	return false;
}



int main(int argc, char* argv[])
{
	{
		char buffer[MAX_PATH];
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		std::string::size_type pos = std::string(buffer).find_last_of("\\/");
		std::string basepath = std::string(buffer).substr(0, pos);
		SetCurrentDirectoryA(basepath.c_str());
	}

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

	std::vector<const char*> instance_layer_names;
	instance_layer_names.push_back("VK_LAYER_GOOGLE_threading");
	instance_layer_names.push_back("VK_LAYER_LUNARG_param_checker");
	instance_layer_names.push_back("VK_LAYER_LUNARG_object_tracker");
	instance_layer_names.push_back("VK_LAYER_LUNARG_mem_tracker");
	instance_layer_names.push_back("VK_LAYER_LUNARG_device_limits");
	instance_layer_names.push_back("VK_LAYER_LUNARG_draw_state");
	instance_layer_names.push_back("VK_LAYER_LUNARG_image");
	instance_layer_names.push_back("VK_LAYER_LUNARG_swapchain");
	instance_layer_names.push_back("VK_LAYER_GOOGLE_unique_objects");
	//instance_layer_names.push_back("VK_LAYER_NV_nsight");
	//instance_layer_names.push_back("VK_LAYER_NV_optimus");
	
	
	// Vulkan instance
	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "TheFuture";
	appInfo.pEngineName = "TheFuture";
	appInfo.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME };

	// Enable surface extensions depending on os
	enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

	vk::InstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	if (enabledExtensions.size() > 0) 
	{
		instanceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	}
	if (instance_layer_names.size() > 0)
	{
		instanceCreateInfo.enabledLayerCount = instance_layer_names.size();
		instanceCreateInfo.ppEnabledLayerNames = instance_layer_names.data();
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
	vk::Extent2D size(1280, 720);
	window = glfwCreateWindow(size.width, size.height, "The Future, Now", NULL, NULL);
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

	if ((formatCount == 1) && (surfaceFormats[0].format == vk::Format::eUndefined)) 
	{
		colorFormat = vk::Format::eB8G8R8A8Unorm;
	}
	else 
	{
		colorFormat = surfaceFormats[0].format;
	}
	colorSpace = surfaceFormats[0].colorSpace;


	uint32_t queueIndex = 0;
	
	std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
	uint32_t queueCount = queueProps.size();

	for (queueIndex = 0; queueIndex < queueCount; queueIndex++) 
	{
		if (queueProps[queueIndex].queueFlags & (vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eGraphics))
		{
			break;
		}
	}
	assert(queueIndex < queueCount);
	

	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = VK_NULL_HANDLE;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = VK_NULL_HANDLE;
	PFN_vkDebugReportMessageEXT dbgBreakCallback = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT msgCallback;

	{
		// Vulkan logical device
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

		// DEBUGGING
		CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		dbgBreakCallback = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");

		vk::DebugReportFlagsEXT flags(vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning |  vk::DebugReportFlagBitsEXT::ePerformanceWarning);
		VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
		dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		dbgCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)messageCallback;
		dbgCreateInfo.flags = flags.operator VkSubpassDescriptionFlags();

		VkResult err = CreateDebugReportCallback(instance, &dbgCreateInfo, nullptr,	&msgCallback);
		assert(!err);
	
		vk::Queue computeQueue = device.getQueue(queueIndex, 0);

		vk::SwapchainKHR swapChain;
		
		vk::SurfaceCapabilitiesKHR surfCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

		auto presentModeCount = presentModes.size();

		vk::Extent2D swapchainExtent;
		
		if (surfCaps.currentExtent.width == -1) 
		{
			swapchainExtent = size;
		}
		else 
		{
			swapchainExtent = surfCaps.currentExtent;
			size = surfCaps.currentExtent;
		}

		vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;
		{
			for (size_t i = 0; i < presentModeCount; i++) 
			{
				if (presentModes[i] == vk::PresentModeKHR::eMailbox) 
				{
					swapchainPresentMode = vk::PresentModeKHR::eMailbox;
					break;
				}
				if ((swapchainPresentMode != vk::PresentModeKHR::eMailbox) && (presentModes[i] == vk::PresentModeKHR::eImmediate)) 
				{
					swapchainPresentMode = vk::PresentModeKHR::eImmediate;
				}
			}
		}

		uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount;
		if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount)) 
		{
			desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
		}

		vk::SurfaceTransformFlagBitsKHR preTransform;
		if (surfCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) 
		{
			preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		}
		else 
		{
			preTransform = surfCaps.currentTransform;
		}

		VkBool32 isSupported;
		physicalDevice.getSurfaceSupportKHR(queueIndex, surface, &isSupported);
		assert(isSupported == true);
	
		vk::SwapchainCreateInfoKHR swapchainCI;
		swapchainCI.surface = surface;
		swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
		swapchainCI.imageFormat = colorFormat;
		swapchainCI.imageColorSpace = colorSpace;
		swapchainCI.imageExtent = vk::Extent2D{ swapchainExtent.width, swapchainExtent.height };
		swapchainCI.imageUsage = vk::ImageUsageFlagBits::eStorage;
		swapchainCI.preTransform = preTransform;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageSharingMode = vk::SharingMode::eExclusive;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.pQueueFamilyIndices = NULL;
		swapchainCI.presentMode = swapchainPresentMode;
		swapchainCI.clipped = true;
		swapchainCI.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapChain = device.createSwapchainKHR(swapchainCI);


		vk::ImageViewCreateInfo colorAttachmentViewInfo;
		colorAttachmentViewInfo.format = colorFormat;
		colorAttachmentViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		colorAttachmentViewInfo.subresourceRange.levelCount = 1;
		colorAttachmentViewInfo.subresourceRange.layerCount = 1;
		colorAttachmentViewInfo.viewType = vk::ImageViewType::e2D;

		// Get the swap chain images
		auto swapChainImages = device.getSwapchainImagesKHR(swapChain);
		uint32_t imageCount = (uint32_t)swapChainImages.size();

		// Get the swap chain buffers containing the image and imageview
		struct SwapChainImage 
		{
			vk::Image image;
			vk::ImageView view;
			vk::Fence fence;
		};

		std::vector<SwapChainImage> images;
		images.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; i++) 
		{
			images[i].image = swapChainImages[i];
			colorAttachmentViewInfo.image = swapChainImages[i];
			images[i].view = device.createImageView(colorAttachmentViewInfo);
		}

		vk::PresentInfoKHR presentInfo;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;

		vk::CommandPool commandPool;
		{
			vk::CommandPoolCreateInfo commandPoolCreateInfo;
			commandPoolCreateInfo.queueFamilyIndex = queueIndex;
			commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
			commandPool = device.createCommandPool(commandPoolCreateInfo);
		}

		std::vector<vk::CommandBuffer> commandBuffers;

		// Allocate command buffers
		{
			vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
			commandBufferAllocateInfo.commandPool = commandPool;
			commandBufferAllocateInfo.commandBufferCount = imageCount;
			commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
			commandBuffers = device.allocateCommandBuffers(commandBufferAllocateInfo);
		}

		vk::Semaphore acquireCompleteSemaphore;
		vk::Semaphore renderCompleteSemaphore;
		{
			vk::SemaphoreCreateInfo info;
			acquireCompleteSemaphore = device.createSemaphore(info);
			renderCompleteSemaphore = device.createSemaphore(info);
		}

		vk::PipelineCache pipelineCache;
		{
			vk::PipelineCacheCreateInfo info;
			info.initialDataSize = 0;
			pipelineCache = device.createPipelineCache(info);
		}

		// load the compute module
		vk::ShaderModule test1;
		{
			std::ifstream f("test1.spv", std::ios::binary);
			std::vector<unsigned char> bin;
			uint32_t format;

			if (f.good())
			{
				f.seekg(0, f.end);
				int filesize = f.tellg();
				bin.resize(filesize);
				f.seekg(0, f.beg);
				f.read((char*)bin.data(), filesize);
				f.close();
			}
			else
			{
				throw std::system_error(std::error_code(), "couldn't load spir-v module");
			}
			vk::ShaderModuleCreateInfo info;
			info.codeSize = bin.size();
			info.pCode = (const uint32_t*)(bin.data());
			test1 = device.createShaderModule(info);
		}

		vk::PipelineLayout test1Layout;
		vk::DescriptorSetLayout test1DescSetLayout;
		{
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings = {
				vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
			};
			vk::DescriptorSetLayoutCreateInfo desclayoutinfo;
			desclayoutinfo.bindingCount = setLayoutBindings.size();
			desclayoutinfo.pBindings = setLayoutBindings.data();

			test1DescSetLayout = device.createDescriptorSetLayout(desclayoutinfo);

			vk::PushConstantRange range;
			range.stageFlags = vk::ShaderStageFlagBits::eCompute;
			range.offset = 0;
			range.size = 4 * sizeof(float);
			
			vk::PipelineLayoutCreateInfo info;
			info.pushConstantRangeCount = 1;
			info.pPushConstantRanges = &range;
			info.setLayoutCount = 1;
			info.pSetLayouts = &test1DescSetLayout;
			test1Layout = device.createPipelineLayout(info);
		}

		// pipeline 
		vk::Pipeline computePipeline;
		{
			vk::PipelineShaderStageCreateInfo computeStageInfo;
			computeStageInfo.module = test1;
			computeStageInfo.pName = "main";
			computeStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
			
			
			vk::ComputePipelineCreateInfo info;
			info.layout = test1Layout;
			info.stage = computeStageInfo;

			computePipeline = device.createComputePipeline(pipelineCache, info);
		}

		// descriptor pool
		vk::DescriptorPool descriptorPool;
		{
			std::vector<vk::DescriptorPoolSize> poolSizes =
			{
				//vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 4),
				//vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 4),
				//vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 4),
				vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 2),
				//vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 4),
				//vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 4),
				//vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 4),
				//vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 4),
				//vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 4),
				//vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 4),
				//vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 4)
			};
			vk::DescriptorPoolCreateInfo descriptorPoolInfo;
			descriptorPoolInfo.maxSets = 16;
			descriptorPoolInfo.poolSizeCount = poolSizes.size();
			descriptorPoolInfo.pPoolSizes = poolSizes.data();
			descriptorPool = device.createDescriptorPool(descriptorPoolInfo);
		}

		// create descriptor sets for both swap chain images
		std::vector<vk::DescriptorSet> descriptorSets;
		{
			vk::DescriptorSetAllocateInfo allocInfo(descriptorPool, 1, &test1DescSetLayout);
			descriptorSets.push_back(device.allocateDescriptorSets(allocInfo)[0]);
			descriptorSets.push_back(device.allocateDescriptorSets(allocInfo)[0]);
		}

		for (int i = 0; i < imageCount; i++)
		{
			vk::DescriptorImageInfo imginfo(nullptr, images[i].view, vk::ImageLayout::eGeneral);
			vk::WriteDescriptorSet writeSet(descriptorSets[i], 0, 0, 1, vk::DescriptorType::eStorageImage, &imginfo);
			device.updateDescriptorSets(1, &writeSet, 0, nullptr);
		}

		// command buffer submission
		vk::SubmitInfo submitInfo;
		vk::PipelineStageFlags stageFlags = vk::PipelineStageFlagBits::eBottomOfPipe;
		submitInfo.pWaitDstStageMask = &stageFlags;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &acquireCompleteSemaphore;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompleteSemaphore;

		// commandBuffers
		vk::CommandBufferBeginInfo cmdBufInfo;
		cmdBufInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

		int c = 0;
		for (auto& computeCmdBuffer : commandBuffers)
		{
			std::vector<float> pushConstantValues = { 1280.0f, 720.0f, 0.0f, 0.0f };
			computeCmdBuffer.begin(cmdBufInfo);
			computeCmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
			computeCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, test1Layout, 0, descriptorSets[c], nullptr);
			computeCmdBuffer.pushConstants<float>(test1Layout, vk::ShaderStageFlagBits::eCompute, 0, pushConstantValues);
			computeCmdBuffer.dispatch(1280 / 16, 720 / 16, 1);
			computeCmdBuffer.end();
			c++;
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		uint32_t currentImage;
		while (glfwWindowShouldClose(window) == false)
		{
			glfwPollEvents();
			float time = glfwGetTime();

			// aquire next swapchain image
			auto resultValue = device.acquireNextImageKHR(swapChain, UINT64_MAX, acquireCompleteSemaphore, vk::Fence());
			vk::Result result = resultValue.result;
			if (result != vk::Result::eSuccess) 
			{
				throw std::error_code(result);
			}
			currentImage = resultValue.value;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &(commandBuffers[currentImage]);
			computeQueue.submit(submitInfo, vk::Fence());

			presentInfo.pImageIndices = &currentImage;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
			computeQueue.presentKHR(presentInfo);
		}
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////


		// cleanup
		DestroyDebugReportCallback(instance, msgCallback, nullptr);

		for (uint32_t i = 0; i < imageCount; i++)
		{
			device.destroyImageView(images[i].view);
		}
		device.destroyCommandPool(commandPool);
		device.destroySwapchainKHR(swapChain);
		device.destroy();
	}
	
	instance.destroy();
	return 0;
}

