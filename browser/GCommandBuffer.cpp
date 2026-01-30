#ifdef _MSC_VER
#define _WINSOCKAPI_    // stops windows.h including winsock.h
#define NOMINMAX
#include <windows.h>
#endif

#include "GCommandBuffer.h"
#include "Vulkan.h"

#include "vk_mem_alloc.h"


GCommandBuffer::GCommandBuffer(Vulkan* vulkan_)
{
    vulkan = vulkan_;
    commandBuffers.resize(vulkan->frameCount());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkan->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(vulkan->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}


GCommandBuffer::~GCommandBuffer()
{
}

void GCommandBuffer::startRecord(uint32_t imageIndex)
{
    currentIndex = imageIndex;

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    if (vkBeginCommandBuffer(commandBuffers[currentIndex], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
}


void GCommandBuffer::startRenderPass()
{ 
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkan->renderPass;
    renderPassInfo.framebuffer = vulkan->getFrameBuffer(currentIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vulkan->finalOutputRenderPass.extent;

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {0.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[currentIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = vulkan->finalOutputRenderPass.extent.width;
    viewport.height = vulkan->finalOutputRenderPass.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(commandBuffers[currentIndex], 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width = vulkan->finalOutputRenderPass.extent.width;
    scissor.extent.height = vulkan->finalOutputRenderPass.extent.height;

    vkCmdSetScissor(commandBuffers[currentIndex], 0, 1, &scissor);
}


void GCommandBuffer::endRenderPass()
{
    vkCmdEndRenderPass(commandBuffers[currentIndex]);
}

void GCommandBuffer::endRecord()
{
    if (vkEndCommandBuffer(commandBuffers[currentIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

const VkCommandBuffer* GCommandBuffer::getBuffer()
{
    return &(commandBuffers[currentIndex]);
}
