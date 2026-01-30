#pragma once

#include "MathUtils.h"
#include "Vulkan.h"

#define MJRB2_INDEXED_FRAMEBUFFER_COUNT 3


struct MJRenderBuffer2MainThreadBufferIndexed {
	dvec3 origin;
	MJVMABuffer gpuBuffer;
	MJVMABuffer indexGpuBuffer;
	int vertCount = 0;
	int indexCount = 0;
};

class MJRenderBufferIndexed2
{
public:

public:
	MJRenderBufferIndexed2(Vulkan* vulkan_, size_t vertSize_, int vertCount, int indexCount, std::string debugName_);
    ~MJRenderBufferIndexed2();

	void cleanup(std::vector<MJVMABuffer>* buffersToDestroy);

    void* getVertexBuffer(int vertCount, std::vector<MJVMABuffer>* buffersToDestroy);
    void* getIndexBuffer(int indexCount, std::vector<MJVMABuffer>* buffersToDestroy);

	MJRenderBuffer2MainThreadBufferIndexed getGPUBuffer();

    void copyToGPU(VkCommandBuffer commandBuffer, dvec3 origin);
	void setZero();


private:
    Vulkan* vulkan;

    int writeBufferIndex;
    int readBufferIndex;

    VmaAllocationInfo allocInfos[MJRB2_INDEXED_FRAMEBUFFER_COUNT];
    MJVMABuffer cpuBuffers[MJRB2_INDEXED_FRAMEBUFFER_COUNT];
    MJVMABuffer gpuBuffers[MJRB2_INDEXED_FRAMEBUFFER_COUNT];
    int allocatedSizes[MJRB2_INDEXED_FRAMEBUFFER_COUNT];

    VmaAllocationInfo indexAllocInfos[MJRB2_INDEXED_FRAMEBUFFER_COUNT];
    MJVMABuffer indexCpuBuffers[MJRB2_INDEXED_FRAMEBUFFER_COUNT];
    MJVMABuffer indexGpuBuffers[MJRB2_INDEXED_FRAMEBUFFER_COUNT];
    int indexAllocatedSizes[MJRB2_INDEXED_FRAMEBUFFER_COUNT];

	dvec3 origins[MJRB2_INDEXED_FRAMEBUFFER_COUNT];

    size_t vertSize;
    int actualVertCounts[MJRB2_INDEXED_FRAMEBUFFER_COUNT];
    int actualIndexCounts[MJRB2_INDEXED_FRAMEBUFFER_COUNT];

    std::string debugName;
	bool cleanedUp = false;
	bool valid = false;

private:

    void allocateBuffers(int vertCountToAllocate, int bufferIndex);
    void allocateIndexBuffers(int countToAllocate, int bufferIndex);
};

