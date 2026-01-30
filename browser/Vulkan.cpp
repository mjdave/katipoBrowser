
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"



#include "Vulkan.h"


#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <set>
#include "MJLog.h"
#include "Timer.h"
#define NOMINMAX
#include "GCommandBuffer.h"
#include "MainController.h"
#include "MJRenderTarget.h"
#include "MJDrawQuad.h"
#include "FileUtils.h"
//#include "backward.hpp"

#define MAX_FRAMES_IN_FLIGHT 2

#define enableValidationLayers (IS_FOR_INTERNAL_DEVELOPMENT && 0)

#define VULKAN_ERROR_TITLE "Failed to initialize Vulkan"
#define VULKAN_SWAPCHAIN_ERROR_TITLE "Failed to submit Vulkan image"
#define VULKAN_ERROR_MESSAGE "Please update your graphics card driver, and also your CPU driver.\n\nThis error is often caused by a bug in old drivers for integrated chipsets,\nso if your graphics card driver is definitely up to date, please also download and\ninstall the most recent driver for your CPU from the manufacturer's website."

#define SHOW_ERROR_WINDOW(title, msg) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,VULKAN_ERROR_TITLE,msg,window);

#define VULKAN_ERROR(errorStuff) MJError("%s", errorStuff);  throw std::runtime_error(errorStuff);

#define LOG_VULKAN_TIMINGS 0
#define DIAGNOSTIC_CHECKPOINTS 0

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

bool checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				MJLog("Vulkan Validation found layer:%s", layerName);
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			MJLog("Vulkan Validation ERROR cannot find layer:%s", layerName);
			return false;
		}
	}

	return true;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}


#if DIAGNOSTIC_CHECKPOINTS

#pragma warning( push )//for void* cast
#pragma warning( disable : 4302 ) 
#pragma warning( disable : 4311 )
#pragma warning( disable : 4312 )

static PFN_vkCmdSetCheckpointNV setCheckpointNVFunc = nullptr;
static PFN_vkGetQueueCheckpointDataNV getQueueCheckpointDataNVFunc = nullptr;

void Vulkan::debugCommandBuffer(VkCommandBuffer commandBuffer, int value)
{
	if(!setCheckpointNVFunc)
	{
		setCheckpointNVFunc = (PFN_vkCmdSetCheckpointNV)vkGetInstanceProcAddr(instance, "vkCmdSetCheckpointNV");
	}

	setCheckpointNVFunc(commandBuffer, (void*)value);
}

void Vulkan::printDebugCommandBuffer(VkQueue queue)
{

	if(queue == transferQueue)
	{
		MJLog("printing debug for transfer queue")
	}
	else
	{
		MJLog("printing debug for graphics queue")
	}

	if(!getQueueCheckpointDataNVFunc)
	{
		getQueueCheckpointDataNVFunc = (PFN_vkGetQueueCheckpointDataNV)vkGetInstanceProcAddr(instance, "vkGetQueueCheckpointDataNV");
	}


	uint32_t count = 0;
	getQueueCheckpointDataNVFunc(queue, &count, NULL);
	if(count != 0)
	{
		MJLog("found checkpoints:%d", count);
		std::vector<VkCheckpointDataNV> checkpoints(count);

		for(auto& checkpoint : checkpoints)
		{
			checkpoint.sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
		}

		getQueueCheckpointDataNVFunc(queue, &count, checkpoints.data());

		for(auto& checkpoint : checkpoints)
		{
			MJLog("checkpoint:%d", (int)(checkpoint.pCheckpointMarker))
		}
	}
}
#pragma warning( pop )

#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
{
    if(messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        if(messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
			std::string message = pCallbackData->pMessage;
			MJLog("%s", message.c_str());
			MJError("validation layer error: %s", pCallbackData->pMessage);
			MJLog("%s", message.c_str());
			//exit(0);
        }
		else
		{
			MJWarn("validation layer: %s", pCallbackData->pMessage);
		}
    }
    return VK_FALSE;
}



void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

void Vulkan::setupDebugMessenger() {
	if (!enableValidationLayers)
	{
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

Vulkan::Vulkan(MainController* controller_, SDL_Window* window_, ivec2 screenSize, bool vsync_)
{
    controller = controller_;
    window = window_;
	vsync = vsync_;
	mainThreadID = std::this_thread::get_id();

	debugTimerA = new Timer();
	debugTimerB = new Timer();
	debugTimerC = new Timer();

    createInstance();
	setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createVMA();

	int sanityCount = 0;
	while(!createSwapChain())
	{
		MJWarn("Failed to create swapchain. Trying again soon.")
		sanityCount++;
		if(sanityCount > 100)
		{
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
    createImageViews();
    createRenderPass();
    createCommandPool();
    createColorResources();
    createFramebuffers();
    createCommandBuffers();
    createSyncObjects();
	hasValidSwapchain = true;


    std::vector<DrawQuadVertex> vertices = {
        {{0.0f, 0.0f}},
    {{0.0f, 1.0f}},
    {{1.0f, 1.0f}},

    {{1.0f, 1.0f}},
    {{1.0f, 0.0f}},
    {{0.0f, 0.0f}}
    };

    VkCommandBuffer tempCommandBuffer = beginSingleTimeCommands();
    setupStaticBuffer(tempCommandBuffer, &drawQuadVertBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(DrawQuadVertex) * vertices.size(), vertices.data(), "main output draw quad");
    endSingleTimeCommands(tempCommandBuffer);

	std::string logFilePath = getSavePath("memoryThread.log");
	memoryQueue = new ThreadSafeQueue<VulkanMemoryThreadInput>();
	memoryThread = new VulkanMemoryThread(memoryQueue, vmaAllocator, logFilePath);
}

Vulkan::~Vulkan()
{
	delete memoryThread;
}

int Vulkan::frameCount()
{
    return swapChainFramebuffers.size();
}

void Vulkan::createInstance()
{

    if (enableValidationLayers && !checkValidationLayerSupport()) {
        VULKAN_ERROR("validation layers requested, but not available!");
    }


	uint32_t availableInstanceExtensionCount = 0;
	if(vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, nullptr) != VK_SUCCESS )
	{
		MJLog("vkEnumerateInstanceExtensionProperties failed");
		SHOW_ERROR_WINDOW(VULKAN_ERROR_TITLE, VULKAN_ERROR_MESSAGE);
		VULKAN_ERROR("vkEnumerateInstanceExtensionProperties failed");
	}
	std::vector<VkExtensionProperties> availableInstanceExtensions(availableInstanceExtensionCount);
	if(availableInstanceExtensionCount > 0)
	{
		if(vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, availableInstanceExtensions.data()) != VK_SUCCESS)
		{
			MJLog("vkEnumerateInstanceExtensionProperties failed (b)");
			SHOW_ERROR_WINDOW(VULKAN_ERROR_TITLE, VULKAN_ERROR_MESSAGE);
			VULKAN_ERROR("vkEnumerateInstanceExtensionProperties failed (b)");
		}
	}

	for(const auto& extensionProperties : availableInstanceExtensions)
	{
		if(strcmp(extensionProperties.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
		{
			VK_KHR_get_physical_device_properties2_enabled = true;
		}
	}

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Ambience";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Ambience";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_MAKE_VERSION(1, 2, 0);

    unsigned int count;
    const char * const *instance_extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    if(count == 0){
        MJLog("SDL_Vulkan_GetInstanceExtensions failed (a) %s", SDL_GetError());
		SHOW_ERROR_WINDOW(VULKAN_ERROR_TITLE, VULKAN_ERROR_MESSAGE);
        VULKAN_ERROR("SDL_Vulkan_GetInstanceExtensions failed (a)");
    }

	std::vector<const char*> extensions;
    
    /*
     if (instance_extensions == NULL) { handle_error(); }

     int count_extensions = count_instance_extensions + 1;
     const char **extensions = SDL_malloc(count_extensions * sizeof(const char *));
     extensions[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
     SDL_memcpy(&extensions[1], instance_extensions, count_instance_extensions * sizeof(const char*));

     // Now we can make the Vulkan instance
     VkInstanceCreateInfo create_info = {};
     create_info.enabledExtensionCount = count_extensions;
     create_info.ppEnabledExtensionNames = extensions;
     */
    

	if(enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}


	if(VK_KHR_get_physical_device_properties2_enabled)
	{
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}
    
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    
    for(int i = 0; i < count; i++)
    {

        if(std::string(instance_extensions[i]) != "VK_KHR_portability_enumeration") //todo hacks around SDL3 on macos
        {
            extensions.push_back(instance_extensions[i]);
        } 
/*
#if __APPLE__ && !TARGET_OS_IPHONE
        if(std::string(instance_extensions[i]) != "VK_KHR_portability_enumeration") //todo hacks around SDL3 on macos
        {
            extensions.push_back(instance_extensions[i]);
        }
#else
        extensions.push_back(instance_extensions[i]);
#endif*/
        
    }

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

#if __APPLE__
     createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR; //for mac only?
#endif
    

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {


		/*VkValidationFeatureEnableEXT enables[] =
		{VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT};
		VkValidationFeaturesEXT features = {};
		features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		features.enabledValidationFeatureCount = 1;
		features.pEnabledValidationFeatures = enables;*/

        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
		//createInfo.pNext = &features;

    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		//SHOW_ERROR_WINDOW(VULKAN_ERROR_TITLE, VULKAN_ERROR_MESSAGE);
        VULKAN_ERROR("failed to create instance!");
        exit(0);
    }

}

/*
void Vulkan::setupDebugCallback() 
{
	MJLog("a");
    if (!enableValidationLayers) return;

	MJLog("b");
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
        VULKAN_ERROR("failed to set up debug callback!");
    }
	MJLog("c");
}*/

void Vulkan::createSurface()
{
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
		SHOW_ERROR_WINDOW(VULKAN_ERROR_TITLE, VULKAN_ERROR_MESSAGE);
        VULKAN_ERROR("SDL_Vulkan_CreateSurface failed");
    }
}

void Vulkan::destroySurface()
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

std::string getDriverVerson(VkPhysicalDeviceProperties physicalDeviceProperties)
{
		// NVIDIA
	if (physicalDeviceProperties.vendorID == 4318)
	{
		return string_format("%d.%d.%d.%d",
			(physicalDeviceProperties.driverVersion >> 22) & 0x3ff,
			(physicalDeviceProperties.driverVersion >> 14) & 0x0ff,
			(physicalDeviceProperties.driverVersion >> 6) & 0x0ff,
			(physicalDeviceProperties.driverVersion) & 0x003f
		);
	}
	if (physicalDeviceProperties.vendorID == 0x8086) //intel
	{
		return string_format("%d.%d",
			(physicalDeviceProperties.driverVersion >> 14),
			(physicalDeviceProperties.driverVersion) & 0x3fff
		);
	}
	// Use Vulkan version conventions if vendor mapping is not available
	return string_format("%d.%d.%d",
		(physicalDeviceProperties.driverVersion >> 22),
		(physicalDeviceProperties.driverVersion >> 12) & 0x3ff,
		physicalDeviceProperties.driverVersion & 0xfff);
}

void Vulkan::logDeviceDetails(VkPhysicalDevice physicalDeviceToLog, std::string prefix)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDeviceToLog, &physicalDeviceProperties);
	deviceName = physicalDeviceProperties.deviceName;
	deviceVendorID = physicalDeviceProperties.vendorID;
	driverVersion = physicalDeviceProperties.driverVersion;

	MJLog("%s: %s apiVersion:%d driverVersion:%s vendorID:%d", prefix.c_str(), deviceName.c_str(), physicalDeviceProperties.apiVersion, getDriverVerson(physicalDeviceProperties).c_str(), physicalDeviceProperties.vendorID);
}

void Vulkan::logMainDeviceDetails()
{
	logDeviceDetails(physicalDevice, "Using GPU");
}

void Vulkan::pickPhysicalDevice()
{
	physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
		SHOW_ERROR_WINDOW(VULKAN_ERROR_TITLE, VULKAN_ERROR_MESSAGE);
        VULKAN_ERROR("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    int bestScore = 0;
    for (const auto& device : devices)
    {
        logDeviceDetails(device, "Found available device");
        int score = getDeviceSuitabilityScore(device);
        if (score > bestScore)
        {
            physicalDevice = device;
            bestScore = score;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
		SHOW_ERROR_WINDOW(VULKAN_ERROR_TITLE, VULKAN_ERROR_MESSAGE);
        VULKAN_ERROR("failed to find a suitable GPU!");
    }
	else
	{
		logDeviceDetails(physicalDevice, "Chose graphics device");
	}
}



#ifdef __APPLE__
const std::vector<const char*> baseDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_MULTIVIEW_EXTENSION_NAME,
};
#else
const std::vector<const char*> baseDeviceExtensions = { //also add to createLogicalDevice
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
#endif



bool Vulkan::checkDeviceBaseExtensionSupport(VkPhysicalDevice device) 
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(baseDeviceExtensions.begin(), baseDeviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
		if(VK_KHR_get_physical_device_properties2_enabled)
		{
			if(strcmp(extension.extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0)
			{
				hasMemoryBudgetExtension = true;
				MJLog("Memory Budget extension available");
			}
		}
	}

    hasMemoryBudgetExtension = false;
	

	return requiredExtensions.empty();
}

int Vulkan::getDeviceSuitabilityScore(VkPhysicalDevice device) 
{
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceBaseExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

	if(indices.isComplete() && extensionsSupported && swapChainAdequate)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);
		if(physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			return 100;
		}
		else if(physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			return 10;
		}
		return 1;
	}
    else
    {
        MJLog("extensionsSupported:%d swapChainAdequate:%d", extensionsSupported, swapChainAdequate)
    }

    return 0;
}

void Vulkan::createLogicalDevice() 
{
	queueFamilyIndices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily, queueFamilyIndices.transferFamily};

    float highQueuePriority = 1.0f;
	float lowQueuePriority = 0.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = ((queueFamily == queueFamilyIndices.transferFamily) ? &lowQueuePriority : &highQueuePriority);
        queueCreateInfos.push_back(queueCreateInfo);
    }



    VkPhysicalDeviceFeatures deviceFeatures = {};
	VkPhysicalDeviceVulkan11Features features11 = {};

	features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	features11.multiview = VK_TRUE;

#ifndef __APPLE__
	deviceFeatures.wideLines = true;
#endif


    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.pNext = &features11;


	std::vector< std::string > requiredDeviceExtensions;
	requiredDeviceExtensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
#ifdef __APPLE__
    requiredDeviceExtensions.push_back( VK_KHR_MULTIVIEW_EXTENSION_NAME );
#endif

    //requiredDeviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    
	if(hasMemoryBudgetExtension)
	{
		requiredDeviceExtensions.push_back( VK_EXT_MEMORY_BUDGET_EXTENSION_NAME );
	}
    
    //mac only?
#ifdef __APPLE__
    const char* ugh = "VK_KHR_portability_subset";
    requiredDeviceExtensions.push_back(ugh);
#endif

#if DIAGNOSTIC_CHECKPOINTS
	requiredDeviceExtensions.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
#endif

	std::vector<const char*> requiredDeviceExtensionCStrings;
	for(int i = 0; i < requiredDeviceExtensions.size(); i++)
	{
		requiredDeviceExtensionCStrings.push_back(requiredDeviceExtensions[i].c_str());
	}

    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensionCStrings.size());
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensionCStrings.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		SHOW_ERROR_WINDOW(VULKAN_ERROR_TITLE, VULKAN_ERROR_MESSAGE);
        VULKAN_ERROR("failed to create logical device!");
    }

    vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamilyIndices.presentFamily, 0, &presentQueue);
	vkGetDeviceQueue(device, queueFamilyIndices.transferFamily, 0, &transferQueue);
}

void Vulkan::createVMA()
{
    /*VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
	allocatorInfo.instance = instance;*/

    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
    
    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    //allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_4;
    allocatorCreateInfo.physicalDevice = physicalDevice;
    allocatorCreateInfo.device = device;
    allocatorCreateInfo.instance = instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
	if(hasMemoryBudgetExtension)
	{
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	}

    vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator);
}

VkSampleCountFlagBits Vulkan::getMaxUsableSampleCount(int desiredCount) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
    if (desiredCount > 32 && counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (desiredCount > 16 && counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (desiredCount > 8 && counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (desiredCount > 4 && counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (desiredCount > 2 && counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (desiredCount > 1 && counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

bool Vulkan::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	if(extent.width < 40 || extent.height < 40)
	{
		MJError("window size too small, not supported: (%d,%d)", extent.width, extent.height)
		return false;
	}

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndicesToUse[] = {queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily};

    if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndicesToUse;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		SHOW_ERROR_WINDOW(VULKAN_ERROR_TITLE, VULKAN_ERROR_MESSAGE);
        VULKAN_ERROR("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

	return true;
}

void Vulkan::cleanupSwapChain() 
{
	if(!hasValidSwapchain)
	{
		return;
	}
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

   // vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    //vkDestroyPipeline(device, graphicsPipeline, nullptr);
    //vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
	hasValidSwapchain = false;
}

void Vulkan::recreateSwapChain()
{
	//MJLog("recreateSwapChain");
	int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);
	//MJLog("drawable size:(%d,%d)", width, height)
	if(width < 40 || height < 40)
	{
		MJError("Zero size window")
		return;
	}
    vkDeviceWaitIdle(device);

    cleanupSwapChain();
	destroySurface();
	createSurface();

    if(!createSwapChain())
	{
		return;
	}
    createImageViews();
    createRenderPass();

    createFramebuffers();
   // createCommandBuffers();

	controller->recreateDrawablesAndSaveSize();
	hasValidSwapchain = true;
}

void Vulkan::screenResolutionChanged(ivec2 screenSize)
{
	vkDeviceWaitIdle(device);

    //needsToRecreateSwapchain = true;
}


VkImageView Vulkan::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, int mipLevels, int arrayLayers, VkImageViewType imageViewType) 
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = imageViewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;//VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = arrayLayers;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        VULKAN_ERROR("failed to create texture image view!");
    }

    return imageView;
}

void Vulkan::createImageViews() 
{

    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}


bool Vulkan::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Vulkan::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels, int layerCount) 
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage = 99;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else {
        VULKAN_ERROR("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}



void Vulkan::copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

VkFormat Vulkan::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    VULKAN_ERROR("failed to find supported format!");
    return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
}

VkFormat Vulkan::findDepthFormat() 
{
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void Vulkan::createColorResources() 
{
    VkFormat colorFormat = swapChainImageFormat;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapChainExtent.width;
    imageInfo.extent.height = swapChainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = colorFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo finalImageBufferAllocInfo = {};
    finalImageBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalImageBufferAllocInfo.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	finalImageBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

	VMA_ALLOCATE_PROTECT(vmaCreateImage(vmaAllocator, &imageInfo, &finalImageBufferAllocInfo, &colorImage, &colorImageAllocation, nullptr));
    
    colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    VkCommandBuffer tempCommandBuffer = beginSingleTimeCommands();
    transitionImageLayout(tempCommandBuffer, colorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    endSingleTimeCommands(tempCommandBuffer);
}


void Vulkan::createRenderPass() 
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<VkAttachmentDescription> attachments(1);
    attachments[0] = colorAttachment;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        VULKAN_ERROR("failed to create render pass!");
    }

	finalOutputRenderPass.renderPass = renderPass;
	finalOutputRenderPass.extent = swapChainExtent;
	finalOutputRenderPass.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
}

void Vulkan::createFramebuffers() 
{
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::vector<VkImageView> attachments;


		attachments.resize(1);
		attachments[0] = swapChainImageViews[i];

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            VULKAN_ERROR("failed to create framebuffer!");
        }
    }
}


void Vulkan::createCommandPool() 
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        VULKAN_ERROR("failed to create command pool!");
    }
}

void Vulkan::createCommandBuffers() 
{
    primaryCommandBuffer = new GCommandBuffer(this);
}

void Vulkan::createSyncObjects() 
{
    imageAvailableSemaphores.resize(8);
    renderFinishedSemaphores.resize(8);
    inFlightFences.resize(8);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < 8; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            VULKAN_ERROR("failed to create synchronization objects for a frame!");
        }
    }

	if (vkCreateFence(device, &fenceInfo, nullptr, &transferFence) != VK_SUCCESS) {
		VULKAN_ERROR("failed to create transferFence");
	}
}

void Vulkan::destroyObjects()
{
    /*for(auto it = renderBuffersToDestroy.begin(); it != renderBuffersToDestroy.end();)
    {
        (*it).counter--;
        if ((*it).counter == 0) 
        {
            MJVMABuffer& renderBuffer = (*it).buffer;
			//MJLog("renderBuffersToDestroy destroySingleBuffer:%p", renderBuffer.buffer);
			//MJLog("final delet buffer:%p", renderBuffer.buffer);
            vmaDestroyBuffer(vmaAllocator, renderBuffer.buffer, renderBuffer.allocation);
            it = renderBuffersToDestroy.erase(it);
        }
        else 
        {
            ++it;
        }
    }*/

	if(!renderBuffersToDestroy.empty())
	{
		VulkanMemoryThreadInput input;
		input.type = VULKAN_MEMORY_THREAD_INPUT_TYPE_NORMAL;

		input.buffersToDestroy.buffers = renderBuffersToDestroy;
		input.buffersToDestroy.delayCounter = (uint32_t)frameCount()  * 2 + 1;

		memoryQueue->push(input);

		renderBuffersToDestroy.clear();
	}


    for(auto it = descripterPoolsToDestroy.begin(); it != descripterPoolsToDestroy.end();)
    {
        (*it).counter--;
        if ((*it).counter == 0) 
        {
			//MJLog("destroy:%p - %d", (*it).pool, (*it).desdtroyDebugCounter);
            vkDestroyDescriptorPool(device, (*it).pool, nullptr);
            it = descripterPoolsToDestroy.erase(it);
        }
        else 
        {
            ++it;
        }
    }
	for(auto it = renderTargetsToDestroy.begin(); it != renderTargetsToDestroy.end();)
	{
		(*it).counter--;
		if ((*it).counter == 0) 
		{
			delete ((*it).renderTarget);
			it = renderTargetsToDestroy.erase(it);
		}
		else 
		{
			++it;
		}
	}
	for(auto it = imagesToDestroy.begin(); it != imagesToDestroy.end();)
	{
		(*it).counter--;
		if ((*it).counter == 0) 
		{
			MJVMAImage& image = (*it).image;
			//MJLog("imageBuffersToDestroy destroySingleBuffer:%p", image.image);
			vmaDestroyImage(vmaAllocator, image.image, image.allocation);
			it = imagesToDestroy.erase(it);
		}
		else 
		{
			++it;
		}
	}
}

void Vulkan::submitTransfers()
{
	bool previousDone = waitingTransfers.empty();
#if LOG_VULKAN_TIMINGS
	MJLog("submitTransfers previousDone:%d", previousDone);
#endif

	if(!previousDone)
	{
		VkResult fenceResult = vkGetFenceStatus(device, transferFence);
#if LOG_VULKAN_TIMINGS
		MJLog("submitTransfers fenceResult:%d", fenceResult);
#endif
		if(fenceResult == VK_SUCCESS)
		{
			previousDone = true;

			for(uint32_t waitingTransfer : waitingTransfers)
			{
				TransferQueueOutput transferQueueOutput;
				transferQueueOutput.done = true;
				transferOutputQueues[waitingTransfer]->push(transferQueueOutput);
			}
			waitingTransfers.clear();
		}
	}

	if(previousDone)
	{
		std::vector<VkCommandBuffer> transferCommandBuffers;
		for(auto& indexAndQueue : transferInputQueues)
		{
			ThreadSafeQueue<TransferQueueInput>* inputQueue = indexAndQueue.second;
			TransferQueueInput transferQueueInput;
			bool found = false;
			while(!inputQueue->empty())
			{
				inputQueue->pop(transferQueueInput);
				found = true;
			}

			if(found)
			{
#if LOG_VULKAN_TIMINGS
				MJLog("adding transfer index:%d", indexAndQueue.first);
#endif
				transferCommandBuffers.push_back(transferQueueInput.commandBuffer);
				waitingTransfers.insert(indexAndQueue.first);
				//break;
			}
		}

		if(!transferCommandBuffers.empty())
		{
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = transferCommandBuffers.size();
			submitInfo.pCommandBuffers = transferCommandBuffers.data();

			VkResult resetResult = vkResetFences(device, 1, &transferFence);
#if LOG_VULKAN_TIMINGS
			MJLog("submitTransfers resetResult:%d", resetResult);
#endif

			VkResult result = vkQueueSubmit(transferQueue, 1, &submitInfo, transferFence);
#if LOG_VULKAN_TIMINGS
			MJLog("submitTransfers vkQueueSubmit:%d commandBufferCount:%d", result, transferCommandBuffers.size());
#endif
		}
	}
}

bool Vulkan::startRecord() 
{
	if(!hasValidSwapchain)
	{
		recreateSwapChain();

		if(!hasValidSwapchain)
		{
			return false;
		}
	}
	//debugTimer->getDt();
#if LOG_VULKAN_TIMINGS
	MJLog("startRecord:%.2f", (debugTimerA->getDt() * 1000.0));
#endif
	submitTransfers();
#if LOG_VULKAN_TIMINGS
	MJLog("submitTransfers done");
#endif

	debugTimerB->getDt();
	VkResult fencesResult = vkWaitForFences(device, 1, &inFlightFences[currentRecordingImageIndex], VK_TRUE, (std::numeric_limits<uint64_t>::max)());
#if LOG_VULKAN_TIMINGS
	MJLog("wait for fences:%.2f fencesResult:%d", (debugTimerB->getDt() * 1000.0), fencesResult);
#endif

	if(fencesResult == VK_ERROR_DEVICE_LOST)
	{
		MJError("vkWaitForFences VK_ERROR_DEVICE_LOST")
		VULKAN_ERROR("Device lost.");
		//printDebugCommandBuffer(graphicsQueue);
		//printDebugCommandBuffer(transferQueue);
	}

	//submitTransfers();


	if(needsToRecreateSwapchain)
	{
		recreateSwapChain();
        needsToRecreateSwapchain = false;
		//return false;
	}

	debugTimerB->getDt();
    VkResult result = vkAcquireNextImageKHR(device, swapChain, (std::numeric_limits<uint64_t>::max)(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &currentRecordingImageIndex);
#if LOG_VULKAN_TIMINGS
	MJLog("aquire:%.2f vkAcquireNextImageKHR result:%d", (debugTimerB->getDt() * 1000.0), result);
#endif

	debugTimerB->getDt();

	//debugTimer->getDt();

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VULKAN_ERROR("failed to acquire swap chain image!");
    }

    primaryCommandBuffer->startRecord(currentRecordingImageIndex);

	//MJLog("start record:%d-%d", currentRecordingImageIndex, currentFrame);
	return true;
}

void Vulkan::endRecord()
{
    primaryCommandBuffer->endRecord();

#if LOG_VULKAN_TIMINGS
	MJLog("record:%.2f", (debugTimerB->getDt() * 1000.0));
#endif

	//MJLog("endRecord:%d (%.4f)", (int)(recordTime * 1000.0), 1.0 / recordTime);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = primaryCommandBuffer->getBuffer();

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

	VkResult resetResult = vkResetFences(device, 1, &inFlightFences[currentRecordingImageIndex]);

#if LOG_VULKAN_TIMINGS
	MJLog("resetResult:%d currentFrame:%d", resetResult, currentFrame);
#endif
	//MJLog("a:%d", (int)(debugTimer->getDt() * 1000.0));


	//double submitTime = debugTimer->getDt();
	debugTimerC->getDt();
	VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentRecordingImageIndex]);
    if (result != VK_SUCCESS) {
		MJError("failed to submit draw command buffer:%d", result)
        VULKAN_ERROR("failed to submit draw command buffer!");

		//printDebugCommandBuffer(graphicsQueue);
		//printDebugCommandBuffer(transferQueue);
    }
#if LOG_VULKAN_TIMINGS
	MJLog("submit graphics queue:%.2f", (debugTimerC->getDt() * 1000.0));
#endif
	debugTimerC->getDt();
//	MJLog("b:%d", (int)(debugTimer->getDt() * 1000.0));

    
}

void Vulkan::submit()
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {swapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &currentRecordingImageIndex;
	//MJLog("present:%d-%d", currentRecordingImageIndex, currentFrame);

	debugTimerC->getDt();
	VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
#if LOG_VULKAN_TIMINGS
	MJLog("vkQueuePresentKHR:%.2f", (debugTimerC->getDt() * 1000.0));
#endif
	//int msPassed = (int)(debugTimer->getDt() * 1000.0);
	//MJLog("c:%d", msPassed);
	//if(msPassed < 3)
	{
		//	MJLog("quick submit");
	}

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapChain();
	} else if (result != VK_SUCCESS) {
		MJError("vkQueuePresentKHR failed. swapChainExtent:(%d,%d)", swapChainExtent.width, swapChainExtent.height);
		SHOW_ERROR_WINDOW(VULKAN_SWAPCHAIN_ERROR_TITLE, VULKAN_ERROR_MESSAGE);
		std::string errorMessage = string_format("failed to present swap chain image:%d", result);
		VULKAN_ERROR(errorMessage.c_str());
	}

	destroyObjects();
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	//submitTransfers();
	vmaSetCurrentFrameIndex(vmaAllocator, currentFrame);
    
//#ifdef WIN32
//	VmaBudget budgetBeg[VK_MAX_MEMORY_HEAPS] = {};
//	vmaGetBudget(vmaAllocator, budgetBeg);
//#else
	VmaBudget budgetBeg[VK_MAX_MEMORY_HEAPS] = {};
	vmaGetHeapBudgets(vmaAllocator, budgetBeg);
//#endif

	vmaBudget = budgetBeg[0];

	/*for(int i = 0; i < VK_MAX_MEMORY_HEAPS; i++)
	{
		VmaBudget budget = budgetBeg[i];
		MJLog("allocationBytes:%.2fMB blockBytes:%.2fMB, budget:%.2fMB, usage:%.2fMB", ((double)budget.allocationBytes) / (1024 * 1024), ((double)budget.blockBytes) / (1024 * 1024), ((double)budget.budget) / (1024 * 1024), ((double)budget.usage) / (1024 * 1024));
	}*/
}

QueueFamilyIndices Vulkan::findQueueFamilies(VkPhysicalDevice device) 
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) 
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
        {
            indices.graphicsFamily = i;
        }


		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) 
		{
			indices.transferFamily = i;
		}

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }

        i++;
    }


	i = 0;
	for (const auto& queueFamily : queueFamilies) 
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) //prioritize a dedicated transfer queue
		{
			indices.transferFamily = i;
		}
		i++;
	}

    return indices;
}

SwapChainSupportDetails Vulkan::querySwapChainSupport(VkPhysicalDevice device) 
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkExtent2D Vulkan::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
{
    if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
        return capabilities.currentExtent;
    } else {
        int width, height;

        SDL_GetWindowSizeInPixels(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}


VkSurfaceFormatKHR Vulkan::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

	/*for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT && availableFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT) {
			return availableFormat;
		}
	}*/

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Vulkan::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
    VkPresentModeKHR prefferedMode = (vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR);
    VkPresentModeKHR nextChoice = (vsync ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR);

    VkPresentModeKHR bestMode = VK_PRESENT_MODE_IMMEDIATE_KHR;//VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == prefferedMode) {
            return availablePresentMode;
        } else if (availablePresentMode == nextChoice) {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}

void Vulkan::createVMABuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateInfo allocInfo, VkBuffer* buffer, VmaAllocation* allocation, VmaAllocationInfo* allocationInfo)
{
	if(size == 0)
	{
		MJLog("ERROR: don't allocate zero sized buffers");
		return;
	}
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, buffer, allocation, allocationInfo);

    if (result != VK_SUCCESS) {
        VULKAN_ERROR("failed to createVMABuffer!");
    }
}

VkCommandBuffer Vulkan::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Vulkan::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Vulkan::copyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) 
{
    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void Vulkan::destroySingleBuffer(MJVMABuffer& buffer)
{
    if(buffer.buffer == nullptr)
    {
        MJLog("calling destroySingleBuffer on null buffer");
    }
    else
    {
		//MJLog("destory buffer:%p", buffer.buffer);
        /*if(destroyedBuffersDebug.count(buffer.buffer) != 0)
        {
            MJLog("double destroy");
        }
        destroyedBuffersDebug.insert(buffer.buffer);*/

        //MJVMABuffersToDestroy dataToDestroy = {buffer, (uint32_t)frameCount() * 2 + 1};
		//MJLog("destroy buffer:%p", &(buffer.buffer));
		//MJLogBacktrace();
        renderBuffersToDestroy.push_back(buffer);
    }
}


void Vulkan::destroySingleImage(MJVMAImage& image)
{
	if(image.image == nullptr)
	{
		MJLog("calling destroySingleImage on null image");
	}
	else
	{
		MJVMAImagesToDestroy dataToDestroy = {image, (uint32_t)frameCount()  * 2 + 1};
		imagesToDestroy.push_back(dataToDestroy);
	}
}

void Vulkan::destroyDynamicMultiBuffer(std::vector<MJVMABuffer>& buffers)
{
    for (size_t i = 0; i < buffers.size(); i++) {

		//MJLog("destroyDynamicMultiBuffer destroySingleBuffer:%p", buffers[i].buffer);
        destroySingleBuffer(buffers[i]);
    }
}

void Vulkan::destroyCommon(MJVulkanRenderData* renderData)
{
	//MJLog("destroyCommon:%p", renderData->vertexBuffer.buffer);
    destroySingleBuffer(renderData->vertexBuffer);
    destroyDynamicMultiBuffer(renderData->uniformBuffers);
}

void Vulkan::setupStaticBuffer(VkCommandBuffer commandBuffer, MJVMABuffer* buffer, VkBufferUsageFlags usageFlags, VkDeviceSize bufferSize, void* data, const char* debugName)
{
	if(bufferSize == 0)
	{
		MJLog("ERROR: don't allocate zero sized buffers");
		return;
	}
    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferAllocation;
    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

#if DEBUG_VKBUFFER_ALLOCATIONS
	stagingAllocInfo.flags = stagingAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	stagingAllocInfo.pUserData = (void*)(debugName);
#endif

    createVMABuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo, &stagingBuffer, &stagingBufferAllocation, nullptr);

    void* mappedData;
    vmaMapMemory(vmaAllocator, stagingBufferAllocation, &mappedData);
    memcpy(mappedData, data, (size_t)bufferSize);
    vmaUnmapMemory(vmaAllocator, stagingBufferAllocation);

    VmaAllocationCreateInfo finalBufferAllocInfo = {};
    finalBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

#if DEBUG_VKBUFFER_ALLOCATIONS
	finalBufferAllocInfo.flags = finalBufferAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	finalBufferAllocInfo.pUserData = (void*)(debugName);
#endif

    createVMABuffer(bufferSize, usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, finalBufferAllocInfo, &(buffer->buffer), &(buffer->allocation), &(buffer->allocInfo));

	//MJLog("allocate buffer a:%p", &(buffer->buffer))
	//MJLogBacktrace();

    copyBuffer(commandBuffer, stagingBuffer, buffer->buffer, bufferSize);
    MJVMABuffer bufferToDestroy;
    bufferToDestroy.allocation = stagingBufferAllocation;
    bufferToDestroy.buffer = stagingBuffer;
	//MJLog("setupStaticBuffer destroySingleBuffer:%p", stagingBuffer);

	//MJLog("destroy staging buffer a:%p", &(stagingBuffer))
    destroySingleBuffer(bufferToDestroy);
}

void Vulkan::setupDynamicBufferForMainOutput(std::vector<MJVMABuffer>* buffers, VkBufferUsageFlags usageFlags, VkDeviceSize bufferSize, const char* debugName)
{
	if(bufferSize == 0)
	{
		MJLog("ERROR: don't allocate zero sized buffers");
		return;
	}
    buffers->resize(frameCount());

    VmaAllocationCreateInfo uniformBufferAllocInfo = {};
    uniformBufferAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
//#ifndef __APPLE__
	uniformBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
//#endif
	uniformBufferAllocInfo.flags |= VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
    
    uniformBufferAllocInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    uniformBufferAllocInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

#if DEBUG_VKBUFFER_ALLOCATIONS
	uniformBufferAllocInfo.flags = uniformBufferAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	uniformBufferAllocInfo.pUserData = (void*)(debugName);
#endif


    for (size_t i = 0; i < frameCount(); i++) {
        createVMABuffer(bufferSize, usageFlags, uniformBufferAllocInfo, &(buffers->at(i).buffer), &(buffers->at(i).allocation), &(buffers->at(i).allocInfo));

		//MJLog("allocate buffer b:%p", &(buffers->at(i).buffer))
    }
	//MJLogBacktrace();
}


void Vulkan::setupDynamicBufferForRenderTarget(MJVMABuffer* buffer, VkBufferUsageFlags usageFlags, VkDeviceSize bufferSize, const char* debugName)
{
    VmaAllocationCreateInfo uniformBufferAllocInfo = {};
    uniformBufferAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
//#ifndef __APPLE__
    uniformBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
//#endif
	uniformBufferAllocInfo.flags |= VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
    
    uniformBufferAllocInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    uniformBufferAllocInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

#if DEBUG_VKBUFFER_ALLOCATIONS
	uniformBufferAllocInfo.flags = uniformBufferAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	uniformBufferAllocInfo.pUserData = (void*)(debugName);
#endif

    createVMABuffer(bufferSize, usageFlags, uniformBufferAllocInfo, &(buffer->buffer), &(buffer->allocation), &(buffer->allocInfo));
	//MJLog("allocate buffer c:%p", &(buffer->buffer))
	//MJLogBacktrace();
}

void Vulkan::setupCommon(VkCommandBuffer commandBuffer, MJVulkanRenderData* renderData, VkDeviceSize uniformBufferSize, VkDeviceSize vertexBufferSize, void* vertexData, const char* debugName)
{
    setupStaticBuffer(commandBuffer, &(renderData->vertexBuffer), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize, vertexData, debugName);
    setupDynamicBufferForMainOutput(&(renderData->uniformBuffers), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uniformBufferSize, debugName);
}

void Vulkan::destroyDescriptorPool(VkDescriptorPool pool)
{
    if(pool)
    {
		/*if(desdtroyDebugCounter == 22)
		{
			MJLog("doh");
		}
		MJLog("destroyDescriptorPool:%p - %d", pool, desdtroyDebugCounter);*/
        MJDescripterPoolToDestroy objectToDestroy = {pool, (uint32_t)frameCount() * 2 + 1};
        descripterPoolsToDestroy.push_back(objectToDestroy);
    }
}

void Vulkan::destroyRenderTarget(MJRenderTarget* renderTarget)
{
	if(renderTarget)
	{
		MJRenderTargetToDestroy objectToDestroy = {renderTarget, (uint32_t)frameCount() * 2 + 1};
		renderTargetsToDestroy.push_back(objectToDestroy);
	}
}

VkFramebuffer Vulkan::getFrameBuffer(int frameBufferIndex)
{
    return swapChainFramebuffers
		[frameBufferIndex];
}


uint32_t Vulkan::addTransferQueue(ThreadSafeQueue<TransferQueueInput>* inputQueue, ThreadSafeQueue<TransferQueueOutput>* outputQueue)
{
	uint32_t index = transferQueueIndexCount++;
	transferInputQueues[index] = inputQueue;
	transferOutputQueues[index] = outputQueue;

	return index;
}

void Vulkan::removeTransferQueue(uint32_t index)
{
	waitingTransfers.erase(index);

	transferInputQueues.erase(index);
	transferOutputQueues.erase(index);
}


void Vulkan::vsyncSettingChanged(bool newVsync_)
{
	vsync = newVsync_;
    needsToRecreateSwapchain = true;
}

void Vulkan::windowSizeChanged()
{
    needsToRecreateSwapchain = true;
}

vec3 cubeFaceDirectionForFaceIndex(int faceIndex)
{
    switch (faceIndex) {
    case 0:
        return vec3(1,0,0);
        break;
    case 1:
        return vec3(-1,0,0);
        break;
    case 2:
        return vec3(0,-1,0);
        break;
    case 3:
        return vec3(0,1,0);
        break;
    case 4:
        return vec3(0,0,1);
        break;
    case 5:
        return vec3(0,0,-1);
        break;

    default:
        break;
    }

    return vec3(1,0,0);
}

vec3 cubeUpForFaceIndex(int faceIndex)
{
    switch (faceIndex) {
    case 0:
        return vec3(0,-1,0);
        break;
    case 1:
        return vec3(0,-1,0);
        break;
    case 2:
        return vec3(0,0,-1);
        break;
    case 3:
        return vec3(0,0,1);
        break;
    case 4:
        return vec3(0,-1,0);
        break;
    case 5:
        return vec3(0,-1,0);
        break;

    default:
        break;
    }
    return vec3(0,-1,0);
}
