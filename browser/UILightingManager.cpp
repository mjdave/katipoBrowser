

#include "UILightingManager.h"
#include "TuiScript.h"
#include "KatipoUtilities.h"


#define ENV_MAP_SIZE 64
#define PBR_MIP_LEVELS 7
#define ENV_MAP_SIZE_UI 128

//
//pbrMap = new RenderTarget(ivec3(ENV_MAP_SIZE, ENV_MAP_SIZE, 0),
                        //  GL_TEXTURE_CUBE_MAP, GL_RGBA16F, GL_RGBA, GL_FLOAT, false, 1, true);

UILightingManager::UILightingManager(Vulkan* vulkan_)
{
    vulkan = vulkan_;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = ENV_MAP_SIZE_UI;
    imageInfo.extent.height = ENV_MAP_SIZE_UI;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = PBR_MIP_LEVELS;
    imageInfo.arrayLayers = 6;
    imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo finalImageBufferAllocInfo = {};
    finalImageBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalImageBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

	VMA_ALLOCATE_PROTECT(vmaCreateImage(vulkan->vmaAllocator, &imageInfo, &finalImageBufferAllocInfo, &textureImage, &textureImageAllocation, nullptr));
	//MJLog("vmaCreateImage c:0x%p",textureImage);

    VkCommandBuffer tempCommandBuffer = vulkan->beginSingleTimeCommands();
    vulkan->transitionImageLayout(tempCommandBuffer, textureImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, PBR_MIP_LEVELS, 6);

    std::vector<VkBuffer> stagingBuffers(6 * PBR_MIP_LEVELS);
    std::vector<VmaAllocation> stagingBufferAllocations(6 * PBR_MIP_LEVELS);

    for(int face = 0; face < 6; face++)
    {
        int mipSize = ENV_MAP_SIZE_UI;
        for(int mip = 0; mip < PBR_MIP_LEVELS; mip++)
        {
            std::string filePath = Katipo::getResourcePath(Tui::string_format("common/img/objectPBR/pbr_%d_%d.RAW", face, mip));
            std::string uncompressed = Tui::getFileContents(filePath);

            void* floatData = (void*)(uncompressed.data());

            VkDeviceSize imageSize = uncompressed.size();

            VmaAllocationCreateInfo stagingAllocInfo = {};
            stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

#if DEBUG_VKBUFFER_ALLOCATIONS
			stagingAllocInfo.flags = stagingAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
			stagingAllocInfo.pUserData = (void*)(filePath.c_str());
#endif

            int bufferIndex = face * PBR_MIP_LEVELS + mip;

            vulkan->createVMABuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo, &stagingBuffers[bufferIndex], &stagingBufferAllocations[bufferIndex]);

            void* mappedData;
            vmaMapMemory(vulkan->vmaAllocator, stagingBufferAllocations[bufferIndex], &mappedData);
            memcpy(mappedData, floatData, static_cast<size_t>(imageSize));
            vmaUnmapMemory(vulkan->vmaAllocator, stagingBufferAllocations[bufferIndex]);


            VkBufferImageCopy region = {};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = (uint32_t)mip;
            region.imageSubresource.baseArrayLayer = (uint32_t)face;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                (uint32_t)mipSize,
                (uint32_t)mipSize,
                1
            };

            vkCmdCopyBufferToImage(tempCommandBuffer, stagingBuffers[bufferIndex], textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            mipSize = mipSize / 2;
        }
    }

    vulkan->transitionImageLayout(tempCommandBuffer, textureImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, PBR_MIP_LEVELS, 6);
    vulkan->endSingleTimeCommands(tempCommandBuffer);

    for(int i = 0; i < stagingBuffers.size(); i++)
    {
        vmaDestroyBuffer(vulkan->vmaAllocator, stagingBuffers[i], stagingBufferAllocations[i]);
    }

    textureImageView = vulkan->createImageView(textureImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, PBR_MIP_LEVELS, 6, VK_IMAGE_VIEW_TYPE_CUBE);


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
    samplerInfo.maxLod = PBR_MIP_LEVELS;

    if (vkCreateSampler(vulkan->device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}


UILightingManager::~UILightingManager()
{
    vkDestroySampler(vulkan->device, textureSampler, nullptr);
    vkDestroyImageView(vulkan->device, textureImageView, nullptr);
    vmaDestroyImage(vulkan->vmaAllocator, textureImage, textureImageAllocation);
}
