
#include "MJTextView.h"
#include "TuiFileUtils.h"
#include "MJImageTexture.h"
#include "MJCache.h"
#include "TuiStringUtils.h"
#include "GCommandBuffer.h"
#include "MJDrawable.h"
#include "Camera.h"
#include "TuiScript.h"

struct UniformBufferObject {
    mat4 mvpMatrix;
    vec4 color;
};

struct vUBOClip {
	mat4 mvpMatrix;
	mat4 clipMatrix;
	vec4 color;
};

MJTextView::MJTextView(MJView* parentView_)
:MJView(parentView_)
{
    textColor = vec4(1.0f,1.0f,1.0f,1.0f);
    textAlignment = MJHorizontalAlignmentLeft;
    masksEvents = false;
    
    font = nullptr;
    wrapWidth = 0;
    
    //todo this is probably a bit slow to do all the time like this. meta tables needed
    stateTable->setFunction("getRectForCharAtIndex", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* indexRef = args->arrayObjects[0];
            if(indexRef->type() == Tui_ref_type_NUMBER)
            {
                dvec4 rect = getRectForCharAtIndex(((TuiNumber*)indexRef)->value);
                return new TuiVec4(rect);
            }
        }
        return nullptr;
    });
    
    stateTable->setFunction("resetVerticalCursorMovementAnchors", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        resetVerticalCursorMovementAnchors();
        return TUI_NIL;
    });
    
    stateTable->setFunction("getCursorOffsetForVerticalCursorMovement", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 2)
        {
            TuiRef* currentCursorOffsetRef = args->arrayObjects[0];
            TuiRef* verticalOffsetRef = args->arrayObjects[1];
            if(currentCursorOffsetRef->type() == Tui_ref_type_NUMBER && verticalOffsetRef->type() == Tui_ref_type_NUMBER)
            {
                int result = getCursorOffsetForVerticalCursorMovement(((TuiNumber*)currentCursorOffsetRef)->value, ((TuiNumber*)verticalOffsetRef)->value);
                return new TuiNumber(result);
            }
            else
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "Incorrect argument type");
            }
            
        }
        return TUI_NIL;
    });
    

}



MJTextView::~MJTextView()
{
	if(hasRenderData)
	{
		Vulkan* vulkan = cache->vulkan;
		vulkan->destroySingleBuffer(vertexBuffer);
	}
}


void MJTextView::setDepthTestEnabled(bool depthTestEnabled_)
{
	if(depthTestEnabled != depthTestEnabled_)
	{
        MJView::setDepthTestEnabled(depthTestEnabled_);
		destroyDrawables();
	}
}


void MJTextView::destroyDrawables()
{
	if(drawable)
	{
		delete drawable;
		drawable = nullptr;
	}

	if(mainXRayDrawable)
	{
		delete mainXRayDrawable;
		mainXRayDrawable = nullptr;
	}
}


void MJTextView::createDrawables(MJRenderPass renderPass, int renderTargetCompatibilityIndex)
{
	if(!drawable && font && !text.empty())
	{
		usingClipUBO = (clippingParent != nullptr && !disableClipping);

		drawable = new MJDrawable(cache, 
			renderPass,
			(usingClipUBO ? "texturedVertColoredClip" : (depthTestEnabled ? "texturedVertColoredDepthTest" : (additiveBlend ? "texturedVertColoredAdditiveBlend" : "texturedVertColored"))), 
			1,
			FontVert::getBindingDescriptions(), 
			FontVert::getAttributeDescriptions(),
			renderTargetCompatibilityIndex);

		drawable->addDynamicUBO((usingClipUBO ? sizeof(vUBOClip) :  sizeof(UniformBufferObject)), "v0", VK_SHADER_STAGE_VERTEX_BIT);
		drawable->addExternalDynamicUBO(cache->getCameraBuffer(), sizeof(CameraUBO), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		drawable->addTexture(font->texture->textureImageView, font->texture->textureSampler, "t0");

		drawable->finalize();



		if(!mainXRayDrawable && renderXRay && !usingClipUBO)
		{
			mainXRayDrawable = new MJDrawable(cache, 
				renderPass,
				"texturedVertColoredXRay", 
				1, 
				FontVert::getBindingDescriptions(), 
				FontVert::getAttributeDescriptions(),
				renderTargetCompatibilityIndex);

			mainXRayDrawable->addExternalDynamicUBO(drawable->dynamicUBOBuffers["v0"][0], sizeof(UniformBufferObject), VK_SHADER_STAGE_VERTEX_BIT);
			mainXRayDrawable->addExternalDynamicUBO(cache->getCameraBuffer(), sizeof(CameraUBO), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
			mainXRayDrawable->addTexture(font->texture->textureImageView, font->texture->textureSampler, "t0");

			mainXRayDrawable->finalize();
		}
	}
}


void MJTextView::updateDimensions()
{
	if(font && textString.size() > 0)
	{
		double scaleToUse = renderScale;
		double textRenderScaleToUse = textRenderScale;
		irect enclosingRect = font->calculateEnclosingRect(text, textAlignment, wrapWidth * scaleToUse, 1.0 / textRenderScaleToUse);
		setSizeInternal((dvec2(enclosingRect.size) * fontGeometryScale) / scaleToUse);

		textRenderOffset = dvec2(-enclosingRect.origin.x, -enclosingRect.origin.y) * fontGeometryScale;

		//bufferNeedsUpdating = true;
	}
}

void MJTextView::updateBuffer(GCommandBuffer* commandBuffer)
{
	Vulkan* vulkan = cache->vulkan;
	/*if(hasRenderData)
	{
		vulkan->destroySingleBuffer(vertexBuffer);
	}
	hasRenderData = false;*/
    
    if(!nextVertices.empty())
    {
        vulkan->destroySingleBuffer(nextVertexBuffer);
    }
    nextVertices.clear();

    if(font && !text.empty())
    {
        irect enclosingRect;
		double scaleToUse = renderScale;
        nextVertices = font->print(text, textAlignment, wrapWidth * scaleToUse, &enclosingRect, 1.0 / textRenderScale).fontVerts;
        
        setSizeInternal((dvec2(enclosingRect.size) * fontGeometryScale) / scaleToUse);
        textRenderOffset = dvec2(-enclosingRect.origin.x, -enclosingRect.origin.y) * fontGeometryScale;

		if(!nextVertices.empty())
		{
			VkDeviceSize vertexBufferSize = sizeof(nextVertices[0]) * nextVertices.size();

			vulkan->setupStaticBuffer(*commandBuffer->getBuffer(), &nextVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize, nextVertices.data(), "MJTextView");

			//hasRenderData = true;
		}
    }
    else
    {
        if(hasRenderData)
        {
            vulkan->destroySingleBuffer(vertexBuffer);
        }
        hasRenderData = false;
        setSizeInternal(dvec2(0.0,0.0));
    }
}

void MJTextView::setText(std::string text_)
{
    if(textString != text_ || text.size() > 1 || (text.size() == 1 && !glm::all(epsilonEqual(text[0].color, vec4(1,1,1,1), 0.01f))))
    {
        textString = "";
        text.clear();
		if(text_.size() > 0)
		{
			addColoredText(text_, vec4(1,1,1,1));
		}
		else
		{
			textRenderOffset = dvec2(0.0,0.0);
		}
		bufferNeedsUpdating = true;
    }
}

void MJTextView::setSizeInternal(dvec2 size_)
{
    size = size_;
    updateMatrix();
    
    for(MJView* subView : subviews)
    {
        if(subView->parentSizeChangedFunction)
        {
            TuiRef* inRef = new TuiVec2(size);
            TuiRef* sizeRef = subView->parentSizeChangedFunction->call("parentSizeChangedFunction", inRef);
            subView->stateTable->setVec2("size", ((TuiVec2*)sizeRef)->value);
            inRef->release();
            sizeRef->release();
        }
    }
}

void MJTextView::setSize(dvec2 size_) //this is intended to be a max size, but constraining by y isn't implemented yet, so it's just a word wrap convenience function
{
    if(!approxEqualVec2(size, size_))
    {
        setWrapWidth(size_.x);
    }
}

void MJTextView::addColoredText(std::string incomingString, dvec4 color)
{
	if(incomingString.size() > 0)
	{
		int textIndex = text.size();
		text.resize(textIndex + 1);
		text[textIndex].color = color;
		text[textIndex].text = incomingString;

		textString = textString + incomingString;
        
		//updateDimensions();

		bufferNeedsUpdating = true;
	}
}

std::string MJTextView::getText() const
{
    return textString;
}

void MJTextView::setFontNameAndSize(FontNameAndSize fontNameAndSize_)
{
    /*if(font != nullptr)
    {
        MJLog("changing the FontNameAndSize after creation is not supported");
        return;
    }*/
    textRenderScale = 1.0;

    fontNameAndSize = fontNameAndSize_;

	double scaleToUse = renderScale;
    MJFont* newFont = cache->getFont(fontNameAndSize_.name, fontNameAndSize_.size * scaleToUse, &textRenderScale);

    if(newFont != font)
    {
        font = newFont;
        updateDimensions();
        bufferNeedsUpdating = true;
    }

	if(!newFont)
	{
        MJWarn("Failed to load font:%s size:%d", fontNameAndSize_.name.c_str(), (int)fontNameAndSize_.size);
	}
}

FontNameAndSize MJTextView::getFontNameAndSize() const
{
    return fontNameAndSize;
}

void MJTextView::setTextColor(dvec4 textColor_)
{
    textColor = textColor_;
}

dvec4 MJTextView::getTextColor() const
{
    return textColor;
}

void MJTextView::setTextAlignment(int textAlignment_)
{
    if(textAlignment_ != textAlignment)
    {
        textAlignment = textAlignment_;
        bufferNeedsUpdating = true;
    }
}

int MJTextView::getTextAlignment() const
{
    return textAlignment;
}

void MJTextView::setWrapWidth(int wrapWidth_)
{
    if(wrapWidth != wrapWidth_)
    {
        wrapWidth = wrapWidth_;
        bufferNeedsUpdating = true;
    }
}

int MJTextView::getWrapWidth() const
{
    return wrapWidth;
}

void MJTextView::preRender(GCommandBuffer* commandBuffer, MJRenderPass renderPass, int renderTargetCompatibilityIndex, double dt, double frameLerp, double animationTimer_)
{
    /*
     Vulkan* vulkan = cache->vulkan;
     if(hasRenderData)
     {
         vulkan->destroySingleBuffer(vertexBuffer);
     }
     hasRenderData = false;
     
     if(!nextVertices.empty())
     {
         vulkan->destroySingleBuffer(nextVertexBuffer);
     }
     nextVertices.clear();

     if(font && !text.empty())
     {
         irect enclosingRect;
         double scaleToUse = renderScale;
         nextVertices = font->print(text, textAlignment, wrapWidth * scaleToUse, &enclosingRect, 1.0 / textRenderScale).fontVerts;

         if(!nextVertices.empty())
         {
             VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

             vulkan->setupStaticBuffer(*commandBuffer->getBuffer(), &nextVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize, nextVertices.data(), "MJTextView");

             //hasRenderData = true;
         }
     }
     */
    
    
    if(!nextVertices.empty())
    {
        Vulkan* vulkan = cache->vulkan;
        
        if(hasRenderData)
        {
            vulkan->destroySingleBuffer(vertexBuffer);
        }
        vertexBuffer = nextVertexBuffer;
        vertices = nextVertices;
        nextVertices.clear();
        hasRenderData = true;
    }
    
    if(bufferNeedsUpdating && !getHidden() && font)
    {
        updateBuffer(commandBuffer);
        bufferNeedsUpdating = false;
    }
    
    MJView::preRender(commandBuffer, renderPass, renderTargetCompatibilityIndex, dt, frameLerp, animationTimer_);
    
    
    //todo don't do this every frame
    /*if(stateTable)
    {
        if(stateTable->hasKey("text"))
        {
            setText(stateTable->objectsByStringKey["text"]->getStringValue());
        }
        
        if(stateTable->hasKey("size"))
        {
            TuiRef* ref = stateTable->objectsByStringKey["size"];
            switch (ref->type()) {
                case Tui_ref_type_VEC2:
                    setSize(((TuiVec2*)ref)->value);
                    break;
                default:
                    MJError("Expected vec2");
                    break;
            }
        }
    }*/
}

void MJTextView::renderSelf(GCommandBuffer* commandBuffer)
{
    if(drawable && hasRenderData)
    {
		if(renderXRay && mainXRayDrawable)
		{
			MJDrawable* drawableToUse = mainXRayDrawable;

			GPipeline* pipeline = drawableToUse->pipeline;
			VkDescriptorSet descriptorSet = drawableToUse->getDescriptorSet(0);

			vkCmdBindPipeline(*(commandBuffer->getBuffer()), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);

			VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(*(commandBuffer->getBuffer()), 0, 1, vertexBuffers, offsets);

			vkCmdBindDescriptorSets(*(commandBuffer->getBuffer()), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

			vkCmdDraw(*commandBuffer->getBuffer(), vertices.size(), 1, 0, 0);
		}

		GPipeline* pipeline = drawable->pipeline;
		VkDescriptorSet descriptorSet = drawable->getDescriptorSet(0);

        vkCmdBindPipeline(*(commandBuffer->getBuffer()), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkPipeline);

        VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(*(commandBuffer->getBuffer()), 0, 1, vertexBuffers, offsets);

		vkCmdBindDescriptorSets(*(commandBuffer->getBuffer()), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        vkCmdDraw(*commandBuffer->getBuffer(), vertices.size(), 1, 0, 0);
    }
}


void MJTextView::updateUBOs(float parentAlpha, dvec3 camPos, dvec3 viewPos)
{
	MJView::updateUBOs(parentAlpha, camPos, viewPos);
	if(hidden || invalidated)
	{
		return;
	}

	if(drawable && hasRenderData)
	{
		float combinedAlpha = parentAlpha * alpha;
		vec4 color = vec4(textColor.r, textColor.g, textColor.b, textColor.a * combinedAlpha);

		dmat4 offsetMatrix = translate(combinedRenderMatrix, dvec3(textRenderOffset.x, textRenderOffset.y, 0.0));

		if(!isWorldView && approxEqual(fontGeometryScale, 1.0))
		{
			offsetMatrix[3][0] = offsetMatrix[3][0] - fract(offsetMatrix[3][0]);
			offsetMatrix[3][1] = offsetMatrix[3][1] - fract(offsetMatrix[3][1]);
			offsetMatrix[3][2] = offsetMatrix[3][2] - fract(offsetMatrix[3][2]);
		}

		dmat4 mvpMatrix = offsetMatrix;

		//Vulkan* vulkan = cache->vulkan;

		MJVMABuffer vUBOBuffer = drawable->getDynamicUBO(0, "v0");

		if(usingClipUBO)
		{
			vUBOClip vubo = {};
			vubo.mvpMatrix = mvpMatrix;
			vubo.mvpMatrix = glm::scale(vubo.mvpMatrix, vec3(dvec3(fontGeometryScale)));
			vubo.mvpMatrix[3] = vec4(dvec3(vubo.mvpMatrix[3]), 1.0);
			vubo.clipMatrix = inverse(glm::scale(clippingParent->combinedRenderMatrix, dvec3(clippingParent->size.x * renderScale, clippingParent->size.y * renderScale, 1.0))) * mvpMatrix;
			vubo.color = color;

			mjCopyGPUMemory(vulkan->vmaAllocator, vUBOBuffer, vubo);
		}
		else
		{
			UniformBufferObject vubo = {};
			vubo.mvpMatrix = mvpMatrix;
			vubo.mvpMatrix = glm::scale(vubo.mvpMatrix, vec3(dvec3(fontGeometryScale)));
			vubo.mvpMatrix[3] = vec4(dvec3(vubo.mvpMatrix[3]), 1.0);
			vubo.color = color;

			mjCopyGPUMemory(vulkan->vmaAllocator, vUBOBuffer, vubo);
		}
	}
}



void MJTextView::setFontGeometryScale(double fontGeometryScale_)
{
	fontGeometryScale = fontGeometryScale_;
	updateDimensions();
}

double MJTextView::getFontGeometryScale() const
{
	return fontGeometryScale;
}


dvec4 MJTextView::getRectForCharAtIndex(int charIndex)
{
	if(font && !text.empty())
	{
		int charIndexToUse = charIndex;
		if(charIndex < 0)
		{
			charIndexToUse = textString.size() + charIndex;
		}
        
        bool returnStartPos = false;
        if(charIndexToUse < 0)
        {
            returnStartPos = true;
        }
        
        charIndexToUse = clamp(charIndexToUse, 0, (int)textString.size() - 1);
		double scaleToUse = renderScale;
		double textRenderScaleToUse = textRenderScale;
		irect charRect = font->calculateRectOfCharAtIndex(text, charIndexToUse, textAlignment, wrapWidth * scaleToUse, 1.0 / textRenderScaleToUse);
        
        if(returnStartPos)
        {
            return dvec4(charRect.origin.x, charRect.origin.y, 0, charRect.size.y) / scaleToUse;
        }

		return dvec4(charRect.origin.x, charRect.origin.y, charRect.size.x, charRect.size.y) / scaleToUse;

	}
	return dvec4(0.0,0.0,0.0,0.0);
}

int MJTextView::getCharIndexForPos(dvec2 pos)
{
    if(font && !text.empty())
    {
        double scaleToUse = renderScale;
        double textRenderScaleToUse = textRenderScale;
        
        //MJLog("textRenderOffset:(%.1f,%.1f)", textRenderOffset.x, textRenderOffset.y);
        
        return font->calculateIndexOfCharAtPos(text,
                                               dvec2(pos.x, pos.y - size.y) * scaleToUse,
                                               textAlignment,
                                               wrapWidth * scaleToUse,
                                               1.0 / textRenderScaleToUse);

    }
    return -1;
}


void MJTextView::setAdditiveBlend(bool additiveBlend_)
{
    if(additiveBlend != additiveBlend_)
    {
        additiveBlend = additiveBlend_;
        destroyDrawables();
    }
}

bool MJTextView::getAdditiveBlend() const
{
    return additiveBlend;
}

std::string MJTextView::getDescription()
{
    std::string result = Tui::string_format("MJTextView %p (%.2f,%.2f) - (%s)\nsubviews:\n", (void*)this, size.x, size.y, textString.c_str());
    for(MJView* subView : subviews)
    {
        result = result + subView->getDescription();
    }
    return result;
}

void MJTextView::resetVerticalCursorMovementAnchors()
{
    cursorVerticalMovementAnchors.clear();
}


int MJTextView::getCursorOffsetForVerticalCursorMovement(int currentCursorOffset, int verticalOffset)
{
    if(font && !text.empty())
    {
        double scaleToUse = renderScale;
        int indexToUse = currentCursorOffset;
        if(indexToUse == 0)
        {
            indexToUse = textString.size() -1;
        }
        dvec4 rect = getRectForCharAtIndex(indexToUse);
        
        double prevCharY = size.y + rect.y - rect.w * 0.5;
        int prevLineIndex = prevCharY/(((double)font->lineHeight) / scaleToUse);
        
        if(cursorVerticalMovementAnchors.count(prevLineIndex) == 0)
        {
            cursorVerticalMovementAnchors[prevLineIndex] = currentCursorOffset;
        }
        
        double charY = prevCharY + (((double)font->lineHeight) / scaleToUse) * verticalOffset;
        int lineIndex = charY/(((double)font->lineHeight) / scaleToUse);
        if(cursorVerticalMovementAnchors.count(lineIndex) != 0)
        {
            return cursorVerticalMovementAnchors[lineIndex];
        }
        
        int charIndex = getCharIndexForPos(dvec2(rect.x, charY));
        if(charIndex >= 0)
        {
            charIndex = charIndex - textString.size();
        }
        cursorVerticalMovementAnchors[lineIndex] = charIndex;
        return charIndex;
    }
    
    return currentCursorOffset;
}


void MJTextView::loadFromTable(TuiTable* table, bool isRoot)
{
    MJView::loadFromTable(table, isRoot);
    
    
    
    if(table->hasKey("font"))
    {
        stateTable->setString("font", table->getString("font"));
    }
    
    if(table->hasKey("fontSize"))
    {
        stateTable->setDouble("fontSize", table->getDouble("fontSize"));
    }
    
    if(table->hasKey("color"))
    {
        stateTable->setVec4("color", table->getVec4("color"));
    }
    
    if(table->hasKey("additiveBlend"))
    {
        stateTable->setBool("additiveBlend", table->getBool("additiveBlend"));
    }
    
    if(table->hasKey("textAlignment"))
    {
        stateTable->setString("textAlignment", table->getString("textAlignment"));
    }
    
    if(table->hasKey("wrapWidth"))
    {
        stateTable->setDouble("wrapWidth", table->getDouble("wrapWidth"));
    }
    
    if(table->hasKey("text"))
    {
        stateTable->set("text", table->objectsByStringKey["text"]);
    }
}


void MJTextView::tableKeyChanged(const std::string& key, TuiRef* value)
{
    MJView::tableKeyChanged(key, value);
    
    if(key == "text")
    {
        setText(stateTable->getString("text"));
    }
    else if(key == "color")
    {
        setTextColor(stateTable->getVec4("color"));
    }
    else if(key == "font" || key == "fontSize")
    {
        FontNameAndSize newNameAndSize;
        newNameAndSize.size = 18;
        newNameAndSize.name = stateTable->getString("font");
        if(stateTable->hasKey("fontSize"))
        {
            newNameAndSize.size = stateTable->getDouble("fontSize");
        }
        setFontNameAndSize(newNameAndSize);
    }
    else if(key == "additiveBlend")
    {
        setAdditiveBlend(stateTable->getBool("additiveBlend"));
    }
    else if(key == "textAlignment")
    {
        std::string alignment = stateTable->getString("textAlignment");
        
        if(alignment == "left")
        {
            setTextAlignment(MJHorizontalAlignmentLeft);
        }
        else if(alignment == "center")
        {
            setTextAlignment(MJHorizontalAlignmentCenter);
        }
        else if(alignment == "right")
        {
            setTextAlignment(MJHorizontalAlignmentRight);
        }
    }
    else if(key == "wrapWidth")
    {
        setWrapWidth(stateTable->getDouble("wrapWidth"));
    }
}
