//
//  MJImageTexture.h
//  World
//
//  Created by David Frampton on 3/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#ifndef __World__MJRawImageTexture3D__
#define __World__MJRawImageTexture3D__

#include "Vulkan.h"
#include "MathUtils.h"
#include <string>

class MJRawImageTexture3D {
public:
    std::string pathname;
    
    ivec3 size;

    VkImageView textureImageView;
    VkSampler textureSampler;

public:
    
    MJRawImageTexture3D(Vulkan* vulkan_, std::string pathname, ivec3 size, VkFormat format);
    ~MJRawImageTexture3D();

private:
    Vulkan* vulkan;
    VkFormat format;

    VkImage textureImage;
    VmaAllocation imageAllocation;

private:
};

#endif /* defined(__World__MJImageTexture__) */
