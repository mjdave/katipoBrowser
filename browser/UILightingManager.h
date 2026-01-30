//
//  UILightingManager.h
//  Ambience
//
//  Created by David Frampton on 7/09/18.
//Copyright © 2018 Majic Jungle. All rights reserved.
//

#ifndef UILightingManager_h
#define UILightingManager_h

#include "Vulkan.h"

class UILightingManager {

public: // members
    VkImageView textureImageView;
    VkSampler textureSampler;
    VkImage textureImage;
    
protected: // members
    Vulkan* vulkan;
    dvec2 size;
    VmaAllocation textureImageAllocation;

  //  GLuint textureID;

public: // functions
    UILightingManager(Vulkan* vulkan_);
    ~UILightingManager();

protected: // functions
    

};

#endif /* UILightingManager_h */
