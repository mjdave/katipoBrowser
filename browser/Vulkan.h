
#ifndef UMJVulkan_h
#define UMJVulkan_h

#ifdef _MSC_VER
#define _WINSOCKAPI_    // stops windows.h including winsock.h
#define NOMINMAX
#include <windows.h>
#endif

#include "SDL.h"
#if IS_SERVER
struct Vulkan {};

#else

#include "SDL_vulkan.h"
#include "vulkan/vulkan.h"
#include <vector>
#include <set>
#include <map>
#include "MathUtils.h"
#include "ThreadSafeQueue.h"
#include "VulkanMemoryThread.h"

#define VULKAN_DEBUG_LOG(fmt__, ...) MJLog("WARNING:" fmt__, ##__VA_ARGS__)


#ifdef _MSC_VER
#define VULKAN_PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#else
#define VULKAN_PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif


vec3 cubeFaceDirectionForFaceIndex(int faceIndex);
vec3 cubeUpForFaceIndex(int faceIndex);

class GCommandBuffer;
class MainController;
class Timer;
class MJRenderTarget;



struct VertexDescriptionInfo {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    VkVertexInputBindingDescription bindingDescription = {};
};

struct QueueFamilyIndices {
    uint32_t graphicsFamily = UINT32_MAX;
    uint32_t presentFamily = UINT32_MAX;
	uint32_t transferFamily = UINT32_MAX;

    bool isComplete() {
        return (graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX && transferFamily != UINT32_MAX);
    }
};


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


struct DrawQuadVertex {
    vec2 pos;
    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(DrawQuadVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return std::vector<VkVertexInputBindingDescription>(1, bindingDescription);
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(DrawQuadVertex, pos);

        return attributeDescriptions;
    }
};

struct TransferQueueInput {
	VkCommandBuffer commandBuffer;
};

struct MJRenderPass {
	VkRenderPass renderPass = nullptr;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkExtent2D extent = {};
};

struct TransferQueueOutput {
	bool done;
};

class Vulkan
{
public:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkInstance instance;
    VkDevice device;
    VmaAllocator vmaAllocator;
	VulkanMemoryThread* memoryThread;
	ThreadSafeQueue<VulkanMemoryThreadInput>* memoryQueue;
	VmaBudget vmaBudget;
	bool hasMemoryBudgetExtension = false;
	bool VK_KHR_get_physical_device_properties2_enabled = false;
    VkRenderPass renderPass;
    VkCommandPool commandPool;

    GCommandBuffer* primaryCommandBuffer;

    VkQueue graphicsQueue;
	VkQueue transferQueue;

    MJVMABuffer drawQuadVertBuffer;

	QueueFamilyIndices queueFamilyIndices;

	MJRenderPass finalOutputRenderPass;

	uint64_t deviceVendorID;
	std::string deviceName;
	uint32_t driverVersion;

	bool hasValidSwapchain = false;

public:
    Vulkan(MainController* controller_, SDL_Window* window_, ivec2 screenSize, bool vsync_);
    ~Vulkan();

    bool startRecord();
    void endRecord();
	void submit();

    int frameCount();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void createVMABuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateInfo allocInfo, VkBuffer* buffer, VmaAllocation* allocation, VmaAllocationInfo* allocationInfo = nullptr);
    void copyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels = 1, int layerCount = 1);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, int mipLevels = 1, int arrayLayers = 1, VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D);
    void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void setupStaticBuffer(VkCommandBuffer commandBuffer, MJVMABuffer* buffer, VkBufferUsageFlags usageFlags, VkDeviceSize bufferSize, void* data, const char* debugName);
    void setupDynamicBufferForMainOutput(std::vector<MJVMABuffer>* buffers, VkBufferUsageFlags usageFlags, VkDeviceSize bufferSize, const char* debugName);
    void setupDynamicBufferForRenderTarget(MJVMABuffer* buffer, VkBufferUsageFlags usageFlags, VkDeviceSize bufferSize, const char* debugName);

    void destroySingleBuffer(MJVMABuffer& buffer);
    void destroyDynamicMultiBuffer(std::vector<MJVMABuffer>& buffers);

    void setupCommon(VkCommandBuffer commandBuffer, MJVulkanRenderData* renderData, VkDeviceSize uniformBufferSize, VkDeviceSize vertexBufferSize, void* vertexData, const char* debugName);
    void destroyCommon(MJVulkanRenderData* renderData);


    void destroyDescriptorPool(VkDescriptorPool pool);
	void destroyRenderTarget(MJRenderTarget* renderTarget);
	void destroySingleImage(MJVMAImage& image);

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);

    VkFramebuffer getFrameBuffer(int frameBufferIndex);

    VkSampleCountFlagBits getMaxUsableSampleCount(int desiredCount);

	uint32_t addTransferQueue(ThreadSafeQueue<TransferQueueInput>* inputQueue, ThreadSafeQueue<TransferQueueOutput>* outputQueue);
	void removeTransferQueue(uint32_t index);

    void windowSizeChanged();
	void screenResolutionChanged(ivec2 screenSize);
	void vsyncSettingChanged(bool newVsync_);

	void logDeviceDetails(VkPhysicalDevice device, std::string prefix);
	void logMainDeviceDetails();

	void debugCommandBuffer(VkCommandBuffer commandBuffer, int userData);
	void printDebugCommandBuffer(VkQueue queue);


private:
    MainController* controller;
    SDL_Window* window;


    VkSurfaceKHR surface;
    std::vector<VkFramebuffer> swapChainFramebuffers;
	VkExtent2D swapChainExtent;

	bool vsync;
	bool needsToRecreateSwapchain = false;

	VkDebugUtilsMessengerEXT debugMessenger;

	std::thread::id mainThreadID;

	Timer* debugTimerA;
	Timer* debugTimerB;
	Timer* debugTimerC;
	double debugTimeCounter = 0;
	int debugFrameCounter= 0;

    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    std::vector<VkImageView> swapChainImageViews;

    VkImage colorImage;
    VmaAllocation colorImageAllocation;
    VkImageView colorImageView;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    size_t currentFrame = 0;

    uint32_t currentRecordingImageIndex;

    bool framebufferResized = false;

    std::vector<MJVMABuffer> renderBuffersToDestroy;
    std::vector<MJDescripterPoolToDestroy> descripterPoolsToDestroy;
	std::vector<MJRenderTargetToDestroy> renderTargetsToDestroy;
	std::vector<MJVMAImagesToDestroy> imagesToDestroy;

	uint32_t transferQueueIndexCount = 0;
	std::map<uint32_t, ThreadSafeQueue<TransferQueueInput>*> transferInputQueues;
	std::map<uint32_t, ThreadSafeQueue<TransferQueueOutput>*> transferOutputQueues;
	std::set<uint32_t> waitingTransfers;
	VkFence transferFence;

	


private:
    void createInstance();
    //void setupDebugCallback();
    void createSurface();
	void destroySurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createVMA();
    bool createSwapChain();
    void createImageViews();
    void createRenderPass();

    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    void cleanupSwapChain();
    void recreateSwapChain();

	void setupDebugMessenger();


    int getDeviceSuitabilityScore(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceBaseExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);


    void createColorResources();

    void destroyObjects();
	void submitTransfers();

    //std::set<VkBuffer> destroyedBuffersDebug;

};

#endif //IS_SERVER
#endif
