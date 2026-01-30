#pragma once

#include <vector>
#include "Vulkan.h"

class GCommandBuffer
{
public:
    uint32_t currentIndex;
	Vulkan* vulkan;

public:
    GCommandBuffer(Vulkan* vulkan);
    ~GCommandBuffer();

    void startRecord(uint32_t imageIndex);
    void startRenderPass();
    void endRenderPass();
    void endRecord();

    const VkCommandBuffer* getBuffer();

private:

    std::vector<VkCommandBuffer> commandBuffers;
};

