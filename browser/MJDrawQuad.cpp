#include "MJDrawQuad.h"
#include "Vulkan.h"
#include "GPipeline.h"
#include "GCommandBuffer.h"
#include "MJCache.h"
#include "MJImageTexture.h"
#include "MJRenderTarget.h"
#include "MJDrawable.h"
#include "Camera.h"
#include <array>

struct DrawQuadUBO{
    mat4 mvpMatrix;
    vec4 color;
    vec4 scale;
    vec4 userData;
};

struct DrawQuadClipUBO{
	mat4 mvpMatrix;
	vec4 color;
	vec4 scale;
    vec4 userData;
	mat4 clipMatrix;
};


struct DrawQuadTexturedUBO {
    mat4 mvpMatrix;
    vec4 color;
    vec4 scale_greyFraction;
    vec4 userData;
    vec4 texMinAndScale;
};

struct DrawQuadTexturedClipUBO {
	mat4 mvpMatrix;
	vec4 color;
	vec4 scale_greyFraction;
    vec4 userData;
	vec4 texMinAndScale;
	mat4 clipMatrix;
};


MJDrawQuad::MJDrawQuad(MJCache* cache_, MJRenderPass renderPass, int renderTargetCompatibilityIndex, bool clip, vec2 size_)
{
    cache = cache_;
	usingClipUBO = clip;
    imageTexture = nullptr;
    std::vector<PipelineUniform> uniforms;

	drawable = new MJDrawable(cache, 
		renderPass,
		(clip ? "drawQuadClip" : "drawQuad"), 
		1, 
		DrawQuadVertex::getBindingDescriptions(), 
		DrawQuadVertex::getAttributeDescriptions(),
		renderTargetCompatibilityIndex);

	if(clip)
	{
		drawable->addDynamicUBO(sizeof(DrawQuadClipUBO), "v0", VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	else
	{
		drawable->addDynamicUBO(sizeof(DrawQuadUBO), "v0", VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
    
    drawable->addExternalDynamicUBO(cache->getCameraBuffer(), sizeof(CameraUBO), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	drawable->finalize();

    size = size_;
}

MJDrawQuad::MJDrawQuad(MJCache* cache_, MJRenderPass renderPass, int renderTargetCompatibilityIndex, bool clip, MJImageTexture* imageTexture_, vec2 size_, vec2 texMin_, vec2 texMax_, std::string shaderName)
{
    cache = cache_;
	usingClipUBO = clip;
    imageTexture = imageTexture_;
	textured = true;
	texMin = texMin_;
	texMax = texMax_;
	size = size_;

	if(!imageTexture->loaded)
	{
        MJError("MJDrawQuad init called when texture hasn't been loaded yet.");
		return;
	}
    
    std::string shaderNameToUse = shaderName;
    if(shaderNameToUse == "")
    {
        shaderNameToUse = (clip ? "drawQuadTexturedClip" : "drawQuadTextured");
    }

	drawable = new MJDrawable(cache, 
		renderPass,
                              shaderNameToUse,
		1, 
		DrawQuadVertex::getBindingDescriptions(), 
		DrawQuadVertex::getAttributeDescriptions(),
		renderTargetCompatibilityIndex);

	if(clip)
	{
		drawable->addDynamicUBO(sizeof(DrawQuadTexturedClipUBO), "v0", VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	else
	{
		drawable->addDynamicUBO(sizeof(DrawQuadTexturedUBO), "v0", VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
    
    drawable->addExternalDynamicUBO(cache->getCameraBuffer(), sizeof(CameraUBO), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    
	drawable->addTexture(imageTexture->textureImageView, imageTexture->textureSampler, "t0");

	drawable->finalize();

}

MJDrawQuad::MJDrawQuad(MJCache* cache_, MJRenderPass renderPass, int renderTargetCompatibilityIndex, bool clip, MJRenderTarget* renderTarget_, vec2 size_, vec2 texMin_, vec2 texMax_, std::string shaderName)
{
    cache = cache_;
	usingClipUBO = clip;
    renderTarget = renderTarget_;

    std::string shaderNameToUse = shaderName;
    if(shaderNameToUse == "")
    {
        shaderNameToUse = "drawQuadTextured";
		if(clip)
		{
			shaderNameToUse = shaderNameToUse + "Clip";
		}
    }


	drawable = new MJDrawable(cache, 
		renderPass,
		shaderNameToUse, 
		1, 
		DrawQuadVertex::getBindingDescriptions(), 
		DrawQuadVertex::getAttributeDescriptions(),
		renderTargetCompatibilityIndex);

	if(clip)
	{
		drawable->addDynamicUBO(sizeof(DrawQuadTexturedClipUBO), "v0", VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	else
	{
		drawable->addDynamicUBO(sizeof(DrawQuadTexturedUBO), "v0", VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
    
    drawable->addExternalDynamicUBO(cache->getCameraBuffer(), sizeof(CameraUBO), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	drawable->addRenderTarget(renderTarget->textureImageViews, renderTarget->textureSampler);

	drawable->finalize();

    textured = true;
    texMin = texMin_;
    texMax = texMax_;
    size = size_;
}

MJDrawQuad::MJDrawQuad(MJCache* cache_, MJRenderPass renderPass, int renderTargetCompatibilityIndex, bool clip, MJRenderTarget* renderTarget_, MJImageTexture* maskTexture_, vec2 size_, vec2 texMin_, vec2 texMax_, std::string shaderName)
{
	cache = cache_;
	usingClipUBO = clip;
	renderTarget = renderTarget_;
	maskTexture = maskTexture_;

	std::string shaderNameToUse = shaderName;
	if(shaderNameToUse == "")
	{
		shaderNameToUse = "drawQuadTexturedNonPremultipledMasked";
		if(clip)
		{
			shaderNameToUse = shaderNameToUse + "Clip";
		}
	}


	drawable = new MJDrawable(cache, 
		renderPass,
		shaderNameToUse, 
		1, 
		DrawQuadVertex::getBindingDescriptions(), 
		DrawQuadVertex::getAttributeDescriptions(),
		renderTargetCompatibilityIndex);

	if(clip)
	{
		drawable->addDynamicUBO(sizeof(DrawQuadTexturedClipUBO), "v0", VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	else
	{
		drawable->addDynamicUBO(sizeof(DrawQuadTexturedUBO), "v0", VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
    drawable->addExternalDynamicUBO(cache->getCameraBuffer(), sizeof(CameraUBO), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	drawable->addRenderTarget(renderTarget->textureImageViews, renderTarget->textureSampler);
	drawable->addTexture(maskTexture->textureImageView, maskTexture->textureSampler, "t0");

	drawable->finalize();

	textured = true;
	texMin = texMin_;
	texMax = texMax_;
	size = size_;
}




MJDrawQuad::~MJDrawQuad()
{
	if(drawable)
	{
		delete drawable;
	}
}

void MJDrawQuad::setSize(vec2 newSize)
{
    size = newSize;
}

void MJDrawQuad::setTexCoords(vec2 texMin_, vec2 texMax_)
{
    texMin = texMin_;
    texMax = texMax_;
}

void MJDrawQuad::render(GCommandBuffer* mainCommandBuffer)
{
	if(!drawable || (textured && (!imageTexture && !renderTarget && !vrRenderTarget)))
	{
		return;
	}
	if(needsToReloadTexture && imageTexture && imageTexture->loaded)
	{
		drawable->updateTexture(imageTexture->textureImageView, imageTexture->textureSampler, "t0");
		needsToReloadTexture = false;
	}
	if(imageTexture && !imageTexture->loaded)
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


void MJDrawQuad::updateUBOs(mat4 mvpMatrix, dmat4 clipMatrix, vec4 color, bool greyScale)
{
	if(!drawable)
	{
		return;
	}
	MJVMABuffer vUBOBuffer = drawable->getDynamicUBO(0, "v0");
	if(textured)
	{
		if(usingClipUBO)
		{
			DrawQuadTexturedClipUBO vubo = {};
			vubo.mvpMatrix = mvpMatrix;
			vubo.color = color;
			vubo.scale_greyFraction = vec4(size.x, size.y, (greyScale ? 1.0 : 0.0), 1.0);
            vubo.userData = userData;
			vubo.texMinAndScale = vec4(texMin.x, texMin.y, texMax.x - texMin.x, texMax.y - texMin.y);
			vubo.clipMatrix = clipMatrix;
			mjCopyGPUMemory(cache->vulkan->vmaAllocator, vUBOBuffer, vubo);
		}
		else
		{
			DrawQuadTexturedUBO vubo = {};
			vubo.mvpMatrix = mvpMatrix;
			vubo.color = color;
			vubo.scale_greyFraction = vec4(size.x, size.y, (greyScale ? 1.0 : 0.0), 1.0);
            vubo.userData = userData;
			vubo.texMinAndScale = vec4(texMin.x, texMin.y, texMax.x - texMin.x, texMax.y - texMin.y);
			mjCopyGPUMemory(cache->vulkan->vmaAllocator, vUBOBuffer, vubo);
		}

	}
	else
	{
		if(usingClipUBO)
		{
			DrawQuadClipUBO vubo = {};
			vubo.mvpMatrix = mvpMatrix;
			vubo.color = color;
			vubo.scale = vec4(size.x, size.y, 0.0, 1.0);
            vubo.userData = userData;
			vubo.clipMatrix = clipMatrix;
			mjCopyGPUMemory(cache->vulkan->vmaAllocator, vUBOBuffer, vubo);
		}
		else
		{
			DrawQuadUBO vubo = {};
			vubo.mvpMatrix = mvpMatrix;
			vubo.color = color;
			vubo.scale = vec4(size.x, size.y, 0.0, 1.0);
            vubo.userData = userData;
			mjCopyGPUMemory(cache->vulkan->vmaAllocator, vUBOBuffer, vubo);
		}
	}
}

void MJDrawQuad::setImageTeture(MJImageTexture* imageTexture_)
{
    if(imageTexture && imageTexture_ != imageTexture)
    {
		imageTexture = imageTexture_;

		if(drawable && imageTexture && imageTexture->loaded)
		{
			drawable->updateTexture(imageTexture->textureImageView, imageTexture->textureSampler, "t0");
		}
		else
		{
			needsToReloadTexture = true;
		}
    }
}
