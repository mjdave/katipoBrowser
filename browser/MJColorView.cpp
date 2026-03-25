
#include "MJColorView.h"
#include "MJCache.h"
#include "MJDrawQuad.h"
#include "TuiStringUtils.h"
#include "GCommandBuffer.h"
#include "MJDrawable.h"
#include "Camera.h"

#include "TuiScript.h"

struct vUBO {
	mat4 modelMatrix;
	vec4 color;
	vec4 size;
	vec4 animationTimer;
	vec4 userUniformA;
	vec4 userUniformB;
	mat4 clipMatrix;
};

MJColorView::MJColorView(MJView* parentView_)
:MJView(parentView_)
{
    color = vec4(1.0,1.0,1.0,1.0);
    masksEvents = true;
    shaderName = "colorView";
    shaderNameClip = shaderName + "Clip";
}

MJColorView::~MJColorView()
{
}

void MJColorView::destroyDrawables()
{
	if(drawable)
	{
		delete drawable;
		drawable = nullptr;
	}
}

void MJColorView::createDrawables(MJRenderPass renderPass, int renderTargetCompatibilityIndex)
{
	if(!drawable)
	{
		bool usingClipUBO = (clippingParent != nullptr && !disableClipping);

		drawable = new MJDrawable(cache, 
			renderPass,
			(usingClipUBO ? shaderNameClip : shaderName), 
			1, 
			DrawQuadVertex::getBindingDescriptions(), 
			DrawQuadVertex::getAttributeDescriptions(),
			renderTargetCompatibilityIndex);

		drawable->addDynamicUBO(sizeof(vUBO), "v0", VK_SHADER_STAGE_VERTEX_BIT);
        drawable->addExternalDynamicUBO(cache->getCameraBuffer(), sizeof(CameraUBO), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

		drawable->finalize();
	}
}


void MJColorView::setShaderName(std::string shaderName_)
{
    if(shaderName != shaderName_)
    {
        shaderName = shaderName_;
        destroyDrawables();
    }
}

dvec4 MJColorView::getColor() const
{
	return color;
}

void MJColorView::setColor(dvec4 color_)
{
	color = color_;
}

void MJColorView::setSize(dvec2 size_)
{
    MJView::setSize(size_);
}

void MJColorView::setShader(std::string shaderName_)
{
	if(shaderName_ != shaderName)
	{
		shaderName = shaderName_;
		shaderNameClip = shaderName;
		destroyDrawables();
	}
}


void MJColorView::renderSelf(GCommandBuffer* mainCommandBuffer)
{
	if(!drawable)
	{
		return;
	}
	Vulkan* vulkan = cache->vulkan;

	const VkCommandBuffer* commandBuffer = mainCommandBuffer->getBuffer();
	GPipeline* pipeline = drawable->pipeline;
	VkDescriptorSet descriptorSet = drawable->getDescriptorSet(0);

	vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);

	VkBuffer vertexBuffers[] = {vulkan->drawQuadVertBuffer.buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(*commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

	vkCmdDraw(*commandBuffer, 6, 1, 0, 0);
}

void MJColorView::preRender(GCommandBuffer* commandBuffer, MJRenderPass renderPass, int renderTargetCompatibilityIndex, double dt, double frameLerp, double animationTimer_)
{
	MJView::preRender(commandBuffer, renderPass, renderTargetCompatibilityIndex, dt, frameLerp, animationTimer_);
}

void MJColorView::updateUBOs(float parentAlpha, dvec3 camPos, dvec3 viewPos)
{
	MJView::updateUBOs(parentAlpha, camPos, viewPos);
	if(hidden || invalidated)
	{
		return;
	}

	if(!drawable)
	{
		return;
	}

	float combinedAlpha = parentAlpha * alpha;

	MJVMABuffer vUBOBuffer = drawable->getDynamicUBO(0, "v0");

	dmat4 mvpMatrix = combinedRenderMatrix;
	mvpMatrix = glm::scale(mvpMatrix, dvec3(1.0));
	mvpMatrix[3] = vec4(dvec3(mvpMatrix[3]), 1.0);

	vUBO vubo = {};
	vubo.modelMatrix = mvpMatrix;
	vubo.color = vec4(color.x,color.y,color.z,color.w * combinedAlpha);
	dvec2 sizeToUse = getSize() * renderScale;
	vubo.size = vec4(sizeToUse.x, sizeToUse.y, 0.0, 1.0);
	vubo.animationTimer = vec4(animationTimer, 0.0, 0.0, 0.0);
	vubo.userUniformA = shaderUniformA;
	vubo.userUniformB = shaderUniformB;

	if(clippingParent != nullptr && !disableClipping)
	{
		vubo.clipMatrix = inverse(glm::scale(clippingParent->combinedRenderMatrix, dvec3(clippingParent->size.x * renderScale, clippingParent->size.y * renderScale, 1.0))) * combinedRenderMatrix;
	}

	mjCopyGPUMemory(vulkan->vmaAllocator, vUBOBuffer, vubo);
}

std::string MJColorView::getDescription()
{
    std::string result = Tui::string_format("MJColorView %p (%.2f,%.2f) - (%.2f,%.2f,%.2f,%.2f)\nsubviews:\n", (void*)this, size.x, size.y, color.x, color.y, color.z, color.w);
    for(MJView* subView : subviews)
    {
        result = result + subView->getDescription();
    }
    return result;
}


void MJColorView::loadFromTable(TuiTable* table, bool needsToLayoutSubviews, TuiTable* rootTableOrNil)
{
    MJView::loadFromTable(table, needsToLayoutSubviews, rootTableOrNil);
    
    if(table->hasKey("color"))
    {
        stateTable->setVec4("color", table->getVec4("color"));
    }
    
    if(table->hasKey("shader"))
    {
        stateTable->setString("shader", table->getString("shader"));
    }
    else if(table->hasKey("additiveBlend"))
    {
        if(table->getBool("additiveBlend"))
        {
            setShaderName("colorViewAdditiveBlend");
        }
        else
        {
            setShaderName("colorView");
        }
    }
    if(table->hasKey("shaderUserData"))
    {
        stateTable->setVec4("shaderUserData", table->getVec4("shaderUserData"));
    }
}


void MJColorView::tableKeyChanged(const std::string& key, TuiRef* value)
{
    MJView::tableKeyChanged(key, value);
    
    if(key == "color")
    {
        setColor(stateTable->getVec4("color"));
    }
    else if(key == "shader")
    {
        setShaderName(stateTable->getString("shader"));
    }
    else if(key == "additiveBlend" && !stateTable->hasKey("shader"))
    {
        if(stateTable->getBool("additiveBlend"))
        {
            setShaderName("colorViewAdditiveBlend");
        }
        else
        {
            setShaderName("colorView");
        }
    }
    else if(key == "shaderUserData")
    {
        shaderUniformA = stateTable->getVec4("shaderUserData");
    }
}
