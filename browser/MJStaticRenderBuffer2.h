#pragma once

#include "MathUtils.h"
#include "Vulkan.h"


struct MJStaticRenderBuffer2MainThreadBuffer {
	MJVMABuffer gpuBuffer;
	int vertCount = 0;
};

class MJStaticRenderBuffer2
{
public:

public:
    MJStaticRenderBuffer2(Vulkan* vulkan_, VkCommandBuffer commandBuffer, size_t vertSize_, int vertCount, void* data, std::string debugName_);
    ~MJStaticRenderBuffer2();

	void cleanup(std::vector<MJVMABuffer>* buffersToDestroy);

	MJStaticRenderBuffer2MainThreadBuffer getGPUBuffer();

private:
    Vulkan* vulkan;

    MJVMABuffer cpuBuffer;
    MJVMABuffer gpuBuffer;

    size_t vertSize;
    int vertCount;

    std::string debugName;
	bool cleanedUp = false;

private:

};

