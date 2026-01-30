#include "MJStaticRenderBuffer.h"
#include "Vulkan.h"
#include "GCommandBuffer.h"



MJStaticRenderBuffer::MJStaticRenderBuffer(Vulkan* vulkan_, size_t vertSize_, int vertCount_, std::string debugName_)
{
    vulkan = vulkan_;

    debugName = debugName_;
    vertSize = vertSize_;
    vertCount = vertCount_;

    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = vertCount * vertSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

#if DEBUG_VKBUFFER_ALLOCATIONS
	if(!debugName.empty())
	{
		stagingAllocInfo.flags = stagingAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
		stagingAllocInfo.pUserData = (void*)(debugName.c_str());
	}
#endif

    VkResult result = vmaCreateBuffer(vulkan->vmaAllocator, &bufferInfo, &stagingAllocInfo,  &(cpuBuffer.buffer), &(cpuBuffer.allocation), &allocInfo);

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

    vulkan->createVMABuffer(vertCount * vertSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, finalBufferAllocInfo, &(gpuBuffer.buffer), &(gpuBuffer.allocation));
}


MJStaticRenderBuffer::~MJStaticRenderBuffer()
{
    if(!isReady)
    {
        vulkan->destroySingleBuffer(cpuBuffer);
    }
    vulkan->destroySingleBuffer(gpuBuffer);
}

void* MJStaticRenderBuffer::getBuffer()
{
    return allocInfo.pMappedData;
}

void MJStaticRenderBuffer::copyToGPU(GCommandBuffer* commandBuffer)
{
    VkBufferCopy copyRegion = {};
    copyRegion.size = vertCount * vertSize;
    if(copyRegion.size == 0)
    {
        MJLog("attempting to cpy zero bytes");
    }
    vkCmdCopyBuffer(*commandBuffer->getBuffer(), cpuBuffer.buffer, gpuBuffer.buffer, 1, &copyRegion);

    vulkan->destroySingleBuffer(cpuBuffer);

    isReady = true;
}

MJVMABuffer MJStaticRenderBuffer::getGPUBuffer()
{
    return gpuBuffer;
}

bool MJStaticRenderBuffer::ready()
{
    return isReady;
}
