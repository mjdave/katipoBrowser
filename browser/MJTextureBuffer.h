#pragma once

#include "MathUtils.h"
#include "Vulkan.h"

#define MJTB_FRAMEBUFFER_COUNT 3

struct MJTextureBufferGPUImage {
	MJVMAImage image;
	VkImageView imageView;
	VkSampler sampler;
};

class MJTextureBuffer
{
public:

public:
	MJTextureBuffer(Vulkan* vulkan_, ivec2 size_, std::string debugName_);
    ~MJTextureBuffer();

    void* getBuffer();
    MJTextureBufferGPUImage getGPUImage();

    void copyToGPU(VkCommandBuffer commandBuffer);

private:
    Vulkan* vulkan;

	ivec2 size;

    int writeBufferIndex;
    int readBufferIndex;

    VmaAllocationInfo allocInfos[MJTB_FRAMEBUFFER_COUNT];
    MJVMABuffer cpuBuffers[MJTB_FRAMEBUFFER_COUNT];
	MJVMAImage gpuImages[MJTB_FRAMEBUFFER_COUNT];

	VkImageView imageViews[MJTB_FRAMEBUFFER_COUNT];
	VkSampler samplers[MJTB_FRAMEBUFFER_COUNT];

    std::string debugName;
	bool mapped = false;

	bool used[MJTB_FRAMEBUFFER_COUNT];

private:

    void allocateBuffers(int bufferIndex);
};

