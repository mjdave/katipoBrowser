#pragma once

#include "MathUtils.h"
#include "Vulkan.h"
#include <array>

class MJCache;
class GPipeline;
class GCommandBuffer;

class MJRenderTargetSSAO
{
public:
    std::vector<VkImageView> depthImageViews;
	VkImageView depthImageView;
    VkSampler textureSampler;

	MJRenderPass renderPass;

    ivec2 size;

public:
	MJRenderTargetSSAO(Vulkan* vulkan_,
        ivec2 size_);
    ~MJRenderTargetSSAO();

    void lock(GCommandBuffer* commandBuffer);
    void unlock(GCommandBuffer* commandBuffer);

private:
    Vulkan* vulkan;

    VkImage depthImage;
    VmaAllocation depthImageAllocation;

	VkFramebuffer framebuffer;

    int lockedCommandBufferIndex;

private:

};

