//
//  MJImageTexture.h
//  World
//
//  Created by David Frampton on 3/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#ifndef __World__MJImageTexture__
#define __World__MJImageTexture__

#include "Vulkan.h"
#include "MathUtils.h"
#include <string>

struct MJImageTextureOptions {
	bool repeat = false;
	bool loadFlipped = false;
	bool mipmap = false;
	VkFilter magFilter = VK_FILTER_LINEAR;
	VkFilter minFilter = VK_FILTER_LINEAR;
	VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
};

class MJImageTexture {
public:
    std::string pathname;
	std::string alphaChannelPath;

	MJImageTextureOptions options;
	bool loaded = false;
	bool valid = false;
    
    ivec2 sizei;
    dvec2 size;
   // GLuint textureID;
   // GLenum target;

    VkImageView textureImageView;
    VkSampler textureSampler;

public:
    
    MJImageTexture(Vulkan* vulkan_, std::string pathname, MJImageTextureOptions options = MJImageTextureOptions(), std::string alphaChannel_ = "");
    ~MJImageTexture();
    
    void load(VkCommandBuffer commandBuffer);

private:
    Vulkan* vulkan;

    uint32_t mipLevels;
    VkImage textureImage;
    VmaAllocation imageAllocation;

	MJVMABuffer stagingBuffer;
	int texWidth,texHeight;

private:

    void generateMipmaps(VkImage image, VkFormat imageFormat, VkCommandBuffer commandBuffer);
};

#endif /* defined(__World__MJImageTexture__) */
