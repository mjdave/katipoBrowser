#include "MJRenderTarget3D.h"
#include "Vulkan.h"
#include "GPipeline.h"


MJRenderTarget3D::MJRenderTarget3D(Vulkan* vulkan_,
    ivec3 size_,
    VkFormat format_,
    bool writable)
{
    vulkan = vulkan_;
    size = size_;
    format = format_;
    lockedIndex= -1;

	renderPass.extent.width = size.x;
	renderPass.extent.height = size.y;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = size.x;
    imageInfo.extent.height = size.y;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = size.z;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if(writable)
    {
        imageInfo.usage = imageInfo.usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo finalImageBufferAllocInfo = {};
    finalImageBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalImageBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

	VMA_ALLOCATE_PROTECT(vmaCreateImage(vulkan->vmaAllocator, &imageInfo, &finalImageBufferAllocInfo, &textureImage, &textureImageAllocation, nullptr));
	//MJLog("vmaCreateImage m:0x%p",textureImage);

    textureImageViews.resize(size.z);

    for(int i = 0; i < size.z; i++)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = i;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(vulkan->device, &viewInfo, nullptr, &textureImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
    }

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(vulkan->device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(vulkan->device, &renderPassInfo, nullptr, &renderPass.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

    framebuffers.resize(size.z);
    for(int i = 0; i < size.z; i++)
    {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &textureImageViews[i];
        framebufferInfo.width = size.x;
        framebufferInfo.height = size.y;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vulkan->device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

    VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool = vulkan->commandPool;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount = 1;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


    //commandBuffers.resize(size.z);
    //fences.resize(size.z);
    semaphores.resize(size.z);

    for(int i = 0; i < size.z; i++)
    {
        /*if (vkAllocateCommandBuffers(vulkan->device, &commandBufferAllocInfo, &(commandBuffers[i])) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }*/

        /*if (vkCreateFence(vulkan->device, &fenceInfo, nullptr, &(fences[i])) != VK_SUCCESS) {
            throw std::runtime_error("failed to vkCreateFence objects for a render target");
        }*/

        if (vkCreateSemaphore(vulkan->device, &semaphoreInfo, nullptr, &(semaphores[i])) != VK_SUCCESS) {
            throw std::runtime_error("failed to vkCreateSemaphore objects for a render target");
        }
    }

    if (vkAllocateCommandBuffers(vulkan->device, &commandBufferAllocInfo, &(commandBuffer)) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
    if (vkCreateFence(vulkan->device, &fenceInfo, nullptr, &(fence)) != VK_SUCCESS) {
        throw std::runtime_error("failed to vkCreateFence objects for a render target");
    }
}


void MJRenderTarget3D::waitFences()
{
    //vkWaitForFences(vulkan->device, fences.size(), (VkFence*)fences.data(), VK_TRUE, (std::numeric_limits<uint64_t>::max)());
    vkWaitForFences(vulkan->device, 1, &fence, VK_TRUE, (std::numeric_limits<uint64_t>::max)());
}

MJRenderTarget3D::~MJRenderTarget3D()
{
    //vkWaitForFences(vulkan->device, fences.size(), (VkFence*)fences.data(), VK_TRUE, (std::numeric_limits<uint64_t>::max)());
    vkWaitForFences(vulkan->device, 1, &fence, VK_TRUE, (std::numeric_limits<uint64_t>::max)());
    for(int i = 0; i < framebuffers.size(); i++)
    {
        vkDestroyFramebuffer(vulkan->device, framebuffers[i], nullptr);
        vkDestroyImageView(vulkan->device, textureImageViews[i], nullptr);
        //vkDestroyFence(vulkan->device, fences[i], nullptr);
        vkDestroySemaphore(vulkan->device, semaphores[i], nullptr);
    }
    vkDestroyFence(vulkan->device, fence, nullptr);

    vkDestroyRenderPass(vulkan->device, renderPass.renderPass, nullptr);

    vmaDestroyImage(vulkan->vmaAllocator, textureImage, textureImageAllocation);

    vkDestroySampler(vulkan->device, textureSampler, nullptr);
    //vkFreeCommandBuffers(vulkan->device, vulkan->commandPool, commandBuffers.size(), commandBuffers.data());
    vkFreeCommandBuffers(vulkan->device, vulkan->commandPool, 1, &commandBuffer);
}

VkCommandBuffer* MJRenderTarget3D::lock(int layer)
{
    if(lockedIndex != -1)
    {
        MJLog("ERROR: trying to lock an already locked 3d render target");
        return nullptr;
    }
    lockedIndex = layer;
    //vkWaitForFences(vulkan->device, 1, &fences[layer], VK_TRUE, (std::numeric_limits<uint64_t>::max)());
    vkWaitForFences(vulkan->device, 1, &fence, VK_TRUE, (std::numeric_limits<uint64_t>::max)());

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    /*if (vkBeginCommandBuffer(commandBuffers[layer], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }*/
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass.renderPass;
    renderPassInfo.framebuffer = framebuffers[layer];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent.width = size.x;
    renderPassInfo.renderArea.extent.height = size.y;

    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    //vkCmdBeginRenderPass(commandBuffers[layer], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = size.x;
    viewport.height = size.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    //vkCmdSetViewport(commandBuffers[layer], 0, 1, &viewport);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width = size.x;
    scissor.extent.height = size.y;

    // vkCmdSetScissor(commandBuffers[layer], 0, 1, &scissor);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

   // return &(commandBuffers[layer]);
    return &commandBuffer;
}

void MJRenderTarget3D::unlock(bool shouldSignalSmaphores, std::vector<VkSemaphore> waitSemaphores)
{

    //vkCmdEndRenderPass(commandBuffers[lockedIndex]);
    vkCmdEndRenderPass(commandBuffer);

    /*if (vkEndCommandBuffer(commandBuffers[lockedIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }*/
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    //submitInfo.pCommandBuffers = &commandBuffers[lockedIndex];
    submitInfo.pCommandBuffers = &commandBuffer;

    std::vector<VkPipelineStageFlags> waitStages(waitSemaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    if(!waitSemaphores.empty())
    {
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
    }

    if(shouldSignalSmaphores)
    {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &semaphores[lockedIndex];
    }

    //vkResetFences(vulkan->device, 1, &fences[lockedIndex]);
    vkResetFences(vulkan->device, 1, &fence);

    //vkQueueSubmit(vulkan->graphicsQueue, 1, &submitInfo, fences[lockedIndex]);
    vkQueueSubmit(vulkan->graphicsQueue, 1, &submitInfo, fence);
    lockedIndex = -1;
}


void MJRenderTarget3D::writeToDisk(const std::string& filepath)
{
    VkImage srcImage = textureImage;
    VkImage dstImage;
    VmaAllocation dstImageAllocation;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent.width = size.x;
    imageInfo.extent.height = size.y;
    imageInfo.extent.depth = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.mipLevels = 1;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo finalImageBufferAllocInfo = {};
    finalImageBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
	finalImageBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
	VMA_ALLOCATE_PROTECT(vmaCreateImage(vulkan->vmaAllocator, &imageInfo, &finalImageBufferAllocInfo, &dstImage, &dstImageAllocation, nullptr));
	//MJLog("vmaCreateImage n:0x%p",dstImage);

    VkCommandBuffer commandBuffer = vulkan->beginSingleTimeCommands();

    VkImageMemoryBarrier imageMemoryBarrier {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };



    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = size.x;
    imageCopyRegion.extent.height = size.y;
    imageCopyRegion.extent.depth = 1;

    std::vector<char> outData;

    for(int layer = 0; layer < size.z; layer++)
    {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.image = dstImage;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.image = srcImage;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = layer;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        imageCopyRegion.srcSubresource.baseArrayLayer = layer;

        vkCmdCopyImage(
            commandBuffer,
            srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &imageCopyRegion);

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier.image = dstImage;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageMemoryBarrier.image = srcImage;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = layer;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);


        vulkan->endSingleTimeCommands(commandBuffer);

        VkImageSubresource subResource { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
        VkSubresourceLayout subResourceLayout;
        vkGetImageSubresourceLayout(vulkan->device, dstImage, &subResource, &subResourceLayout);

        int fileSize = subResourceLayout.size;

        if(layer == 0)
        {
            outData.resize(fileSize * size.z);
        }

        const char* data;
        vmaMapMemory(vulkan->vmaAllocator, dstImageAllocation, (void**)&data);
        data += subResourceLayout.offset;

        memcpy(&outData[layer * fileSize],data,fileSize);
        vmaUnmapMemory(vulkan->vmaAllocator, dstImageAllocation);

        commandBuffer = vulkan->beginSingleTimeCommands(); 
    }



    std::ofstream file((filepath).c_str(), std::ios::out | std::ios::binary);
    file.write(outData.data(), outData.size());
    file.close();

    MJLog("Wrote image %s. width:%d height:%d size:%d (expected %d)", filepath.c_str(), size.x, size.y, outData.size(), sizeof(uint16) * size.x * size.y * size.z * 4);


    vulkan->endSingleTimeCommands(commandBuffer);

    vmaDestroyImage(vulkan->vmaAllocator, dstImage, dstImageAllocation);

}
