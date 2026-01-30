//
//  DrawMesh.h
//  World
//
//  Created by David Frampton on 1/09/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#ifndef __World__MJ_DRAWABLE__
#define __World__MJ_DRAWABLE__

#include "Vulkan.h"
#include "GPipeline.h"

class MJCache;

class MJDrawable {
public:

    GPipeline* pipeline;
	std::string pipelineName;

	std::map<std::string, std::vector<std::vector<MJVMABuffer>>> dynamicUBOBuffers;
    
public:
    MJDrawable(MJCache* cache, 
        MJRenderPass renderPass,
        std::string pipelineName, 
        int renderGroupCount_, 
        std::vector<VkVertexInputBindingDescription> bindingDescriptions,
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
		int renderPassCompatibilityIndex_);
    ~MJDrawable();

    void addDynamicUBO(size_t uboSize, std::string identifier, VkShaderStageFlags stageFlags);
    void addExternalStaticUBO(MJVMABuffer buffer, size_t size, VkShaderStageFlags stageFlags);
    void addExternalDynamicUBO(std::vector<MJVMABuffer> buffers, size_t size, VkShaderStageFlags stageFlags);
    void addTexture(VkImageView imageView, VkSampler sampler, std::string identifier, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);
    void addRenderTarget(std::vector<VkImageView> imageViews, VkSampler sampler, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);
	void addStereoRenderTargets(std::vector<VkImageView> imageViewsLeft, VkSampler samplerLeft, std::vector<VkImageView> imageViewsRight, VkSampler samplerRight, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);
    void addRenderTargetArray(std::vector<std::vector<VkImageView>> imageViewArrays, VkSampler sampler, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);
	void updateTexture(VkImageView imageView, VkSampler sampler, std::string identifier);

    void finalize();

    MJVMABuffer getDynamicUBO(int renderGroupIndex, std::string identifier);
	std::vector<MJVMABuffer>* getDynamicUBOs(int renderGroupIndex, std::string identifier);
    VkDescriptorSet getDescriptorSet(int renderGroupIndex);

private:
    Vulkan* vulkan;
    int renderGroupCount;

    MJCache* cache;
	MJRenderPass renderPass;
	int renderPassCompatibilityIndex;
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    std::vector<PipelineUniform> uniforms;

    std::vector<std::vector<std::vector<MJVKDescriptor>>> descriptorArrays[2];


    VkDescriptorPool descriptorPool = nullptr;
    std::vector<std::vector<VkDescriptorSet>> descriptorSets[2];

	int loadingBindIndex = 0;
	std::map<std::string, int> imageIdentifierToBindingIndexMap;
	std::map<int, std::map<int, std::map<std::string, MJVKDescriptor>>> imagesNeedingUpdate[2];
    
};


#endif /* defined(__World__DrawMesh__) */
