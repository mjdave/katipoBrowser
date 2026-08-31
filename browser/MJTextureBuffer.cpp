#include "MJTextureBuffer.h"
#include "Vulkan.h"
#include "GCommandBuffer.h"



MJTextureBuffer::MJTextureBuffer(Vulkan* vulkan_, ivec2 size_, std::string debugName_)
{
    vulkan = vulkan_;

    writeBufferIndex = 0;
    readBufferIndex = -1;
    debugName = debugName_;
	size = size_;


    for(int i = 0; i < MJTB_FRAMEBUFFER_COUNT; i++)
    {
        allocateBuffers(i);
		used[i] = false;
    }
}

MJTextureBuffer::~MJTextureBuffer()
{
	for(int i = 0; i < MJTB_FRAMEBUFFER_COUNT; i++)
	{
		vulkan->destroySingleImage(gpuImages[i]);
		vulkan->destroySingleBuffer(cpuBuffers[i]);
	}
}

void MJTextureBuffer::allocateBuffers(int bufferIndex)
{
	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;


#if DEBUG_VKBUFFER_ALLOCATIONS
	if(!debugName.empty())
	{
		stagingAllocInfo.flags = stagingAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
		stagingAllocInfo.pUserData = (void*)(debugName.c_str());
	}
#endif

	VkDeviceSize imageSize = size.x * size.y * sizeof(u8vec4);


	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = imageSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vmaCreateBuffer(vulkan->vmaAllocator, &bufferInfo, &stagingAllocInfo,  &(cpuBuffers[bufferIndex].buffer), &(cpuBuffers[bufferIndex].allocation), &allocInfos[bufferIndex]);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to createVMABuffer!");
	}

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = size.x;
	imageInfo.extent.height = size.y;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	
	/*uint32_t queueFamilyIndicesToUse[] = {vulkan->queueFamilyIndices.graphicsFamily, vulkan->queueFamilyIndices.transferFamily};

	if (vulkan->queueFamilyIndices.graphicsFamily != vulkan->queueFamilyIndices.transferFamily) {
		imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		imageInfo.queueFamilyIndexCount = 2;
		imageInfo.pQueueFamilyIndices = queueFamilyIndicesToUse;
	} else {
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}*/

	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo finalImageBufferAllocInfo = {};
	finalImageBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalImageBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

	VMA_ALLOCATE_PROTECT(vmaCreateImage(vulkan->vmaAllocator, &imageInfo, &finalImageBufferAllocInfo, &(gpuImages[bufferIndex].image), &(gpuImages[bufferIndex].allocation), &(gpuImages[bufferIndex].allocInfo)));
	//MJLog("vmaCreateImage o:0x%p",gpuImages[bufferIndex].image);


	VkCommandBuffer tempCommandBuffer = vulkan->beginSingleTimeCommands();
	vulkan->transitionImageLayout(tempCommandBuffer, gpuImages[bufferIndex].image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


	{

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = vulkan->queueFamilyIndices.graphicsFamily; //VK_QUEUE_FAMILY_IGNORED
		barrier.dstQueueFamilyIndex = vulkan->queueFamilyIndices.transferFamily;
		barrier.image = gpuImages[bufferIndex].image;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;


		vkCmdPipelineBarrier(
			tempCommandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}
	vulkan->endSingleTimeCommands(tempCommandBuffer);

	imageViews[bufferIndex] = vulkan->createImageView(gpuImages[bufferIndex].image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	if (vkCreateSampler(vulkan->device, &samplerInfo, nullptr, &samplers[bufferIndex]) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void* MJTextureBuffer::getBuffer()
{
	void* mappedData;
	vmaMapMemory(vulkan->vmaAllocator, cpuBuffers[writeBufferIndex].allocation, &mappedData);
	mapped = true;
    return mappedData;
}

void MJTextureBuffer::copyToGPU(VkCommandBuffer commandBuffer)
{
	if(!mapped)
	{
		return;
	}
	vmaUnmapMemory(vulkan->vmaAllocator, cpuBuffers[writeBufferIndex].allocation);
	mapped = false;
	readBufferIndex = writeBufferIndex;
	writeBufferIndex = (writeBufferIndex + 1) % MJTB_FRAMEBUFFER_COUNT;

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {
		(uint32_t)size.x,
		(uint32_t)size.y,
		1
	};


	//if(used[readBufferIndex])
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = vulkan->queueFamilyIndices.graphicsFamily; //VK_QUEUE_FAMILY_IGNORED
		barrier.dstQueueFamilyIndex = vulkan->queueFamilyIndices.transferFamily;
		barrier.image = gpuImages[readBufferIndex].image;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;


		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}



	vkCmdCopyBufferToImage(commandBuffer, cpuBuffers[readBufferIndex].buffer, gpuImages[readBufferIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	//vulkan->transitionImageLayout(commandBuffer, gpuImages[readBufferIndex].image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	//vulkan->transitionImageLayout(commandBuffer, gpuImages[writeBufferIndex].image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = vulkan->queueFamilyIndices.transferFamily;
		barrier.dstQueueFamilyIndex = vulkan->queueFamilyIndices.graphicsFamily;
		barrier.image = gpuImages[readBufferIndex].image;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;


		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
		used[readBufferIndex] = true;

		//MJLog("barrier:%p", barrier.image);
	}


	/*i
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;



	} else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else {
		VULKAN_ERROR("unsupported layout transition!");
	}*/

}


MJTextureBufferGPUImage MJTextureBuffer::getGPUImage()
{
    MJTextureBufferGPUImage mainThreadBuffer = {};
	mainThreadBuffer.image = gpuImages[readBufferIndex];
	mainThreadBuffer.imageView = imageViews[readBufferIndex];
	mainThreadBuffer.sampler = samplers[readBufferIndex];

    return mainThreadBuffer;
}
