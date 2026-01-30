#pragma once

#include "MathUtils.h"
#include "Vulkan.h"

#define MJRB2_FRAMEBUFFER_COUNT 3

struct MJRenderBuffer2MainThreadBuffer {
	MJVMABuffer gpuBuffer;
	dvec3 origin;
	int vertCount = 0;
};

class MJRenderBuffer2
{
public:

public:
	MJRenderBuffer2(Vulkan* vulkan_, size_t vertSize_, int vertCount, std::string debugName_, int usage_ = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    ~MJRenderBuffer2();

	void cleanup(std::vector<MJVMABuffer>* buffersToDestroy);

    void* getBuffer(int vertCount, std::vector<MJVMABuffer>* buffersToDestroy);
	MJRenderBuffer2MainThreadBuffer getGPUBuffer(dvec3 origin);

    void copyToGPU(VkCommandBuffer commandBuffer);
	void setZero();

private:
    Vulkan* vulkan;

    int usage;

    int writeBufferIndex;
    int readBufferIndex;

    VmaAllocationInfo allocInfos[MJRB2_FRAMEBUFFER_COUNT];
    MJVMABuffer cpuBuffers[MJRB2_FRAMEBUFFER_COUNT];
    MJVMABuffer gpuBuffers[MJRB2_FRAMEBUFFER_COUNT];
    int allocatedSizes[MJRB2_FRAMEBUFFER_COUNT];

    size_t vertSize;
    int actualVertCounts[MJRB2_FRAMEBUFFER_COUNT];

    std::string debugName;
	bool cleanedUp = false;
	bool valid = false;

private:

    void allocateBuffers(int vertCountToAllocate, int bufferIndex);
};

