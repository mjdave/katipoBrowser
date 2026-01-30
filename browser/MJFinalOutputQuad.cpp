#include "MJFinalOutputQuad.h"
#include "Vulkan.h"
#include "GPipeline.h"
#include "GCommandBuffer.h"
#include "MJCache.h"
#include "MJImageTexture.h"
#include "MJRenderTarget.h"
#include "MJDrawable.h"
#include "Camera.h"
#include <array>


MJFinalOutputQuad::MJFinalOutputQuad(MJCache* cache_, MJRenderPass renderPass, MJRenderTarget* renderTarget_, MJRenderTarget* bloomRenderTarget)
{
    cache = cache_;
    renderTarget = renderTarget_;

    std::string shaderNameToUse = "finalOutputDrawQuad";

	drawable = new MJDrawable(cache, 
		renderPass,
		shaderNameToUse, 
		1, 
		DrawQuadVertex::getBindingDescriptions(), 
		DrawQuadVertex::getAttributeDescriptions(),
		0);

	drawable->addRenderTarget(renderTarget->textureImageViews, renderTarget->textureSampler);
	drawable->addRenderTarget(bloomRenderTarget->textureImageViews, bloomRenderTarget->textureSampler);

	drawable->finalize();
}

MJFinalOutputQuad::~MJFinalOutputQuad()
{
	if(drawable)
	{
		delete drawable;
	}
}

void MJFinalOutputQuad::render(GCommandBuffer* mainCommandBuffer)
{
	if(!drawable || !renderTarget)
	{
		return;
	}

	GPipeline* pipeline = drawable->pipeline;
	Vulkan* vulkan = pipeline->vulkan;
	VkDescriptorSet descriptorSet = drawable->getDescriptorSet(0);
	const VkCommandBuffer* commandBuffer =  mainCommandBuffer->getBuffer();


	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);

	VkBuffer vertexBuffers[] = {vulkan->drawQuadVertBuffer.buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(*commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

	vkCmdDraw(*commandBuffer, 6, 1, 0, 0);
}
