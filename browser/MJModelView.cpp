//
//  MJTextView.cpp
//  World
//
//  Created by David Frampton on 4/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#include "MJModelView.h"
#include "TuiScript.h"
#include "KatipoUtilities.h"
#include "MJCache.h"
#include "MJDrawQuad.h"
#include "MJRenderTarget.h"
#include "UILightingManager.h"
#include "MJDrawable.h"
#include "MJRawImageTexture.h"
#include "GCommandBuffer.h"
#include "MathUtils.h"
#include "Camera.h"
#include "ObjectVert.h"
#include "TuiScript.h"


VULKAN_PACK(
    struct vUBO {
    mat4 mv_matrix;
    mat4 normal_matrix;
    vec4 color;
	vec4 camPos;
	vec4 translation;
});

VULKAN_PACK(
	struct vUBOClip {
	mat4 mv_matrix;
	mat4 normal_matrix;
	mat4 clip_matrix;
	vec4 color;
	vec4 camPos;
	vec4 translation;
});

VULKAN_PACK(
	struct fUBO {
	vec4 sunPosAnimationTimer;
});

MJModelView::MJModelView(MJView* parentView_)
:MJView(parentView_)
{
    color = vec4(1.0,1.0,1.0,1.0);
    masksEvents = true;
    depthTestEnabled = true;
}


MJModelView::~MJModelView()
{
}

void MJModelView::removeModel()
{
	model = nullptr;
}

/*std::string MJModelView::getHash(int modelIndex, std::map<int, u8vec4>& materialRemaps)
{

}*/


void MJModelView::setModel(const std::string& modelPath_, std::map<std::string, std::string>& remaps, std::string defaultMaterial_)
{
    modelPath = modelPath_;
    model = cache->getModel(modelPath);
    
	defaultMaterial = defaultMaterial_;
    needsToLoadNewModel = true;
    materialRemaps = remaps;
}

void MJModelView::setModel(const std::string& modelPath_)
{
    modelPath = modelPath_;
    model = cache->getModel(modelPath);
    
    defaultMaterial = "";
    needsToLoadNewModel = true;
}

#define GameObjectViewFOVY (60.0 * 0.0174533)



void MJModelView::destroyDrawables()
{
	if(mainDrawable)
	{
		delete mainDrawable;
		mainDrawable = nullptr;
	}

	if(decalDrawable)
	{
		delete decalDrawable;
		decalDrawable = nullptr;
	}

	if(mainXRayDrawable)
	{
		delete mainXRayDrawable;
		mainXRayDrawable = nullptr;
	}
}

void MJModelView::createDrawables(MJRenderPass renderPass, int renderTargetCompatibilityIndex)
{
	if(model)
    {
        if(!model->hasBones)
        {
            if(!mainDrawable)
            {
				usingClipUBO = (clippingParent != nullptr && !disableClipping);

				MJImageTextureOptions textureOptions = {};
				textureOptions.mipmap = true;
				textureOptions.magFilter = VK_FILTER_LINEAR;
				textureOptions.minFilter = VK_FILTER_LINEAR;
				textureOptions.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				MJImageTexture* objectTexture = cache->getTextureWithOptions("img/objectTexturesNormal.png", textureOptions, "img/objectTextures.png");

                mainDrawable = new MJDrawable(cache, 
					renderPass,
                    (radialMask ? (depthTestEnabled ? (usingClipUBO ? "objectUIRadialMaskClip" :  "objectUIRadialMask") : "objectUIRadialMaskNoDepth") : (depthTestEnabled ? (usingClipUBO ? "objectUIClip" :  "objectUI") : "objectUINoDepth" )), 
                    1, 
                    ObjectVert::getBindingDescriptions(), 
                    ObjectVert::getAttributeDescriptions(),
					renderTargetCompatibilityIndex);

                mainDrawable->addDynamicUBO((usingClipUBO ? sizeof(vUBOClip) :  sizeof(vUBO)), "v0", VK_SHADER_STAGE_VERTEX_BIT);
				mainDrawable->addExternalDynamicUBO(cache->getCameraBuffer(), sizeof(CameraUBO), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
                mainDrawable->addTexture(cache->uiLightingManager->textureImageView, cache->uiLightingManager->textureSampler, "t0");
                mainDrawable->addTexture(cache->brdfMap->textureImageView, cache->brdfMap->textureSampler, "t0");
				mainDrawable->addTexture(objectTexture->textureImageView, objectTexture->textureSampler, "t1");

                mainDrawable->finalize();


				mainDrawableUBOBuffers = mainDrawable->getDynamicUBOs(0, "v0");
            }

            if(!decalDrawable && !model->edgeDecals.empty())
            {
                MJImageTexture* decalImageTexture = cache->getTexture("img/objectDecals.png", false, false, true);

                decalDrawable = new MJDrawable(cache, 
					renderPass,
                    "objectToTextureDecal", 
                    1, 
                    ObjectDecalVert::getBindingDescriptions(), 
                    ObjectDecalVert::getAttributeDescriptions(),
					renderTargetCompatibilityIndex);

                decalDrawable->addDynamicUBO(sizeof(vUBO), "v0", VK_SHADER_STAGE_VERTEX_BIT);
				decalDrawable->addExternalDynamicUBO(cache->getCameraBuffer(), sizeof(CameraUBO), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
				decalDrawable->addDynamicUBO(sizeof(fUBO), "f0", VK_SHADER_STAGE_FRAGMENT_BIT);
                decalDrawable->addTexture(cache->uiLightingManager->textureImageView, cache->uiLightingManager->textureSampler, "t0");
                decalDrawable->addTexture(cache->brdfMap->textureImageView, cache->brdfMap->textureSampler, "t0");
                decalDrawable->addTexture(decalImageTexture->textureImageView, decalImageTexture->textureSampler, "t0");

                decalDrawable->finalize();
            }

			if(!mainXRayDrawable && renderXRay)
			{
				MJImageTextureOptions textureOptions = {};
				textureOptions.mipmap = true;
				textureOptions.magFilter = VK_FILTER_LINEAR;
				textureOptions.minFilter = VK_FILTER_LINEAR;
				textureOptions.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				MJImageTexture* objectTexture = cache->getTextureWithOptions("img/objectTexturesNormal.png", textureOptions, "img/objectTextures.png");

				mainXRayDrawable = new MJDrawable(cache, 
					renderPass,
					(radialMask ? "objectUIRadialMaskOutline" : "modelViewOutline"), 
					1, 
					ObjectVert::getBindingDescriptions(), 
					ObjectVert::getAttributeDescriptions(),
					renderTargetCompatibilityIndex);

				mainXRayDrawable->addExternalDynamicUBO(mainDrawable->dynamicUBOBuffers["v0"][0], sizeof(vUBO), VK_SHADER_STAGE_VERTEX_BIT);
				//mainXRayDrawable->addDynamicUBO(sizeof(vUBO), "v0", VK_SHADER_STAGE_VERTEX_BIT);
				mainXRayDrawable->addExternalDynamicUBO(cache->getCameraBuffer(), sizeof(CameraUBO), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
				mainXRayDrawable->addTexture(cache->uiLightingManager->textureImageView, cache->uiLightingManager->textureSampler, "t0");
				mainXRayDrawable->addTexture(cache->brdfMap->textureImageView, cache->brdfMap->textureSampler, "t0");
				mainXRayDrawable->addTexture(objectTexture->textureImageView, objectTexture->textureSampler, "t1");

				mainXRayDrawable->finalize();
			}
        }
    }
}


void MJModelView::renderSelf(GCommandBuffer* mainCommandBuffer)
{
	if(!model)
	{
		return;
	}

	const VkCommandBuffer* commandBuffer = mainCommandBuffer->getBuffer();



	if(renderXRay && mainXRayDrawable)
	{
		MJDrawable* drawable = mainXRayDrawable;
		GPipeline* pipeline = drawable->pipeline;

		VkDescriptorSet descriptorSet = drawable->getDescriptorSet(0);

		vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);

		VkBuffer vertexBuffers[] = {modelViewBuffers.vertexBuffer.buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(*commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		vkCmdDraw(*commandBuffer, modelViewBuffers.vertCount, 1, 0, 0);
	}


	MJDrawable* drawable = mainDrawable;

	if(!drawable)
	{
		return;
	}

	//dvec3 pos = combinedRenderMatrix[3];
	//MJLog("%p rendering:(%.6f %.6f, %.6f)", this, pos.x, pos.y, pos.z);

	if(modelViewBuffers.vertCount > 0)
	{
		GPipeline* pipeline = drawable->pipeline;

		VkDescriptorSet descriptorSet = drawable->getDescriptorSet(0);

		vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);

		VkBuffer vertexBuffers[] = {modelViewBuffers.vertexBuffer.buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(*commandBuffer, 0, 1, vertexBuffers, offsets);

 		vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		vkCmdDraw(*commandBuffer, modelViewBuffers.vertCount, 1, 0, 0);
	}

	if(modelViewBuffers.edgeDecalCount > 0)
	{
		MJDrawable* drawable = decalDrawable;
		GPipeline* pipeline = drawable->pipeline;

		VkDescriptorSet descriptorSet = drawable->getDescriptorSet(0);

		vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);

		VkBuffer vertexBuffers[] = {modelViewBuffers.edgeDecalBuffer.buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(*commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		vkCmdDraw(*commandBuffer, modelViewBuffers.edgeDecalCount, 1, 0, 0);
	}
}

void MJModelView::updateUBOs(float parentAlpha, dvec3 camPos, dvec3 viewPos)
{
	MJView::updateUBOs(parentAlpha, camPos, viewPos);
	
	if(hidden || invalidated)
	{
		return;
	}

	if(!model)
	{
		return;
	}


	float combinedAlpha = parentAlpha * alpha;
	Vulkan* vulkan = cache->vulkan;

	MJDrawable* drawable = mainDrawable;
	if(!drawable)
	{
		return;
	}

	const MJVMABuffer& vUBOBuffer = mainDrawableUBOBuffers->at(vulkan->primaryCommandBuffer->currentIndex);

	//dvec3 pos = combinedRenderMatrix[3];
	//MJLog("%p update UBOs:(%.6f %.6f, %.6f)", this, pos.x, pos.y, pos.z);

	dmat4 offsetMatrix = translate(combinedRenderMatrix, dvec3(size.x * 0.5 * renderScale, size.y * 0.5 * renderScale, 0.0));
	dmat4 scaledMatrix = glm::scale(offsetMatrix, dvec3(scale3D.x * renderScale, scale3D.y * renderScale, scale3D.z * renderScale));
	dmat4 normalMatrix = dmat4((dmat3(combinedRenderMatrix)));

	if(usingClipUBO)
	{
		vUBOClip vubo = {};
		vubo.mv_matrix =  scaledMatrix;
		vubo.mv_matrix = glm::scale(vubo.mv_matrix, vec3(dvec3(1.0)));
		vubo.mv_matrix[3] = vec4(dvec3(vubo.mv_matrix[3]), 1.0);

		vubo.clip_matrix = inverse(glm::scale(clippingParent->combinedRenderMatrix, dvec3(clippingParent->size.x * renderScale, clippingParent->size.y * renderScale, 1.0))) * scaledMatrix;

		vubo.normal_matrix = normalMatrix;
		vubo.color = vec4(1.0,1.0,1.0, combinedAlpha);
		vubo.camPos = vec4(camPos, animationTimer);
		vubo.translation = vubo.mv_matrix[3] + vec4(viewPos, 0.0f);
		vubo.translation.w = radialMaskFraction;
		mjCopyGPUMemory(vulkan->vmaAllocator, vUBOBuffer, vubo);
	}
	else
	{
		vUBO vubo = {};
		vubo.mv_matrix =  scaledMatrix;
		vubo.mv_matrix = glm::scale(vubo.mv_matrix, vec3(dvec3(1.0)));
		vubo.mv_matrix[3] = vec4(dvec3(vubo.mv_matrix[3]), 1.0);
		vubo.normal_matrix = normalMatrix;
		vubo.color = vec4(1.0,1.0,1.0, combinedAlpha);
		vubo.camPos = vec4(camPos, animationTimer);
		vubo.translation = vubo.mv_matrix[3] + vec4(viewPos, 0.0f);
		vubo.translation.w = radialMaskFraction;
		mjCopyGPUMemory(vulkan->vmaAllocator, vUBOBuffer, vubo);

		if(!model->edgeDecals.empty())
		{
			MJDrawable* drawable = decalDrawable;

			const MJVMABuffer& vUBOBuffer = drawable->getDynamicUBO(0, "v0");

			//vmaMapMemory(vulkan->vmaAllocator, vUBOBuffer.allocation, &mappedData);
			//memcpy(vUBOBuffer.allocInfo.pMappedData, &vubo, sizeof(vubo));
			mjCopyGPUMemory(vulkan->vmaAllocator, vUBOBuffer, vubo);
			//vmaUnmapMemory(vulkan->vmaAllocator, vUBOBuffer.allocation);
		}
	}
}

void MJModelView::preRender(GCommandBuffer* commandBuffer, MJRenderPass renderPass, int renderTargetCompatibilityIndex, double dt, double frameLerp, double animationTimer_)
{
    if(!getHidden() && (model || needsToLoadNewModel))
    {
        if(needsToLoadNewModel)
        {
            needsToLoadNewModel = false;
            
			if(model->hasBones)
			{
				MJError("MJModelView does not support boned models:%s", model->debugName.c_str());
				return;
			}


			modelViewBuffers = cache->getModelViewBuffers(commandBuffer, model, materialRemaps, defaultMaterial);
        }
    }

    MJView::preRender(commandBuffer, renderPass, renderTargetCompatibilityIndex, dt, frameLerp, animationTimer_);
}


bool MJModelView::containsPoint(dvec3 windowRayStart, dvec3 windowRayDirection)
{
	if (hidden || invalidated)
	{
		return false;
	}
	if(!model)
	{
		return false;
	}
	//bool withinBounds = MJView::containsPoint(windowRayStart, windowRayDirection); //this breaks for anything outside the norm. Might need some other optimization
	if(!useModelHitTest)// || !withinBounds)
	{
		return MJView::containsPoint(windowRayStart, windowRayDirection);
	}

	if(useCircleHitRadius) //in this case lets just use it as an optimization to discard
	{
		if(!MJView::containsPoint(windowRayStart, windowRayDirection))
		{
			return false;
		}
	}

	dmat4 offsetMatrix = translate(combinedRenderMatrix, dvec3(size.x * 0.5 * renderScale, size.y * 0.5 * renderScale, 0.0));
	dmat4 scaledMatrix = glm::scale(offsetMatrix, dvec3(scale3D.x * renderScale, scale3D.y * renderScale, scale3D.z * renderScale));

	mat4 objectMatrixInverse = inverse(scaledMatrix);

	vec3 modelSpaceStart = objectMatrixInverse * vec4(windowRayStart.x, windowRayStart.y, windowRayStart.z, 1.0);
	vec3 modelSpaceDirection = normalize(mat3(objectMatrixInverse) * vec4(windowRayDirection.x, windowRayDirection.y, windowRayDirection.z, 1.0));

	for(ModelFace& face : model->faces)
	{
		if(dot(face.faceNormal, modelSpaceDirection) < 0)
		{
			vec2 baryPos;
			float distance;
			bool intersection = intersectRayTriangle(modelSpaceStart, modelSpaceDirection, 
				face.verts[0],
				face.verts[1],
				face.verts[2], 
				baryPos, distance);
			if(intersection && distance > 0)
			{
				return true;
			}
		}
	}

	return false;
}

dvec3 MJModelView::getScale3D() const
{
	return scale3D;
}

void MJModelView::setScale3D(dvec3 scale3D_)
{
	scale3D = scale3D_;
}

void MJModelView::setUsesModelHitTest(bool useModelHitTest_)
{
	useModelHitTest = useModelHitTest_;
}

void MJModelView::setDepthTestEnabled(bool depthTestEnabled_)
{
	if(depthTestEnabled != depthTestEnabled_)
	{
		MJView::setDepthTestEnabled(depthTestEnabled_);
		destroyDrawables();
	}
}

void MJModelView::setRadialMaskFraction(double radialMaskFraction_)
{
	if(radialMaskFraction_ >= 0.99999)
	{
		if(radialMask)
		{
			radialMask = false;
			destroyDrawables();
		}
	}
	else
	{
		radialMaskFraction = radialMaskFraction_;
		if(!radialMask)
		{
			radialMask = true;
			destroyDrawables();
		}
	}
}

std::string MJModelView::getDescription()
{
    std::string result = Tui::string_format("MJModelView %p (%.2f,%.2f)\nsubviews:\n", size.x, size.y, (void*)this);
    for(MJView* subView : subviews)
    {
        result = result + subView->getDescription();
    }
    return result;
}



void MJModelView::loadFromTable(TuiTable* table, bool isRoot)
{
    MJView::loadFromTable(table, isRoot);
    
    
    if(table->hasKey("modelName"))
    {
        setModel(Katipo::getResourcePath("models/" + table->getString("modelName")));
    }
    if(table->hasKey("modelScale"))
    {
        setScale3D(table->getVec3("modelScale"));
    }
}
