#pragma once

#include "Vulkan.h"

#include <string>
#include <vector>
#include "MathUtils.h"

class Database;

enum {
    MJVK_BLEND_MODE_PREMULTIPLED = 0,
    MJVK_BLEND_MODE_ADDITIVE,
    MJVK_BLEND_MODE_TREAT_ALPHA_AS_COLOR_CHANNEL,
    MJVK_BLEND_MODE_NONPREMULTIPLED,
    MJVK_BLEND_MODE_DISABLED,
	MJVK_BLEND_MODE_SUBTRACTIVE
};
enum {
    MJVK_DEPTH_DISABLED = 0,
    MJVK_DEPTH_TEST_ONLY,
    MJVK_DEPTH_MASK_ONLY,
    MJVK_DEPTH_TEST_AND_MASK,
	MJVK_DEPTH_TEST_AND_MASK_EQUAL
};
enum {
    MJVK_CULL_DISABLED = 0,
    MJVK_CULL_FRONT,
    MJVK_CULL_BACK
};

enum {
    MJVK_TRIANGLES = 0,
    MJVK_POINTS,
	MJVK_LINES,
	MJVK_FAN,
};

struct PipelineStateOptions {
    int blendMode = MJVK_BLEND_MODE_DISABLED;
    int depthMode = MJVK_DEPTH_DISABLED;
    int cullMode = MJVK_CULL_DISABLED;
    int topology = MJVK_TRIANGLES;
    int vertPushConstantSize = 0;
    int fragPushConstantSize = 0;
    bool alphaToCoverage = false;
	float lineWidth = 1.0f;
};


struct PipelineUniform {
    VkDescriptorType type;
    VkShaderStageFlags stageFlags;
    int count;

    PipelineUniform(VkDescriptorType type_, VkShaderStageFlags stageFlags_, int count_ = 1) {
        type = type_;
        stageFlags = stageFlags_;
        count = count_;
    };
};

struct MJVKDescriptor {
    bool isImage = false;
    VkBuffer buffer; 
    size_t bufferSize = 0;

    std::vector<VkImageView> imageViews;
    VkSampler imageSampler;

	MJVKDescriptor() {};

    MJVKDescriptor(VkBuffer buffer_, size_t bufferSize_) {
        isImage = false;
        buffer = buffer_;
        bufferSize = bufferSize_;
    };

    MJVKDescriptor(VkImageView imageView_, VkSampler imageSampler_) {
        isImage = true;
        imageViews.push_back(imageView_);
        imageSampler = imageSampler_;
    };

    MJVKDescriptor(std::vector<VkImageView> imageViews_, VkSampler imageSampler_) {
        isImage = true;
        imageViews = imageViews_;
        imageSampler = imageSampler_;
    };
};

class GPipeline
{
public:
    Vulkan* vulkan;
    VkPipeline vkPipeline;
    VkPipelineLayout pipelineLayout;

    VkDescriptorSetLayout descriptorSetLayout;

public:
    GPipeline(Vulkan* vulkan,
        std::string name,
		MJRenderPass renderPass,
        std::string vertShaderPathname,
        std::string fragShaderPathname,
        std::vector<VkVertexInputBindingDescription> bindingDescriptions,
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
        std::vector<PipelineUniform> uniforms,
        PipelineStateOptions pipelineStateOptions = PipelineStateOptions());
    ~GPipeline();


    VkDescriptorPool createDescriptorPool(int count);
    std::vector<VkDescriptorSet> createDescriptorSets(VkDescriptorPool pool, const std::vector<std::vector<MJVKDescriptor>>& descriptorArrays, int count);
    VkDescriptorSet createDescriptorSet(VkDescriptorPool pool, const std::vector<MJVKDescriptor>& descriptors);
    void updateSingleDescriptorSet(const VkDescriptorSet& descriptorSet, const MJVKDescriptor& descriptor, int bindingIndex);

private:
    std::vector<PipelineUniform> uniforms;

private:

    void createDescriptorSetLayout(); 
    void updateDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets, const std::vector<std::vector<MJVKDescriptor>>& descriptorArrays, int count);
};

