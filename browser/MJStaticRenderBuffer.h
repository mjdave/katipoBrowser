#pragma once

#include "MathUtils.h"
#include "Vulkan.h"

class GCommandBuffer;


class MJStaticRenderBuffer
{
public:

public:
    MJStaticRenderBuffer(Vulkan* vulkan_, size_t vertSize_, int vertCount, std::string debugName_);
    ~MJStaticRenderBuffer();

    void* getBuffer();
    MJVMABuffer getGPUBuffer();

    void copyToGPU(GCommandBuffer* commandBuffer);
    bool ready();

private:
    Vulkan* vulkan;

    VmaAllocationInfo allocInfo;
    MJVMABuffer cpuBuffer;
    MJVMABuffer gpuBuffer;

    size_t vertSize;
    int vertCount;

    bool isReady = false;

    std::string debugName;

private:

    void allocateBuffers(int vertCountToAllocate, int bufferIndex);
};

