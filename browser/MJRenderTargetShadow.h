#pragma once

#include "MathUtils.h"
#include "Vulkan.h"
#include <array>

class MJCache;
class GPipeline;
class GCommandBuffer;

class MJRenderTargetShadow
{
public:
	std::vector<VkImageView> depthImageViews;
    VkSampler textureSampler;
	VkSampler standardTextureSampler;

	MJRenderPass renderPass;

    ivec2 size;

public:
    MJRenderTargetShadow(Vulkan* vulkan_,
        ivec2 size_);
    ~MJRenderTargetShadow();

    void lock(GCommandBuffer* commandBuffer);
    void unlock(GCommandBuffer* commandBuffer);

private:
    Vulkan* vulkan;

    VkImage depthImage;
    VmaAllocation depthImageAllocation;

    VkFramebuffer framebuffer;
	VkImageView depthImageView;

    int lockedCommandBufferIndex;

private:

};

