#include "MJRenderBufferIndexed2.h"
#include "Vulkan.h"
#include "GCommandBuffer.h"



MJRenderBufferIndexed2::MJRenderBufferIndexed2(Vulkan* vulkan_, size_t vertSize_, int vertCount, int indexCount, std::string debugName_)
{
    vulkan = vulkan_;

    writeBufferIndex = 0;
    readBufferIndex = -1;
    debugName = debugName_;
    vertSize = vertSize_;


    for(int i = 0; i < MJRB2_INDEXED_FRAMEBUFFER_COUNT; i++)
    {
        allocateBuffers(vertCount, i);
        allocateIndexBuffers(indexCount, i);
        actualVertCounts[i] = 0;
        actualIndexCounts[i] = 0;
    }
}


void MJRenderBufferIndexed2::cleanup(std::vector<MJVMABuffer>* buffersToDestroy)
{
	if(!cleanedUp)
	{
		for(int i = 0; i < MJRB2_INDEXED_FRAMEBUFFER_COUNT; i++)
		{
			if(buffersToDestroy)
			{
				buffersToDestroy->push_back(cpuBuffers[i]);
				buffersToDestroy->push_back(gpuBuffers[i]);
				buffersToDestroy->push_back(indexCpuBuffers[i]);
				buffersToDestroy->push_back(indexGpuBuffers[i]);
			}
			else
			{
				vulkan->destroySingleBuffer(cpuBuffers[i]);
				vulkan->destroySingleBuffer(gpuBuffers[i]);
				vulkan->destroySingleBuffer(indexCpuBuffers[i]);
				vulkan->destroySingleBuffer(indexGpuBuffers[i]);
			}
		}

		cleanedUp = true;
	}
}

MJRenderBufferIndexed2::~MJRenderBufferIndexed2()
{
	cleanup(nullptr);
}

void MJRenderBufferIndexed2::allocateBuffers(int vertCountToAllocate, int bufferIndex)
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

    allocatedSizes[bufferIndex] = vertCountToAllocate;

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

    vulkan->createVMABuffer(vertCountToAllocate * vertSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, finalBufferAllocInfo, &(gpuBuffers[bufferIndex].buffer), &(gpuBuffers[bufferIndex].allocation));
}


void MJRenderBufferIndexed2::allocateIndexBuffers(int countToAllocate, int bufferIndex)
{
    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = countToAllocate * sizeof(uint32_t);
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

#if DEBUG_VKBUFFER_ALLOCATIONS
	if(!debugName.empty())
	{
		stagingAllocInfo.flags = stagingAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
		stagingAllocInfo.pUserData = (void*)(debugName.c_str());
	}
#endif

    VkResult result = vmaCreateBuffer(vulkan->vmaAllocator, &bufferInfo, &stagingAllocInfo,  &(indexCpuBuffers[bufferIndex].buffer), &(indexCpuBuffers[bufferIndex].allocation), &indexAllocInfos[bufferIndex]);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to createVMABuffer!");
    }

    indexAllocatedSizes[bufferIndex] = countToAllocate;

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

    vulkan->createVMABuffer(countToAllocate * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, finalBufferAllocInfo, &(indexGpuBuffers[bufferIndex].buffer), &(indexGpuBuffers[bufferIndex].allocation));
}

void* MJRenderBufferIndexed2::getVertexBuffer(int vertCount, std::vector<MJVMABuffer>* buffersToDestroy)
{
    if(vertCount > allocatedSizes[writeBufferIndex])
    {
        int vertCountToAllocate = vertCount * 1.5;
       // MJLog("indexed render VERTEX buffer \"%s\" resized from:%d to:%d (%p)", debugName.c_str(), allocatedSizes[writeBufferIndex], vertCountToAllocate, gpuBuffers[writeBufferIndex].buffer);

		buffersToDestroy->push_back(cpuBuffers[writeBufferIndex]);
		buffersToDestroy->push_back(gpuBuffers[writeBufferIndex]);

        allocateBuffers(vertCountToAllocate, writeBufferIndex);
    }

    actualVertCounts[writeBufferIndex] = vertCount;

    return allocInfos[writeBufferIndex].pMappedData;
}


void* MJRenderBufferIndexed2::getIndexBuffer(int indexCount, std::vector<MJVMABuffer>* buffersToDestroy)
{
    if(indexCount > indexAllocatedSizes[writeBufferIndex])
    {
        int countToAllocate = indexCount * 1.5;
       // MJLog("indexed render INDEX buffer \"%s\" resized from:%d to:%d  (%p)", debugName.c_str(), indexAllocatedSizes[writeBufferIndex], countToAllocate, indexGpuBuffers[writeBufferIndex].buffer);

		buffersToDestroy->push_back(indexCpuBuffers[writeBufferIndex]);
		buffersToDestroy->push_back(indexGpuBuffers[writeBufferIndex]);

        allocateIndexBuffers(countToAllocate, writeBufferIndex);
    }

    actualIndexCounts[writeBufferIndex] = indexCount;

    return indexAllocInfos[writeBufferIndex].pMappedData;
}

void MJRenderBufferIndexed2::setZero()
{
	actualIndexCounts[writeBufferIndex] = 0;
	actualVertCounts[writeBufferIndex] = 0;
	readBufferIndex = writeBufferIndex;
	writeBufferIndex = (writeBufferIndex + 1) % MJRB2_INDEXED_FRAMEBUFFER_COUNT;
	valid = true;
}

void MJRenderBufferIndexed2::copyToGPU(VkCommandBuffer commandBuffer, dvec3 origin)
{
    if(actualVertCounts[writeBufferIndex] > 0)
    {
        readBufferIndex = writeBufferIndex;
        writeBufferIndex = (writeBufferIndex + 1) % MJRB2_INDEXED_FRAMEBUFFER_COUNT;

		origins[readBufferIndex] = origin;

        VkBufferCopy copyRegion = {};
        copyRegion.size = actualVertCounts[readBufferIndex] * vertSize;
        if(copyRegion.size == 0)
        {
            MJLog("attempting to cpy zero bytes");
        }
        vkCmdCopyBuffer(commandBuffer, cpuBuffers[readBufferIndex].buffer, gpuBuffers[readBufferIndex].buffer, 1, &copyRegion);

        VkBufferCopy indexCopyRegion = {};
        indexCopyRegion.size = actualIndexCounts[readBufferIndex] * sizeof(uint32_t);
        if(indexCopyRegion.size == 0)
        {
            MJLog("attempting to cpy zero bytes");
        }
        vkCmdCopyBuffer(commandBuffer, indexCpuBuffers[readBufferIndex].buffer, indexGpuBuffers[readBufferIndex].buffer, 1, &indexCopyRegion);

		valid = true;
    }
}

MJRenderBuffer2MainThreadBufferIndexed MJRenderBufferIndexed2::getGPUBuffer()
{
	MJRenderBuffer2MainThreadBufferIndexed mainThreadBuffer = {};

	if(valid && actualIndexCounts[readBufferIndex] > 0)
	{
		mainThreadBuffer.vertCount = actualVertCounts[readBufferIndex];
		mainThreadBuffer.indexCount = actualIndexCounts[readBufferIndex];
		mainThreadBuffer.gpuBuffer = gpuBuffers[readBufferIndex];
		mainThreadBuffer.indexGpuBuffer = indexGpuBuffers[readBufferIndex];
		mainThreadBuffer.origin = origins[readBufferIndex];
	}
	else
	{
		mainThreadBuffer.vertCount = 0;
		mainThreadBuffer.indexCount = 0;
	}
	return mainThreadBuffer;
}
