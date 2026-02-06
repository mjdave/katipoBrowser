
#include "MJDrawable.h"

#include "MathUtils.h"
#include "MJCache.h"
#include "GCommandBuffer.h"

MJDrawable::MJDrawable(MJCache* cache_, 
	MJRenderPass renderPass_,
    std::string pipelineName_, 
    int renderGroupCount_, 
    std::vector<VkVertexInputBindingDescription> bindingDescriptions_,
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions_,
	int renderPassCompatibilityIndex_)
{
    renderGroupCount = renderGroupCount_;
    cache = cache_;
    vulkan = cache->vulkan;

    renderPass = renderPass_;
	renderPassCompatibilityIndex = renderPassCompatibilityIndex_;
    pipelineName = pipelineName_;
    bindingDescriptions = bindingDescriptions_;
    attributeDescriptions = attributeDescriptions_;

	int setsCount = 1;

	for(int i = 0; i < setsCount; i++)
	{
		descriptorArrays[i].resize(renderGroupCount);
		descriptorSets[i].resize(renderGroupCount);
		for(int s = 0; s < renderGroupCount; s++)
		{
			descriptorArrays[i][s].resize(vulkan->frameCount());
		}
	}

}


MJDrawable::~MJDrawable()
{
    if(descriptorPool)
    {
        vulkan->destroyDescriptorPool(descriptorPool);
    }

    for(auto kv : dynamicUBOBuffers)
    {
        for(auto buffer : kv.second)
        {
            vulkan->destroyDynamicMultiBuffer(buffer);
        }
    }
}


void MJDrawable::addDynamicUBO(size_t uboSize, std::string identifier, VkShaderStageFlags stageFlags)
{
    if(!dynamicUBOBuffers[identifier].empty())
    {
        MJLog("ERROR: attempt to use duplicate identifier in MJDrawable addDynamicUBO");
        return;
    }
    dynamicUBOBuffers[identifier].resize(renderGroupCount);
    for(int s = 0; s < renderGroupCount; s++)
    {
        int descriptorSlotIndex = dynamicUBOBuffers[identifier][s].size();
        dynamicUBOBuffers[identifier][s].resize(descriptorSlotIndex + 1);

		std::string debugName = "MJDrawable::addDynamicUBO_";
		debugName = debugName + pipelineName;

        vulkan->setupDynamicBufferForMainOutput(&dynamicUBOBuffers[identifier][s], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboSize, debugName.c_str());

        for(size_t i = 0; i < vulkan->frameCount(); i++)
        {
            descriptorArrays[0][s][i].push_back(MJVKDescriptor(dynamicUBOBuffers[identifier][s][i].buffer, uboSize));
        }
    }
	loadingBindIndex++;

    uniforms.push_back(PipelineUniform(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlags));
}

void MJDrawable::addExternalStaticUBO(MJVMABuffer buffer, size_t size, VkShaderStageFlags stageFlags)
{
    for(int s = 0; s < renderGroupCount; s++)
    {
        for(size_t i = 0; i < vulkan->frameCount(); i++)
        {
            descriptorArrays[0][s][i].push_back(MJVKDescriptor(buffer.buffer, size));
        }
    }
	loadingBindIndex++;

    uniforms.push_back(PipelineUniform(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlags));
}

void MJDrawable::addExternalDynamicUBO(std::vector<MJVMABuffer> buffers, size_t size, VkShaderStageFlags stageFlags)
{
    for(int s = 0; s < renderGroupCount; s++)
    {
        for(size_t i = 0; i < vulkan->frameCount(); i++)
        {
            descriptorArrays[0][s][i].push_back(MJVKDescriptor(buffers[i].buffer, size));
        }
    }
	loadingBindIndex++;

    uniforms.push_back(PipelineUniform(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlags));
}

void MJDrawable::addTexture(VkImageView imageView, VkSampler sampler, std::string identifier, VkShaderStageFlags stageFlags)
{
    for(int s = 0; s < renderGroupCount; s++)
    {
        for(size_t i = 0; i < vulkan->frameCount(); i++)
        {
            descriptorArrays[0][s][i].push_back(MJVKDescriptor(imageView, sampler));
        }
    }

	imageIdentifierToBindingIndexMap[identifier] = loadingBindIndex;
	loadingBindIndex++;

    uniforms.push_back(PipelineUniform(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags));
}

void MJDrawable::addRenderTarget(std::vector<VkImageView> imageViews, VkSampler sampler, VkShaderStageFlags stageFlags)
{
    for(int s = 0; s < renderGroupCount; s++)
    {
        for(size_t i = 0; i < vulkan->frameCount(); i++)
        {
            descriptorArrays[0][s][i].push_back(MJVKDescriptor(imageViews[i], sampler));
        }
    }
	loadingBindIndex++;

    uniforms.push_back(PipelineUniform(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags));
}

void MJDrawable::addStereoRenderTargets(std::vector<VkImageView> imageViewsLeft, VkSampler samplerLeft, std::vector<VkImageView> imageViewsRight, VkSampler samplerRight, VkShaderStageFlags stageFlags)
{
	for(int s = 0; s < renderGroupCount; s++)
	{
		for(size_t i = 0; i < vulkan->frameCount(); i++)
		{
			descriptorArrays[0][s][i].push_back(MJVKDescriptor(imageViewsLeft[i], samplerLeft));
		}
	}
	loadingBindIndex++;

	uniforms.push_back(PipelineUniform(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags));
}

void MJDrawable::addRenderTargetArray(std::vector<std::vector<VkImageView>> imageViewArrays, VkSampler sampler, VkShaderStageFlags stageFlags)
{
    for(int s = 0; s < renderGroupCount; s++)
    {
        for(size_t i = 0; i < vulkan->frameCount(); i++)
        {
            descriptorArrays[0][s][i].push_back(MJVKDescriptor(imageViewArrays[i], sampler));
        }
    }

	loadingBindIndex += imageViewArrays[0].size();

    uniforms.push_back(PipelineUniform(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags, imageViewArrays[0].size()));
}

void MJDrawable::finalize()
{
    if(descriptorPool)
    {
        MJLog("WARNING: calling finalize twice on drawable");
        return;
    }



    pipeline = cache->getPipeline(pipelineName, renderPass, bindingDescriptions, attributeDescriptions, uniforms, renderPassCompatibilityIndex);

	if(renderGroupCount > 0 && !descriptorArrays[0][0][0].empty())
	{
		int setsCount = 1;

		descriptorPool = pipeline->createDescriptorPool(vulkan->frameCount() * renderGroupCount * setsCount);
		for(int s = 0; s < renderGroupCount; s++)
		{
			descriptorSets[0][s] = pipeline->createDescriptorSets(descriptorPool, descriptorArrays[0][s], vulkan->frameCount());
		}
	}

    descriptorArrays[0].clear();
	descriptorArrays[1].clear();
}

MJVMABuffer MJDrawable::getDynamicUBO(int renderGroupIndex, std::string identifier)
{
    return dynamicUBOBuffers[identifier][renderGroupIndex][vulkan->primaryCommandBuffer->currentIndex];
}

std::vector<MJVMABuffer>* MJDrawable::getDynamicUBOs(int renderGroupIndex, std::string identifier)
{
	return &(dynamicUBOBuffers[identifier][renderGroupIndex]);
}


VkDescriptorSet MJDrawable::getDescriptorSet(int renderGroupIndex)
{
    int vrSetIndex = 0;
	
	if(imagesNeedingUpdate[vrSetIndex].count(renderGroupIndex) != 0)
	{
		std::map<int, std::map<std::string,MJVKDescriptor >>& imagesNeedingUpdateByFrameIndex = imagesNeedingUpdate[vrSetIndex][renderGroupIndex];
		if(imagesNeedingUpdateByFrameIndex.count(vulkan->primaryCommandBuffer->currentIndex) != 0)
		{
			std::map<std::string,MJVKDescriptor>& descriptorsByIdenitifier = imagesNeedingUpdateByFrameIndex[vulkan->primaryCommandBuffer->currentIndex];

			for(auto & identifierAndDescriptor : descriptorsByIdenitifier)
			{
				const std::string& identifier = identifierAndDescriptor.first;
				const MJVKDescriptor& descriptor = identifierAndDescriptor.second;
				int bindingIndex = imageIdentifierToBindingIndexMap[identifier];
				pipeline->updateSingleDescriptorSet(descriptorSets[vrSetIndex][renderGroupIndex][vulkan->primaryCommandBuffer->currentIndex], descriptor, bindingIndex);
			}

			imagesNeedingUpdateByFrameIndex.erase(vulkan->primaryCommandBuffer->currentIndex);

			if(imagesNeedingUpdateByFrameIndex.empty())
			{
				imagesNeedingUpdate[vrSetIndex].erase(renderGroupIndex);
			}
		}
	}

    return descriptorSets[vrSetIndex][renderGroupIndex][vulkan->primaryCommandBuffer->currentIndex];
}

void MJDrawable::updateTexture(VkImageView imageView, VkSampler sampler, std::string identifier)
{
	for(int s = 0; s < renderGroupCount; s++)
	{
		for(size_t i = 0; i < vulkan->frameCount(); i++)
		{
			imagesNeedingUpdate[0][s][i][identifier] = MJVKDescriptor(imageView, sampler);
		}
	}
} 
