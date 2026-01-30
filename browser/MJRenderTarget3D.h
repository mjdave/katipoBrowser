#pragma once

#include "MathUtils.h"
#include "Vulkan.h"
#include <array>

class MJCache;
class GPipeline;

class MJRenderTarget3D
{
public:
    std::vector<VkImageView> textureImageViews;
    VkSampler textureSampler;
	MJRenderPass renderPass;

    ivec3 size;

public:
    MJRenderTarget3D(Vulkan* vulkan_,
        ivec3 size_,
        VkFormat format_ = VK_FORMAT_R8G8B8A8_UNORM,
        bool writable = false);
    ~MJRenderTarget3D();

    VkCommandBuffer* lock(int layer);
    void unlock(bool shouldSignalSmaphores = false, std::vector<VkSemaphore> waitSemaphores = std::vector<VkSemaphore>());
    void waitFences();

    std::vector<VkSemaphore> semaphores;

    void writeToDisk(const std::string& filepath);

private:
    Vulkan* vulkan;
    VkFormat format;

    VkImage textureImage;
    VmaAllocation textureImageAllocation;

    std::vector<VkFramebuffer> framebuffers;

    //std::vector<VkCommandBuffer> commandBuffers;
    VkCommandBuffer commandBuffer;

    //std::vector<VkFence> fences;
    VkFence fence;

    int lockedIndex;

};

