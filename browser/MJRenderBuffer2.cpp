#include "MJRenderBuffer2.h"
#include "Vulkan.h"
#include "GCommandBuffer.h"



MJRenderBuffer2::MJRenderBuffer2(Vulkan* vulkan_, size_t vertSize_, int vertCount, std::string debugName_, int usage_)
{
    vulkan = vulkan_;

    writeBufferIndex = 0;
    readBufferIndex = -1;
    debugName = debugName_;
    vertSize = vertSize_;
    usage = usage_;


    for(int i = 0; i < MJRB2_FRAMEBUFFER_COUNT; i++)
    {
        allocateBuffers(vertCount, i);
    }
}

void MJRenderBuffer2::cleanup()
{
	if(!cleanedUp)
	{
		for(int i = 0; i < MJRB2_FRAMEBUFFER_COUNT; i++)
		{
            vulkan->destroySingleBuffer(gpuBuffers[i]);
            vulkan->destroySingleBuffer(cpuBuffers[i]);
		}
		cleanedUp = true;
	}
}

MJRenderBuffer2::~MJRenderBuffer2()
{
	cleanup();
}

void MJRenderBuffer2::allocateBuffers(int vertCountToAllocate, int bufferIndex)
{
    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

#if DEBUG_VKBUFFER_ALLOCATIONS
	if(!debugName.empty())
	{
		stagingAllocInfo.flags = stagingAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
		stagingAllocInfo.pUserData = (void*)(debugName.c_str());
	}
#endif

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = vertCountToAllocate * vertSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vmaCreateBuffer(vulkan->vmaAllocator, &bufferInfo, &stagingAllocInfo,  &(cpuBuffers[bufferIndex].buffer), &(cpuBuffers[bufferIndex].allocation), &allocInfos[bufferIndex]);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to createVMABuffer!");
    }

    VmaAllocationCreateInfo finalBufferAllocInfo = {};
    finalBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

#if DEBUG_VKBUFFER_ALLOCATIONS
	if(!debugName.empty())
	{
		finalBufferAllocInfo.flags = finalBufferAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
		finalBufferAllocInfo.pUserData = (void*)(debugName.c_str());
	}
#endif

    vulkan->createVMABuffer(vertCountToAllocate * vertSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, finalBufferAllocInfo, &(gpuBuffers[bufferIndex].buffer), &(gpuBuffers[bufferIndex].allocation));
    
}

void* MJRenderBuffer2::getBuffer()
{
    return allocInfos[writeBufferIndex].pMappedData;
}

void MJRenderBuffer2::copyToGPU(VkCommandBuffer commandBuffer, int vertCount)
{
    readBufferIndex = writeBufferIndex;
    writeBufferIndex = (writeBufferIndex + 1) % MJRB2_FRAMEBUFFER_COUNT;

    VkBufferCopy copyRegion = {};
    copyRegion.size = vertCount * vertSize;
    if(copyRegion.size == 0)
    {
        MJLog("attempting to cpy zero bytes");
    }
    vkCmdCopyBuffer(commandBuffer, cpuBuffers[readBufferIndex].buffer, gpuBuffers[readBufferIndex].buffer, 1, &copyRegion);
    valid = true;
}

MJVMABuffer MJRenderBuffer2::getGPUBuffer()
{
	if(valid)
	{
        return gpuBuffers[readBufferIndex];
	}
    
    return MJVMABuffer();
    
}
