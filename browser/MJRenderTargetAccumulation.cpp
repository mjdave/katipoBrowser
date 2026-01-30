
#include "MJRenderTargetAccumulation.h"
#include "MJRenderTarget.h"
#include "Vulkan.h"
#include "GCommandBuffer.h"


MJRenderTargetAccumulation::MJRenderTargetAccumulation(Vulkan* vulkan_,
    ivec2 size_,
    VkFormat format_)
{
    vulkan = vulkan_;
    format = format_;
    size = size_;

	renderPass.extent.width = size.x;
	renderPass.extent.height = size.y;

    int bufferCount = vulkan->frameCount();

    VkImageCreateInfo renderToImageInfo = {};
    renderToImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    renderToImageInfo.imageType = VK_IMAGE_TYPE_2D;
    renderToImageInfo.extent.width = size.x;
    renderToImageInfo.extent.height = size.y;
    renderToImageInfo.extent.depth = 1;
    renderToImageInfo.mipLevels = 1;
    renderToImageInfo.arrayLayers = 1;
    renderToImageInfo.format = format;
    renderToImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    renderToImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    renderToImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    renderToImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    renderToImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImageCreateInfo renderWithImageInfo = renderToImageInfo;
    renderWithImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VmaAllocationCreateInfo finalImageBufferAllocInfo = {};
    finalImageBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalImageBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;


	VMA_ALLOCATE_PROTECT(vmaCreateImage(vulkan->vmaAllocator, &renderToImageInfo, &finalImageBufferAllocInfo, &renderToTextureImage, &renderToTextureImageAllocation, nullptr));
    renderToTextureImageView = vulkan->createImageView(renderToTextureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);

	VMA_ALLOCATE_PROTECT(vmaCreateImage(vulkan->vmaAllocator, &renderWithImageInfo, &finalImageBufferAllocInfo, &renderWithTextureImage, &renderWithTextureImageAllocation, nullptr));
    renderWithTextureImageView = vulkan->createImageView(renderWithTextureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);

	for(int i = 0; i < bufferCount; i++)
	{
		renderWithTextureImageViews.push_back(renderWithTextureImageView);
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
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

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
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
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


    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass.renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &renderToTextureImageView;
    framebufferInfo.width = size.x;
    framebufferInfo.height = size.y;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(vulkan->device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }


    VkCommandBuffer commandBuffer = vulkan->beginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = renderWithTextureImage;

    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vulkan->endSingleTimeCommands(commandBuffer);
}



MJRenderTargetAccumulation::~MJRenderTargetAccumulation()
{

    vkDestroyFramebuffer(vulkan->device, framebuffer, nullptr);

    vkDestroyImageView(vulkan->device, renderWithTextureImageView, nullptr);
    vkDestroyImageView(vulkan->device, renderToTextureImageView, nullptr);
    vmaDestroyImage(vulkan->vmaAllocator, renderWithTextureImage, renderWithTextureImageAllocation);
    vmaDestroyImage(vulkan->vmaAllocator, renderToTextureImage, renderToTextureImageAllocation);
    
    vkDestroyRenderPass(vulkan->device, renderPass.renderPass, nullptr);
    vkDestroySampler(vulkan->device, textureSampler, nullptr);
}

void MJRenderTargetAccumulation::lock(GCommandBuffer* commandBuffer)
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass.renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent.width = size.x;
    renderPassInfo.renderArea.extent.height = size.y;

    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(*commandBuffer->getBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = size.x;
    viewport.height = size.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(*commandBuffer->getBuffer(), 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width = size.x;
    scissor.extent.height = size.y;

    vkCmdSetScissor(*commandBuffer->getBuffer(), 0, 1, &scissor);

    /*if(!cleared[commandBuffer->currentIndex])
    {
        cleared[commandBuffer->currentIndex] = true;
        unlock(commandBuffer);
        lock(commandBuffer);
    }*/ //doesnt seem to work
}

void MJRenderTargetAccumulation::unlock(GCommandBuffer* commandBuffer)
{
    vkCmdEndRenderPass(*commandBuffer->getBuffer());

    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = renderWithTextureImage;

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            *commandBuffer->getBuffer(),
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }


    VkImageBlit imageBlit{};				

    imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlit.srcSubresource.layerCount = 1;
    imageBlit.srcOffsets[1].x = size.x;
    imageBlit.srcOffsets[1].y = size.y;
    imageBlit.srcOffsets[1].z = 1;
    imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlit.dstSubresource.layerCount = 1;
    imageBlit.dstOffsets[1].x = size.x;
    imageBlit.dstOffsets[1].y = size.y;
    imageBlit.dstOffsets[1].z = 1;

    vkCmdBlitImage(*commandBuffer->getBuffer(),
        renderToTextureImage, 
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
        renderWithTextureImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &imageBlit,
        VK_FILTER_NEAREST);

    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = renderWithTextureImage;

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            *commandBuffer->getBuffer(),
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }
}
