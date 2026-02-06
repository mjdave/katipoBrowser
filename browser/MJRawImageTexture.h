
#ifndef __World__MJRawImageTexture__
#define __World__MJRawImageTexture__

#include "Vulkan.h"
#include "MathUtils.h"
#include <string>

class MJRawImageTexture {
public:
    std::string pathname;
    
    ivec2 size;

    VkImageView textureImageView;
    VkSampler textureSampler;

public:
    
    MJRawImageTexture(Vulkan* vulkan_, std::string pathname, ivec2 size, VkFormat format);
    ~MJRawImageTexture();

private:
    Vulkan* vulkan;
    VkFormat format;

    VkImage textureImage;
    VmaAllocation imageAllocation;

private:
};

#endif /* defined(__World__MJImageTexture__) */
