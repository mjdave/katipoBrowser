#include "MJRenderTarget.h"
#include "Vulkan.h"
#include "GCommandBuffer.h"
#include "MJCache.h"

MJRenderTarget::MJRenderTarget(Vulkan* vulkan_,
    ivec2 size_,
    VkFormat format_,
    bool requiresDepth_,
    int multiSampleCount,
	VkFilter minFilter, 
	VkFilter magFilter,
	bool canBeTransferSource,
	bool canBeTransferDestination)
{
    vulkan = vulkan_;
    format= format_;
    size = size_;
    requiresDepth = requiresDepth_;
    multiSample = (multiSampleCount > 1);

    if(multiSample)
    {
        renderPass.msaaSamples = vulkan->getMaxUsableSampleCount(multiSampleCount);
    }

	renderPass.extent.width = size.x;
	renderPass.extent.height = size.y;

    int bufferCount = vulkan->frameCount();

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = size.x;
    imageInfo.extent.height = size.y;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if(canBeTransferSource)
	{
		imageInfo.usage = imageInfo.usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		//imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	}
	else
	{
		imageInfo.usage = imageInfo.usage | VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	if(canBeTransferDestination)
	{
		imageInfo.usage = imageInfo.usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo finalImageBufferAllocInfo = {};
    finalImageBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalImageBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

    //for(int i = 0; i < bufferCount; i++)
    {
		VMA_ALLOCATE_PROTECT(vmaCreateImage(vulkan->vmaAllocator, &imageInfo, &finalImageBufferAllocInfo, &textureImage, &textureImageAllocation, nullptr));
		//MJLog("vmaCreateImage j:0x%p",textureImage);
		//MJLogBacktrace();


        textureImageView = vulkan->createImageView(textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);
    }

	/*if(forVR)
	{

		VkCommandBuffer tempCommandBuffer = vulkan->beginSingleTimeCommands();
		for(int i = 0; i < bufferCount; i++)
		{
			vulkan->transitionImageLayout(tempCommandBuffer, textureImages[i], format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		}
		vulkan->endSingleTimeCommands(tempCommandBuffer);
	}*/


	for(int i = 0; i < bufferCount; i++)
	{
		textureImageViews.push_back(textureImageView);
	}

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = magFilter;
    samplerInfo.minFilter = minFilter;
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

    if(requiresDepth)
    {
        createDepthResources();
    }

    if(multiSample)
    {
        createMultisampleResources();
    }

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	if(multiSample)
	{
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}
	else
	{
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	}
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if(canBeTransferSource)
	{
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	}
	else
	{
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;

    std::vector<VkAttachmentDescription> attachments;
    attachments.push_back(colorAttachment);

    VkAttachmentDescription depthAttachment = {};
    VkAttachmentReference depthAttachmentRef = {};

    VkAttachmentDescription colorAttachmentMultiSample = {};
    VkAttachmentReference colorAttachmentMultiSampleRef = {};

    if(requiresDepth)
    {
        depthAttachment.format = vulkan->findDepthFormat();
        depthAttachment.samples = renderPass.msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        attachments.push_back(depthAttachment);
    }

    if(multiSample)
    {
        colorAttachmentMultiSample.format = format;
        colorAttachmentMultiSample.samples = renderPass.msaaSamples;
        colorAttachmentMultiSample.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentMultiSample.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentMultiSample.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentMultiSample.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentMultiSample.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentMultiSample.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colorAttachmentMultiSampleRef.attachment = (requiresDepth ? 2 : 1);
        colorAttachmentMultiSampleRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.pResolveAttachments = &colorAttachmentRef;
        subpass.pColorAttachments = &colorAttachmentMultiSampleRef;

        attachments.push_back(colorAttachmentMultiSample);
    }
    else
    {
        subpass.pColorAttachments = &colorAttachmentRef;
    }


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
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(vulkan->device, &renderPassInfo, nullptr, &renderPass.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

   // for(int i = 0; i < bufferCount; i++)
    {
        std::vector<VkImageView> attachmentImageViews;
        attachmentImageViews.push_back(textureImageView);
        if(requiresDepth)
        {
            attachmentImageViews.push_back(depthImageView);
        }
        if(multiSample)
        {
            attachmentImageViews.push_back(msaaColorImageView);
        }

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass.renderPass;
        framebufferInfo.attachmentCount = attachmentImageViews.size();
        framebufferInfo.pAttachments = attachmentImageViews.data();
        framebufferInfo.width = size.x;
        framebufferInfo.height = size.y;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vulkan->device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

}


void MJRenderTarget::createDepthResources() 
{
    VkFormat depthFormat = vulkan->findDepthFormat();

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = size.x;
    imageInfo.extent.height = size.y;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = renderPass.msaaSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo finalImageBufferAllocInfo = {};
    finalImageBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalImageBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

	VMA_ALLOCATE_PROTECT(vmaCreateImage(vulkan->vmaAllocator, &imageInfo, &finalImageBufferAllocInfo, &depthImage, &depthImageAllocation, nullptr));
	//MJLog("vmaCreateImage k:0x%p",depthImage);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(vulkan->device, &viewInfo, nullptr, &depthImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture depth image view!");
    }

    VkCommandBuffer commandBuffer = vulkan->beginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = depthImage;

    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (vulkan->hasStencilComponent(depthFormat)) {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vulkan->endSingleTimeCommands(commandBuffer);

}


void MJRenderTarget::createMultisampleResources() 
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = size.x;
    imageInfo.extent.height = size.y;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples = renderPass.msaaSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo finalImageBufferAllocInfo = {};
    finalImageBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalImageBufferAllocInfo.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	finalImageBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

    VMA_ALLOCATE_PROTECT(vmaCreateImage(vulkan->vmaAllocator, &imageInfo, &finalImageBufferAllocInfo, &msaaColorImage, &msaaColorImageAllocation, nullptr));
	//MJLog("vmaCreateImage l:0x%p",msaaColorImage);

    msaaColorImageView = vulkan->createImageView(msaaColorImage, format, VK_IMAGE_ASPECT_COLOR_BIT);

    VkCommandBuffer tempCommandBuffer = vulkan->beginSingleTimeCommands();
    vulkan->transitionImageLayout(tempCommandBuffer, msaaColorImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vulkan->endSingleTimeCommands(tempCommandBuffer);
}


MJRenderTarget::~MJRenderTarget()
{
    //for(int i = 0; i < vulkan->frameCount(); i++)
    {
        vkDestroyFramebuffer(vulkan->device, framebuffer, nullptr);
        vkDestroyImageView(vulkan->device, textureImageView, nullptr);
        vmaDestroyImage(vulkan->vmaAllocator, textureImage, textureImageAllocation);
    }
    
    if(requiresDepth)
    {
        vkDestroyImageView(vulkan->device, depthImageView, nullptr);
        vmaDestroyImage(vulkan->vmaAllocator, depthImage, depthImageAllocation);
    }

    if(multiSample)
    {
        vkDestroyImageView(vulkan->device, msaaColorImageView, nullptr);
        vmaDestroyImage(vulkan->vmaAllocator, msaaColorImage, msaaColorImageAllocation);
    }

    vkDestroyRenderPass(vulkan->device, renderPass.renderPass, nullptr);
    vkDestroySampler(vulkan->device, textureSampler, nullptr);
}

void MJRenderTarget::lock(GCommandBuffer* commandBuffer, vec4 clearColor)
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass.renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent.width = size.x;
    renderPassInfo.renderArea.extent.height = size.y;

    std::vector<VkClearValue> clearValues;

    VkClearValue colorAttachmentClearValue = {};
	colorAttachmentClearValue.depthStencil = {0.0f, 0};
    if(multiSample)
    {
        colorAttachmentClearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};
    }
    else
    {
        colorAttachmentClearValue.color = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
    }
    clearValues.push_back(colorAttachmentClearValue);

    if(requiresDepth)
    {
        VkClearValue depthAttachmentClearValue = {};
        depthAttachmentClearValue.depthStencil = {0.0f, 0};
        clearValues.push_back(depthAttachmentClearValue);
    }
    if(multiSample)
    {
        VkClearValue multiSampleAttachmentClearValue = {};
		multiSampleAttachmentClearValue.depthStencil = {0.0f, 0};
        multiSampleAttachmentClearValue.color = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
        clearValues.push_back(multiSampleAttachmentClearValue);
    }

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
    renderPassInfo.pClearValues = clearValues.data();

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
}

void MJRenderTarget::unlock(GCommandBuffer* commandBuffer)
{
    vkCmdEndRenderPass(*commandBuffer->getBuffer());
}
