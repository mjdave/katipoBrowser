
#include "MJImageView.h"
#include "TuiFileUtils.h"
#include "GPipeline.h"
#include "MJImageTexture.h"
#include "MJCache.h"
#include "MJDrawQuad.h"
#include "TuiStringUtils.h"
#include "TuiScript.h"

MJImageView::MJImageView(MJView* parentView_)
:MJView(parentView_)
{
    
    imageTexture = nullptr;
    drawQuad = nullptr;
    color = vec4(1.0,1.0,1.0,1.0);
    imageSize = vec2(1.0,1.0);
    imageOffset = vec2(0.0,0.0);
    masksEvents = true;
}

MJImageView::~MJImageView()
{
}

void MJImageView::destroyDrawables()
{
	if(drawQuad)
	{
		delete drawQuad;
		drawQuad = nullptr;
	}
}

void MJImageView::createDrawables(MJRenderPass renderPass, int renderTargetCompatibilityIndex)
{
	if(!drawQuad && imageTexture && imageTexture->loaded)
	{
		 usingClipUBO = (clippingParent != nullptr && !disableClipping);

        drawQuad = new MJDrawQuad(cache, renderPass, renderTargetCompatibilityIndex, usingClipUBO, imageTexture, getSize() * renderScale, imageOffset, imageSize + imageOffset, shaderName);
        updateTexCoords();
        drawQuad->userData = shaderUserData;
	}
}

void MJImageView::setSize(dvec2 size_)
{
    MJView::setSize(size_);
	if(drawQuad)
	{
		drawQuad->setSize(getSize() * renderScale);
	}
    updateTexCoords();
}

dvec2 MJImageView::getImageSize() const
{
    return imageSize;
}

void MJImageView::setImageSize(dvec2 imageSize_)
{
    imageSize = imageSize_;
    updateTexCoords();
}

dvec2 MJImageView::getImageOffset() const
{
    return imageOffset;
}

void MJImageView::setImageOffset(dvec2 imageOffset_)
{
    imageOffset = imageOffset_;
    updateTexCoords();
}

MJImageTexture* MJImageView::getImageTexture() const
{
    return imageTexture;
}

void MJImageView::setImageTexture(MJImageTexture* imageTexture_)
{
    if(imageTexture_ != imageTexture)
    {
        imageTexture = imageTexture_;
        if(drawQuad)
        {
            drawQuad->setImageTeture(imageTexture);
        }
    }
}

void MJImageView::renderSelf(GCommandBuffer* commandBuffer)
{
    if(!getHidden() && drawQuad && !invalidated)
    {
        drawQuad->render(commandBuffer);
    }
}


void MJImageView::updateUBOs(float parentAlpha, dvec3 camPos, dvec3 viewPos)
{
	MJView::updateUBOs(parentAlpha, camPos, viewPos);
	if(hidden || invalidated)
	{
		return;
	}

	if(!getHidden() && drawQuad && !invalidated)
	{
		float combinedAlpha = parentAlpha * alpha;
		dmat4 mvpMatrix = combinedRenderMatrix;
		mvpMatrix = glm::scale(mvpMatrix, dvec3(1.0));
		mvpMatrix[3] = vec4(dvec3(mvpMatrix[3]), 1.0);

		dmat4 clipMatrix = dmat4(1.0);
		if(clippingParent != nullptr && !disableClipping)
		{
			clipMatrix = inverse(glm::scale(clippingParent->combinedRenderMatrix, dvec3(clippingParent->size.x * renderScale, clippingParent->size.y * renderScale, 1.0))) * combinedRenderMatrix;
		}

		drawQuad->updateUBOs(mvpMatrix, clipMatrix, vec4(color.r, color.g, color.b, color.a * combinedAlpha));
	}
}



std::string MJImageView::getShaderName() const
{
    return shaderName;
}

void MJImageView::setShaderName(std::string shaderName_)
{
    if(shaderName != shaderName_)
    {
        shaderName = shaderName_;
        destroyDrawables();
    }
}

void MJImageView::updateTexCoords()
{
    if(drawQuad)
    {
        if(cropImage && imageTexture)
        {
            float imageRatio = imageTexture->size.x / imageTexture->size.y;
            float viewRatio = drawQuad->size.x / drawQuad->size.y;
            
            dvec2 newImageSizeFraction = vec2(1.0,1.0);
            
            if(imageRatio > viewRatio)
            {
                newImageSizeFraction.x = viewRatio / imageRatio;
            }
            else
            {
                newImageSizeFraction.y = imageRatio / viewRatio;
            }
            
            dvec2 croppedOffset = (dvec2(1.0) - newImageSizeFraction) * 0.5;
            
            drawQuad->setTexCoords(imageOffset + croppedOffset, newImageSizeFraction + imageOffset + croppedOffset);
        }
        else
        {
            drawQuad->setTexCoords(imageOffset, imageSize + imageOffset);
        }
    }
}

void MJImageView::setCropImage(bool cropImage_) //todo recalc this if cropImage and a new image is set
{
    if(cropImage != cropImage_)
    {
        cropImage = cropImage_;
        updateTexCoords();
    }
}


std::string MJImageView::getDescription()
{
    std::string result = Tui::string_format("MJImageView %p (%.2f,%.2f) - (%s)\nsubviews:\n", (void*)this, size.x, size.y, (imageTexture ? imageTexture->pathname.c_str() : "null"));
    for(MJView* subView : subviews)
    {
        result = result + subView->getDescription();
    }
    return result;
}


void MJImageView::loadFromTable(TuiTable* table, bool isRoot)
{
    MJView::loadFromTable(table, isRoot);
    
    if(table->hasKey("color"))
    {
        stateTable->setVec4("color", table->getVec4("color"));
    }
    if(table->hasKey("shader"))
    {
        stateTable->set("shader", table->objectsByStringKey["shader"]);
    }
    if(table->hasKey("crop"))
    {
        stateTable->set("crop", table->objectsByStringKey["crop"]);
    }
    if(table->hasKey("path"))
    {
        stateTable->set("path", table->objectsByStringKey["path"]);
    }
    if(table->hasKey("shaderUserData"))
    {
        stateTable->set("shaderUserData", table->objectsByStringKey["shaderUserData"]);
    }
}


void MJImageView::tableKeyChanged(const std::string& key, TuiRef* value)
{
    MJView::tableKeyChanged(key, value);
    
    if(key == "shader")
    {
        setShaderName(stateTable->getString("shader"));
    }
    else if(key == "color")
    {
        color = stateTable->getVec4("color");
    }
    else if(key == "path")
    {
        setImageTexture(cache->getTexture(stateTable->getString("path"), rootTable, false, false, true));
    }
    else if(key == "crop")
    {
        setCropImage(stateTable->getBool("crop"));
    }
    else if(key == "shaderUserData")
    {
        shaderUserData = stateTable->getVec4("shaderUserData");
        if(drawQuad)
        {
            drawQuad->userData = shaderUserData;
        }
    }
    
}
