
#ifdef _MSC_VER
#define _WINSOCKAPI_    // stops windows.h including winsock.h
#define NOMINMAX
#include <windows.h>
#endif

#ifndef VulkanMemoryThread_h
#define VulkanMemoryThread_h

#include "MJLog.h"
#include "ThreadSafeQueue.h"
#include "vulkan/vulkan.h"

#define DEBUG_VKBUFFER_ALLOCATIONS 0

/*# ifdef __APPLE__
#define mjCopyGPUMemoryPtr(__allocator, __mjVMABuffer, __bufPtr, __bufSize) {\
void* mappedData;\
vmaMapMemory(__allocator, __mjVMABuffer.allocation, &mappedData);\
memcpy(mappedData, __bufPtr, __bufSize);\
vmaUnmapMemory(__allocator, __mjVMABuffer.allocation);\
}
#else*/
#define mjCopyGPUMemoryPtr(__allocator, __mjVMABuffer, __bufPtr, __bufSize) memcpy(__mjVMABuffer.allocInfo.pMappedData, __bufPtr, __bufSize)
//#endif

#define mjCopyGPUMemory(__allocator, __mjVMABuffer, __buf) mjCopyGPUMemoryPtr(__allocator, __mjVMABuffer, &__buf, sizeof(__buf));

#define VMA_ALLOCATE_PROTECT(__funcCall__) if((__funcCall__) < 0) { \
MJError("FAILED TO ALLOCATE VIDEO MEMORY: " #__funcCall__);\
                throw std::runtime_error("FAILED TO ALLOCATE VIDEO MEMORY: " #__funcCall__); \
            }


#undef VMA_DEBUG_INITIALIZE_ALLOCATIONS
#undef VMA_DEBUG_MARGIN
#undef VMA_DEBUG_DETECT_CORRUPTION
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1

//#include "Vulkan.h"
#include "vk_mem_alloc.h"


class MJRenderTarget;

struct MJVMABuffer {
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo allocInfo;
};

struct MJVMAImage {
	VkImage image;
	VmaAllocation allocation;
	VmaAllocationInfo allocInfo;
};

struct MJVulkanRenderData {
	MJVMABuffer vertexBuffer;
	std::vector<MJVMABuffer> uniformBuffers;
};

struct MJVMABuffersToDestroy {
	std::vector<MJVMABuffer> buffers;
	uint32_t delayCounter;
};

struct MJDescripterPoolToDestroy {
	VkDescriptorPool pool;
	uint32_t counter;
};

struct MJRenderTargetToDestroy {
	MJRenderTarget* renderTarget;
	uint32_t counter;
};

struct MJVMAImagesToDestroy {
	MJVMAImage image;
	uint32_t counter;
};

struct VulkanMemoryThreadInput {
	uint8_t type;
	MJVMABuffersToDestroy buffersToDestroy;
};


#define VULKAN_MEMORY_THREAD_INPUT_TYPE_NORMAL 0
#define VULKAN_MEMORY_THREAD_INPUT_TYPE_EXIT 1

class VulkanMemoryThread {
public:

public:
	VulkanMemoryThread(ThreadSafeQueue<VulkanMemoryThreadInput>* inputQueue_,
		VmaAllocator vmaAllocator_,
		std::string& logFilePath_);
    
    ~VulkanMemoryThread();
    
private:
	VmaAllocator vmaAllocator;

	std::thread* _thread;

	ThreadSafeQueue<VulkanMemoryThreadInput>* inputQueue;
	std::string logFilePath;

	std::vector<MJVMABuffersToDestroy> buffersToDestroy;
    
private:

	void cleanupThread();
	void updateThread();

};

#endif /* Database_h */
