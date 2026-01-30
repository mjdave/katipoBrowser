#pragma once

#include "MathUtils.h"
#include "Vulkan.h"
#include <array>
#include <string>

class MJCache;
class GCommandBuffer;
class MJRenderTarget;

class MJRenderTargetAccumulation
{
public:
	VkImageView renderWithTextureImageView;
	std::vector<VkImageView> renderWithTextureImageViews;
    VkSampler textureSampler;

	MJRenderPass renderPass;

    ivec2 size;
    VkFormat format;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<bool> cleared;

public:
    MJRenderTargetAccumulation(Vulkan* vulkan_,
        ivec2 size_,
        VkFormat format_ = VK_FORMAT_R8G8B8A8_UNORM);
    ~MJRenderTargetAccumulation();

    void lock(GCommandBuffer* commandBuffer);
    void unlock(GCommandBuffer* commandBuffer);

private:
    Vulkan* vulkan;

    VkFramebuffer framebuffer;

    VkImage renderWithTextureImage;
    VmaAllocation renderWithTextureImageAllocation;

    VkImageView renderToTextureImageView;
    VkImage renderToTextureImage;
    VmaAllocation renderToTextureImageAllocation;

};

