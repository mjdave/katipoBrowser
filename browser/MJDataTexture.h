//
//  MJImageTexture.h
//  World
//
//  Created by David Frampton on 3/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#ifndef __World__MJDataTexture__
#define __World__MJDataTexture__

#include "Vulkan.h"
#include "MathUtils.h"
#include <string>

class MJDataTexture {
public:
    bool repeat;
    bool loadFlipped;
    bool mipmap;
	bool loaded = false;
    
    ivec2 sizei;
    dvec2 size;

    VkImageView textureImageView;
    VkSampler textureSampler;

public:
    
	MJDataTexture(Vulkan* vulkan_, ivec2 sizei_, void* data, bool repeat = false, bool mipmap_ = false, VkFilter minFilter = VK_FILTER_LINEAR, VkFilter magFilter = VK_FILTER_LINEAR);
    ~MJDataTexture();

private:
    Vulkan* vulkan;

    uint32_t mipLevels;
    VkImage textureImage;
    VmaAllocation imageAllocation;

private:

    void generateMipmaps(VkImage image, VkFormat imageFormat);
};

#endif /* defined(__World__MJImageTexture__) */
