#include "MJStaticRenderBuffer2.h"
#include "Vulkan.h"
#include "GCommandBuffer.h"



MJStaticRenderBuffer2::MJStaticRenderBuffer2(Vulkan* vulkan_, VkCommandBuffer commandBuffer, size_t vertSize_, int vertCount_, void* data, std::string debugName_)
{
    vulkan = vulkan_;

    debugName = debugName_;
    vertSize = vertSize_;
    vertCount = vertCount_;

	if(vertCount == 0)
	{
		cleanedUp = true;
		return;
	}

    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = vertCount * vertSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationInfo allocInfo;

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

	memcpy(allocInfo.pMappedData, data, vertCount * vertSize);

	VkBufferCopy copyRegion = {};
	copyRegion.size = vertCount * vertSize;
	vkCmdCopyBuffer(commandBuffer, cpuBuffer.buffer, gpuBuffer.buffer, 1, &copyRegion);

}

void MJStaticRenderBuffer2::cleanup(std::vector<MJVMABuffer>* buffersToDestroy)
{
	if(!cleanedUp)
	{
		if(buffersToDestroy)
		{
			buffersToDestroy->push_back(cpuBuffer); //could delete this cpu buffer earlier once the transfer is complete.
			buffersToDestroy->push_back(gpuBuffer);
		}
		else
		{
			vulkan->destroySingleBuffer(cpuBuffer);
			vulkan->destroySingleBuffer(gpuBuffer);
		}
		cleanedUp = true;
	}
}

MJStaticRenderBuffer2::~MJStaticRenderBuffer2()
{
	cleanup(nullptr);
}

MJStaticRenderBuffer2MainThreadBuffer MJStaticRenderBuffer2::getGPUBuffer()
{
	MJStaticRenderBuffer2MainThreadBuffer mainThreadBuffer = {};
	mainThreadBuffer.vertCount = vertCount;
	mainThreadBuffer.gpuBuffer = gpuBuffer;
	return mainThreadBuffer;
}