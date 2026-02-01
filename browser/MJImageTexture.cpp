//
//  MJImageTexture.cpp
//  World
//
//  Created by David Frampton on 3/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#include "MJImageTexture.h"
#include "MJLog.h"
#include "TuiStringUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#include "stb_image.h"



MJImageTexture::MJImageTexture(Vulkan* vulkan_, std::string pathname_, MJImageTextureOptions options_, std::string alphaChannel_)
{
    vulkan = vulkan_;
    pathname = pathname_;
	options = options_;
	alphaChannelPath = alphaChannel_;
    
	int channels;
	stbi_set_flip_vertically_on_load(!options.loadFlipped);
	//unsigned char* pixels = stbi_load(pathname.c_str(), &texWidth, &texHeight, &channels, 4);

	unsigned char* pixels = NULL;
#ifdef WIN32
	FILE *f = _wfopen(Tui::convertUtf8ToWide(pathname).c_str(), L"rb");
#else
    FILE *f = fopen(pathname.c_str(), "rb");
#endif
	if(f)
	{
		pixels = stbi_load_from_file(f,&texWidth, &texHeight, &channels, 4);
		fclose(f);
	}

	if(!pixels)
	{
		MJError("Error loading image at path:%s", pathname.c_str());
		return;
	}

	if(options.mipmap)
	{
		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	}
	else
	{
		mipLevels = 1;
	}

	if(!alphaChannelPath.empty())
	{
		int alphaTexWidth,alphaTexHeight,alphaChannels;
		//unsigned char* alphaPixels = stbi_load(alphaChannelPath.c_str(), &alphaTexWidth, &alphaTexHeight, &alphaChannels, 1);

		unsigned char* alphaPixels = NULL;
#ifdef WIN32
		FILE *f = _wfopen(Tui::convertUtf8ToWide(alphaChannelPath).c_str(), L"rb");
#else
        FILE *f = fopen(alphaChannelPath.c_str(), "rb");
#endif
		if(f)
		{
			alphaPixels = stbi_load_from_file(f,&alphaTexWidth, &alphaTexHeight, &alphaChannels, 1);
			fclose(f);
		}

		if(!alphaPixels)
		{
			MJError("Error loading alpha channel image at path:%s", alphaChannelPath.c_str());
		}
		else
		{
			if(alphaTexWidth != texWidth || alphaTexHeight != texHeight)
			{
				MJError("Failed to load seperate alpha channel, dimensions don't match: %s, %s", pathname.c_str(), alphaChannelPath.c_str());
			}
			else
			{
				for(int i = 0; i < texWidth * texHeight; i++)
				{
					pixels[i * 4 + 3] = alphaPixels[i];
				}
			}
			stbi_image_free(alphaPixels);
		}
	}

	size.x = sizei.x = texWidth;
	size.y = sizei.y = texHeight;
    
    if(texWidth < 20)
    {
        MJLog("hmm");
    }

	VkDeviceSize imageSize = texWidth * texHeight * 4;



	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;


#if DEBUG_VKBUFFER_ALLOCATIONS
	stagingAllocInfo.flags = stagingAllocInfo.flags | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
	stagingAllocInfo.pUserData = (void*)(pathname.c_str());
#endif

	vulkan->createVMABuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo, &stagingBuffer.buffer, &stagingBuffer.allocation);

	void* mappedData;
	vmaMapMemory(vulkan->vmaAllocator, stagingBuffer.allocation, &mappedData);
	memcpy(mappedData, pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(vulkan->vmaAllocator, stagingBuffer.allocation);

	stbi_image_free(pixels);

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = texWidth;
	imageInfo.extent.height = texHeight;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	if(options.mipmap)
	{
		imageInfo.usage = imageInfo.usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo finalImageBufferAllocInfo = {};
	finalImageBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	finalImageBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

	VMA_ALLOCATE_PROTECT(vmaCreateImage(vulkan->vmaAllocator, &imageInfo, &finalImageBufferAllocInfo, &textureImage, &imageAllocation, nullptr));

	//MJLog("textureImage is:0x%p at path:%s", textureImage, pathname_);

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = options.magFilter;
	samplerInfo.minFilter = options.minFilter;
	if(options.repeat)
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
	else
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = options.mipmapMode;

	if(options.mipmap)
	{
		samplerInfo.maxLod = mipLevels;
	}

	if (vkCreateSampler(vulkan->device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}

	textureImageView = vulkan->createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	valid = true;
}


void MJImageTexture::generateMipmaps(VkImage image, VkFormat imageFormat, VkCommandBuffer commandBuffer) 
{
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(vulkan->physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }


    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = size.x;
    int32_t mipHeight = size.y;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit = {};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}

void MJImageTexture::load(VkCommandBuffer commandBuffer)
{
	if(valid && !loaded)
	{
		vulkan->transitionImageLayout(commandBuffer, textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
		vulkan->copyBufferToImage(commandBuffer, stagingBuffer.buffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		if(!options.mipmap)
		{
			vulkan->transitionImageLayout(commandBuffer, textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
		}


		vulkan->destroySingleBuffer(stagingBuffer);

		if(options.mipmap)
		{
			generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_UNORM, commandBuffer);
		}
		loaded = true;
	}
}

MJImageTexture::~MJImageTexture()
{
	if(valid)
	{
		vkDestroySampler(vulkan->device, textureSampler, nullptr);
		vkDestroyImageView(vulkan->device, textureImageView, nullptr);

		vmaDestroyImage(vulkan->vmaAllocator, textureImage, imageAllocation);
	}
}
