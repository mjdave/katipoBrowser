
#include "MJRawImageTexture.h"
#include "MJLog.h"
#include "TuiFileUtils.h"


MJRawImageTexture::MJRawImageTexture(Vulkan* vulkan_, std::string pathname_, ivec2 size_, VkFormat format_)
{
    vulkan = vulkan_;
    pathname = pathname_;
    size = size_;
    format = format_;

    std::string fileString = Tui::getFileContents(pathname);

    const char* pixels = fileString.data();
    VkDeviceSize imageSize = fileString.size();


    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferAllocation;
    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;


#if DEBUG_VKBUFFER_ALLOCATIONS
	stagingAllocInfo.flags = stagingAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	stagingAllocInfo.pUserData = (void*)(pathname.c_str());
#endif

    vulkan->createVMABuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo, &stagingBuffer, &stagingBufferAllocation);

    void* mappedData;
    vmaMapMemory(vulkan->vmaAllocator, stagingBufferAllocation, &mappedData);
    memcpy(mappedData, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(vulkan->vmaAllocator, stagingBufferAllocation);

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
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo finalImageBufferAllocInfo = {};
    finalImageBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalImageBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

	VMA_ALLOCATE_PROTECT(vmaCreateImage(vulkan->vmaAllocator, &imageInfo, &finalImageBufferAllocInfo, &textureImage, &imageAllocation, nullptr));
	//MJLog("vmaCreateImage h:0x%p",textureImage);

    VkCommandBuffer tempCommandBuffer = vulkan->beginSingleTimeCommands();
    vulkan->transitionImageLayout(tempCommandBuffer, textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vulkan->copyBufferToImage(tempCommandBuffer, stagingBuffer, textureImage, size.x, size.y);
    vulkan->transitionImageLayout(tempCommandBuffer, textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vulkan->endSingleTimeCommands(tempCommandBuffer);

    vmaDestroyBuffer(vulkan->vmaAllocator, stagingBuffer, stagingBufferAllocation);

    textureImageView = vulkan->createImageView(textureImage, format, VK_IMAGE_ASPECT_COLOR_BIT);

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
}

MJRawImageTexture::~MJRawImageTexture()
{
    vkDestroySampler(vulkan->device, textureSampler, nullptr);
    vkDestroyImageView(vulkan->device, textureImageView, nullptr);
    vmaDestroyImage(vulkan->vmaAllocator, textureImage, imageAllocation);
}
