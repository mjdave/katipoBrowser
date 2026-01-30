#pragma once

#include "MathUtils.h"
#include "Vulkan.h"
#include <array>
#include <string>

class MJCache;
class GCommandBuffer;

class MJRenderTarget
{
public:
    std::vector<VkImageView> textureImageViews;
	VkImageView textureImageView;
    VkSampler textureSampler;
    MJRenderPass renderPass;
    VkImage textureImage;

    ivec2 size;
    VkFormat format;
    bool requiresDepth = false;
    bool multiSample = false;

public:
    MJRenderTarget(Vulkan* vulkan_,
        ivec2 size_,
        VkFormat format_ = VK_FORMAT_R8G8B8A8_UNORM,
        bool requiresDepth_ = false,
        int multiSampleCount = 1,
		VkFilter minFilter_ = VK_FILTER_LINEAR, 
		VkFilter magFilter_ = VK_FILTER_LINEAR,
		bool canBeTransferSource = false,
		bool canBeTransferDestination = false);
    ~MJRenderTarget();

    void lock(GCommandBuffer* commandBuffer, vec4 clearColor = vec4(0.0,0.0,0.0,1.0));
    void unlock(GCommandBuffer* commandBuffer);

private:
    Vulkan* vulkan;

    VmaAllocation textureImageAllocation;

    VmaAllocation depthImageAllocation;
    VkImageView depthImageView;
    VkImage depthImage;

    VkImage msaaColorImage;
    VmaAllocation msaaColorImageAllocation;
    VkImageView msaaColorImageView;

    VkFramebuffer framebuffer;

private:

    void createDepthResources();
    void createMultisampleResources();

};

