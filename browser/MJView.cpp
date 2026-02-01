//
//  MJView.cpp
//  World
//
//  Created by David Frampton on 5/10/17.
//  Copyright © 2017 Majic Jungle. All rights reserved.
//

#include "MJView.h"
#include "MJLog.h"
#include "TuiStringUtils.h"
#include "MJCache.h"
#include "MJRenderTarget.h"
#include "MJDrawQuad.h"
#include "GCommandBuffer.h"
#include "TuiScript.h"
#include "MainController.h"

#include "MJColorView.h"
#include "MJTextView.h"
#include "MJImageView.h"

MJView* MJView::loadUnknownViewFromTable(TuiTable* subViewTable, MJView* parentView, bool isRoot) //static
{
    const std::string& viewTypeString = subViewTable->getString("type");
    if(viewTypeString == "color")
    {
        MJColorView* subView = new MJColorView(parentView);
        subView->loadFromTable(subViewTable, isRoot);
        return subView;
    }
    else if(viewTypeString == "text")
    {
        MJTextView* subView = new MJTextView(parentView);
        subView->loadFromTable(subViewTable, isRoot);
        return subView;
    }
    else if(viewTypeString == "image")
    {
        MJImageView* subView = new MJImageView(parentView);
        subView->loadFromTable(subViewTable, isRoot);
        return subView;
    }
    else if(viewTypeString == "view")
    {
        MJView* subView = new MJView(parentView);
        subView->loadFromTable(subViewTable, isRoot);
        return subView;
    }
    else
    {
        MJError("Unknown or missing view type. Found:%s", viewTypeString.c_str());
    }
    return nullptr;
}

void MJView::initInternals()
{
    invalidated = false;
    hidden = false;
    
    origin = dvec2(0.0f, 0.0f);
    size = dvec2(-100.0f,-100.0f);
    scale = 1.0;
    baseOffset = dvec3(0.0,0.0,0.0);
    additionalOffset = dvec3(0.0,0.0,0.0);
    alpha = 1.0f;
    relativeView = nullptr;
    relativePosition = MJViewPosition();
    parentView = nullptr;
    renderScale = 1.0f;
    
    mouseInside = false;
    for(int i = 0; i < MAX_MOUSE_BUTTONS; i++)
    {
        mouseDownWasInside[i] = false;
        sendClickEventOnMouseUpInside[i] = false;
        receivedMouseDown[i] = false;
    }
    
    stateTable = new TuiTable(nullptr);
    stateTable->onSet = [this](TuiRef* table, const std::string& key, TuiRef* value) {
        tableKeyChanged(key, value);
    };
    stateTable->setUserData("_view", this);
    
    stateTable->setVec3("pos", baseOffset);
    stateTable->setVec2("size", size);
    
    //todo this is probably a bit slow to do all the time like this. meta tables needed
    stateTable->setFunction("addView", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* viewTableRef = args->arrayObjects[0];
            if(viewTableRef->type() == Tui_ref_type_TABLE)
            {
                MJView* result = MJView::loadUnknownViewFromTable((TuiTable*)viewTableRef, this, true);
                if(result)
                {
                    return result->stateTable->retain();
                }
            }
        }
        return nullptr;
    });
    stateTable->setFunction("removeView", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* viewTableRef = args->arrayObjects[0];
            if(viewTableRef->type() == Tui_ref_type_TABLE)
            {
                MJView* viewToRemove = (MJView*)((TuiTable*)viewTableRef)->getUserData("_view");
                if(!viewToRemove)
                {
                    MJError("no _view ptr found");
                    return nullptr;
                }
                removeSubview(viewToRemove);
            }
        }
        return nullptr;
    });
    
    stateTable->setFunction("getView", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* viewNameRef = args->arrayObjects[0];
            if(viewNameRef->type() == Tui_ref_type_STRING)
            {
                MJView* subView = getSubViewWithID(((TuiString*)viewNameRef)->value);
                if(!subView)
                {
                    MJError("no subView named:%s", ((TuiString*)viewNameRef)->value.c_str());
                }
                return subView->stateTable->retain();
            }
        }
        return nullptr;
    });
    
    stateTable->setFunction("locationRelativeToView", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 2)
        {
            TuiRef* locationRef = args->arrayObjects[0];
            TuiRef* viewTableRef = args->arrayObjects[1];
            
            if(locationRef->type() == Tui_ref_type_VEC2 && viewTableRef->type() == Tui_ref_type_TABLE)
            {
                MJView* relativeToView = (MJView*)((TuiTable*)viewTableRef)->getUserData("_view");
                
                dvec2 resultVec = locationRelativeToView(((TuiVec2*)locationRef)->value, relativeToView);
                
                return new TuiVec2(resultVec);
            }
            else
            {
                MJError("bad args");
            }
        }
        return nullptr;
    });
    
    stateTable->setFunction("setClipChildren", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* boolRef = args->arrayObjects[0];
            
            if(boolRef->type() == Tui_ref_type_BOOL)
            {
                setClipChildren(((TuiBool*)boolRef)->value);
            }
            else
            {
                MJError("bad arg");
            }
        }
        return nullptr;
    });
    
    
}


MJView::MJView(WindowInfo* windowInfo_, MJCache* cache_)
{
    isTopLevel = true;
    windowInfo = windowInfo_;
    cache = cache_;
    viewsByID = new std::map<std::string, MJView*>();
    initInternals();
}

MJView::MJView(MJView* parentView_)
{
    windowInfo = parentView_->windowInfo;
    cache = parentView_->cache;
    viewsByID = parentView_->viewsByID;
    initInternals();
    parentView_->addSubview(this);
}

MJView::~MJView()
{
    //MJLog("MJView %p delete", this);
    if(!invalidated)
    {
        if(parentView)
        {
            parentView->removeSubview(this);
        }
        else
        {
            invalidate();
        }
    }
    
    cleanupRemovedSubviews();
    
    stateTable->release();
    
    if(updateFunction) {updateFunction->release();}

    if(parentSizeChangedFunction) {parentSizeChangedFunction->release();}

    if(hoverStartFunction) {hoverStartFunction->release();}
    if(hoverMovedFunction) {hoverMovedFunction->release();}
    if(hoverEndFunction) {hoverEndFunction->release();}
    if(hiddenStateChangedFunction) {hiddenStateChangedFunction->release();}

    if(mouseDownFunction) {mouseDownFunction->release();}
    if(mouseUpFunction) {mouseUpFunction->release();}
    if(mouseDraggedFunction) {mouseDraggedFunction->release();}
    if(mouseWheelFunction) {mouseWheelFunction->release();}


    if(clickFunction) {clickFunction->release();}
    if(rightClickFunction) {rightClickFunction->release();}
    if(clickOutsideFunction) {clickOutsideFunction->release();}
    if(clickDownOutsideFunction) {clickDownOutsideFunction->release();}
}

void MJView::cleanupRemovedSubviews()
{
    while(oldSubviewsToDelete.size() > 0)
    {
        MJView* deleteView = *(oldSubviewsToDelete.begin());
        delete deleteView;
        oldSubviewsToDelete.erase(deleteView);
    }
}

void MJView::invalidate()
{
	if(debugRenderFlag)
	{
		MJError("Removing view before UBOs have been sent.");
	}
    if(!invalidated)
    {
        invalidated = true;

		/*if(wasRemovedLuaFunction) {
			callMJLuaFunction(wasRemovedLuaFunction);
			delete wasRemovedLuaFunction; 
			wasRemovedLuaFunction = nullptr; 
		}*/


		destroyDrawables();
        
        removeAllSubviews();
        
        while(dependentViews.size() > 0)
        {
            MJView* dependentView = *(dependentViews.begin());
            dependentView->removeFromParentView();
            if(dependentViews.count(dependentView) > 0)
            {
                //this should never happen, invalidate() should have called removeDependentView()
                dependentViews.erase(dependentView);
            }
        }
        
        if(relativeView)
        {
            relativeView->removeDependentView(this);
        }
        
        
        if(!idString.empty())
        {
            viewsByID->erase(idString);
        }
        
    }
}


void MJView::destroyDrawables()
{

}

void MJView::createDrawables(MJRenderPass renderPass, int renderTargetCompatibilityIndex)
{

}

dvec2 MJView::getSize() const
{
    return size;
}

void MJView::setSize(dvec2 size_) //WARNING! MJTextView completely overrides this, so changes made here should be made there
{
	if(!approxEqualVec2(size, size_))
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
}

double MJView::getScale() const
{
    return scale;
}

void MJView::updateRenderScale()
{
    if(parentView)
    {
        renderScale = parentView->renderScale * scale;
    }
    else
    {
        renderScale = scale;
    }
    for(MJView* subView : subviews)
    {
        subView->updateRenderScale();
    }
}

void MJView::setScale(double scale_)
{
	if(!approxEqual(scale_, scale))
	{
		scale = scale_;
		updateRenderScale();
		updateMatrix();
	}
}

MJView* MJView::getRelativeView() const
{
    return relativeView;
}

void MJView::setRelativeView(MJView* relativeView_)
{
    if(relativeView)
    {
        relativeView->removeDependentView(this);
    }
	hasAdditionalMatrix = false;

    relativeView = relativeView_;
    if(relativeView)
    {
        relativeView->addDependentView(this);
		if(relativeView->hasAdditionalMatrix)
		{
			additionalMatrix = relativeView->additionalMatrix;
			hasAdditionalMatrix = true;
		}
    }
    updateMatrix();
}

MJViewPosition MJView::getRelativePosition() const
{
    return relativePosition;
}

void MJView::setRelativePosition(MJViewPosition relativePosition_)
{
	if(relativePosition.h != relativePosition_.h || relativePosition.v != relativePosition_.v)
	{
		relativePosition = relativePosition_;
		updateMatrix();
	}
}


dmat3 MJView::getRotation() const
{
	return rotation;
}

void MJView::setRotation(dmat3 rotation_)
{
	rotation = rotation_;
	updateMatrix();
}

dvec3 MJView::getBaseOffset() const
{
	return baseOffset;
}

void MJView::setBaseOffset(dvec3 baseOffset_)
{
	if(!approxEqualVec(baseOffset, baseOffset_))
	{
		baseOffset = baseOffset_;
		updateMatrix();
	}
}

dvec3 MJView::getAdditionalOffset() const
{
	return additionalOffset;
}

void MJView::setAdditionalOffset(dvec3 additionalOffset_)
{
	if(!approxEqualVec(additionalOffset, additionalOffset_))
	{
		additionalOffset = additionalOffset_;
		updateMatrix();
	}
}


double MJView::getAlpha() const
{
	return alpha;
}

void MJView::setAlpha(double alpha_)
{
	if(!approxEqual(alpha_, alpha))
	{
		alpha = alpha_;
	}
}


bool MJView::getRenderXRay() const
{
    return renderXRay;
}

void MJView::setRenderXRay(bool renderXRay_)
{
    if(renderXRay != renderXRay_)
    {
        renderXRay = renderXRay_;
        
        for(MJView* subview : subviews)
        {
            subview->setRenderXRay(renderXRay);
        }
    }
}


bool MJView::getHidden() const
{
    return hidden;
}

void MJView::setDepthTestEnabled(bool depthTestEnabled_)
{
    depthTestEnabled = depthTestEnabled_;
    for(MJView* subview : subviews)
    {
        subview->setDepthTestEnabled(depthTestEnabled);
    }
}

void MJView::parentHiddenChanged(bool hidden_)
{
	if(hidden_)
	{
		if(mouseInside)
		{
			mouseInside = false;
			if(hoverEndFunction)
			{
                hoverEndFunction->call("hoverEndFunction");
			}
		}

		for(int i = 0; i < MAX_MOUSE_BUTTONS; i++)
		{
			mouseDownWasInside[i] = false;
			sendClickEventOnMouseUpInside[i] = false;
			receivedMouseDown[i] = false;
		}
	}

	if(hiddenStateChangedFunction)
	{
        hiddenStateChangedFunction->call("hiddenStateChangedFunction", TUI_BOOL(hidden_));
	}

	std::vector<MJView*> subviewsCopy = subviews;
	for(MJView* subView : subviewsCopy)
	{
		subView->parentHiddenChanged(hidden_);
	}
}

void MJView::setHidden(bool hidden_)
{
    if(hidden != hidden_)
    {
        hidden = hidden_;
		parentHiddenChanged(hidden_);
    }
}

void MJView::setAdditionalMatrix(dmat4 additionalMatrix_)
{
	hasAdditionalMatrix = true;
	additionalMatrix = additionalMatrix_;
	combinedRenderMatrix = additionalMatrix * combinedRenderMatrixWithoutAdditionalMatrix;

	for(MJView* dependentView : dependentViews)
	{
		dependentView->setAdditionalMatrix(additionalMatrix_);
	}
	//updateMatrix();
}

void MJView::addedToParentView(MJView* parentView_)
{
    parentView = parentView_;

	isWorldView = parentView->isWorldView;
	windowZ = parentView->windowZ;
	renderXRay = parentView->renderXRay;
	//hackyFlagDontMultiplyUsedByWorldSpaceUIObjects = parentView->hackyFlagDontMultiplyUsedByWorldSpaceUIObjects;
    if(!relativeView)
    {
        setRelativeView(parentView);
    }
    updateRenderScale();
}

void MJView::updateMatrix(int sanityCheck)
{
    if(sanityCheck > 1000)
    {
        MJError("Circular view heirachy or > 1000 views");
        return;
    }
    vec2 relativeViewSize = vec2(0.0);
    double relativeViewRenderScale = renderScale;
    dmat4 relativeMatrix = dmat4(1.0);
    
    if(relativeView)
    {
        relativeViewSize = relativeView->getSize();
		if(relativeView == parentView)
		{
			relativeMatrix = relativeView->combinedRenderMatrixWithoutAdditionalMatrix;
		}
		else
		{
			relativeMatrix = relativeView->baseRenderMatrix;
		}
        relativeViewRenderScale = relativeView->renderScale;
    }

	
	relativeMatrix = translate(relativeMatrix, windowPosition);

	relativeMatrix = relativeMatrix * dmat4(windowRotation);

    dvec3 baseOffsetToUse = baseOffset * relativeViewRenderScale + dvec3(0.0,0.0,windowZRenderOffset);
	dvec3 localOffsetToUse = dvec3(0.0);
    
    switch (relativePosition.h) {
        case MJPositionOuterLeft:
			localOffsetToUse.x -= size.x * renderScale;
            break;
        case MJPositionInnerLeft:
            break;
        case MJPositionCenter:
			baseOffsetToUse.x += relativeViewSize.x * 0.5 * relativeViewRenderScale;
			localOffsetToUse.x -= size.x * 0.5 * renderScale;
            break;
        case MJPositionInnerRight:
			baseOffsetToUse.x += relativeViewSize.x * relativeViewRenderScale;
			localOffsetToUse.x -= size.x * renderScale;
            break;
        case MJPositionOuterRight:
			baseOffsetToUse.x += relativeViewSize.x * relativeViewRenderScale;
            break;
        default:
            break;
    }
    
    
    switch (relativePosition.v) {
        case MJPositionBelow:
			localOffsetToUse.y -= size.y * renderScale;
            break;
        case MJPositionBottom:
            break;
        case MJPositionCenter:
			baseOffsetToUse.y += relativeViewSize.y * 0.5 * relativeViewRenderScale;
			localOffsetToUse.y -= size.y * 0.5 * renderScale;
            break;
        case MJPositionTop:
			baseOffsetToUse.y += relativeViewSize.y * relativeViewRenderScale;
			localOffsetToUse.y -= size.y * renderScale;
            break;
        case MJPositionAbove:
			baseOffsetToUse.y += relativeViewSize.y * relativeViewRenderScale;
            break;
        default:
            break;
    }

	baseRenderMatrix = translate(relativeMatrix, baseOffsetToUse);
	baseRenderMatrix = baseRenderMatrix * dmat4(rotation);
	baseRenderMatrix = translate(baseRenderMatrix, localOffsetToUse);
	combinedRenderMatrix = translate(baseRenderMatrix, additionalOffset * relativeViewRenderScale);

	combinedRenderMatrixWithoutAdditionalMatrix = combinedRenderMatrix;
	if(hasAdditionalMatrix)
	{
		combinedRenderMatrix = additionalMatrix * combinedRenderMatrixWithoutAdditionalMatrix;
	}
	//combinedRenderMatrixInverse = inverse(combinedRenderMatrix); //ugh no


	//combinedRenderMatrix = additionalMatrix * combinedRenderMatrix;
    
    for(MJView* dependentView : dependentViews)
    {
        dependentView->updateMatrix(sanityCheck + 1);
    }

	if(!isWorldView)
	{
		MJView* parent = this;
		while(parent->parentView)
		{
			parent = parent->parentView;
		}

		parent->mouseHoverDirty = true;
	}

	validMatrix = true;
}

void MJView::orderBack(MJView* subView)
{
    std::vector<MJView*>::iterator found = std::find(subviews.begin(), subviews.end(), subView);
    
    if(found != subviews.end())
    {
        subviews.erase(found);
        subviews.insert(subviews.begin(), subView);
    }
}

void MJView::orderFront(MJView* subView)
{
    std::vector<MJView*>::iterator found = std::find(subviews.begin(), subviews.end(), subView);
    
    if(found != subviews.end())
    {
        subviews.erase(found);
        subviews.push_back(subView);
    }
}

void MJView::update(float dt)
{
    if(hidden || invalidated || !hasUpdateFunctionOrChildWithUpdateFunction)
    {
        return;
    }

    if(updateFunction)
    {
        
        TuiDebugInfo debugInfo;
        debugInfo.fileName = "MJView::update";
        TuiTable* args = new TuiTable(nullptr);
        
        TuiNumber* agr1Ref = new TuiNumber(dt);
        args->arrayObjects.push_back(agr1Ref);
        
        updateFunction->call(args, nullptr, &debugInfo);
        
        args->release();
        
        //updateFunction(dt);
    }
	
	for(int i = 0; i < subviews.size(); i++)
	{
        subviews[i]->update(dt);
	}
}

void MJView::childHasUpdateFunctionChanged(bool childHasUpdateFunction)
{
	bool newHasUpdateFunctionOrChildWithUpdateFunction = false;
	if(childHasUpdateFunction)
	{
		newHasUpdateFunctionOrChildWithUpdateFunction = true;
	}
	else 
	{
		//if(updateLuaFunction)
		{
		//	newHasUpdateFunctionOrChildWithUpdateFunction = true;
		}
		//else
		{
			for(MJView* subview : subviews)
			{
				if(subview->hasUpdateFunctionOrChildWithUpdateFunction)
				{
					newHasUpdateFunctionOrChildWithUpdateFunction = true;
					break;
				}
			}
		}
	}

	if(newHasUpdateFunctionOrChildWithUpdateFunction != hasUpdateFunctionOrChildWithUpdateFunction)
	{
		hasUpdateFunctionOrChildWithUpdateFunction = newHasUpdateFunctionOrChildWithUpdateFunction;
		if(parentView)
		{
			parentView->childHasUpdateFunctionChanged(hasUpdateFunctionOrChildWithUpdateFunction);
		}
	}
}

void MJView::preRender(GCommandBuffer* commandBuffer, MJRenderPass renderPass, int renderTargetCompatibilityIndex, double dt, double frameLerp, double animationTimer_)
{
    if(hidden || invalidated)
    {
        return;
    }
    
    if(needsAnimationTimerReset)
    {
        animationTimerOffset = -animationTimer_;
        animationTimer = 0.0;
        needsAnimationTimerReset = false;
    }
    else
    {
        animationTimer = animationTimer_ + animationTimerOffset;
    }
	
	if(!isWorldView && mouseHoverDirty)
	{
        //MJLog("hover dirty:%.2f", lastMouseMovedWindowRayStart.x);
		mouseMoved3D(MainController::getInstance()->getPointerRayStartUISpace(), MainController::getInstance()->getPointerRayDirectionUISpace(), false, true);
		mouseHoverDirty = false;
	}

	if(currentRenderPass.renderPass && (renderPass.renderPass != currentRenderPass.renderPass || renderPass.extent.width != currentRenderPass.extent.width || renderPass.extent.height != currentRenderPass.extent.height))
	{
		destroyDrawables();
		currentRenderPass = renderPass;
	}

	MJRenderPass subViewRenderPass = renderPass;
	int subViewRenderTargetCompatibilityIndex = renderTargetCompatibilityIndex;

	createDrawables(subViewRenderPass, subViewRenderTargetCompatibilityIndex);

	for(int i = 0; i < subviews.size(); i++)
	{
        subviews[i]->preRender(commandBuffer, subViewRenderPass, renderTargetCompatibilityIndex, dt, frameLerp, animationTimer);
	}

    cleanupRemovedSubviews();
}

void MJView::render(GCommandBuffer* commandBuffer)
{
	if(hidden || invalidated)
	{
		return;
	}

	debugRenderFlag = true;
	renderSelf(commandBuffer);
	
	for(int i = 0; i < subviews.size(); i++)
	{
        subviews[i]->render(commandBuffer);
	}
    
    cleanupRemovedSubviews();
}

void MJView::renderSelf(GCommandBuffer* commandBuffer)
{
//dsa
}

void MJView::updateUBOs(float parentAlpha, dvec3 camPos, dvec3 viewPos)
{
	if(!debugRenderFlag)
	{
		if(hidden || invalidated)
		{
			if(debugRenderFlag)
			{
				MJError("Not updating UBOs for view that has already been rendered.");
			}
			return;
		}
	}
	if(!validMatrix)
	{
		updateMatrix();
	}
	debugRenderFlag = false;
	for(MJView* subView : subviews)
	{
		subView->updateUBOs(parentAlpha * alpha, camPos, viewPos);
	}
}

void MJView::addSubview(MJView* subView)
{
    if(!subView)
    {
        MJLog("Trying to add nil subview");
        return;
    }
    if(std::find(subviews.begin(), subviews.end(), subView) != subviews.end())
    {
        MJLog("ERROR: Trying to add a view already added.");
        return;
    }
    
    subviews.push_back(subView);
    subView->addedToParentView(this);
	if(clipChildren || clippingParent)
	{
		subView->reccursivelySetClipParent((clipChildren ? this : clippingParent));
	}
}


void MJView::addDependentView(MJView* dependentView)
{
    dependentViews.insert(dependentView);
}

void MJView::removeDependentView(MJView* dependentView)
{
    dependentViews.erase(dependentView);
}

dvec2 MJView::windowPointToLocal(dvec3 windowRayStart, dvec3 windowRayDirection)
{
	mat4 objectMatrixInverse = inverse(combinedRenderMatrix);

	vec3 modelSpacePosA = objectMatrixInverse * vec4(windowRayStart.x, windowRayStart.y, windowRayStart.z, 1.0);
	vec3 modelSpacePosB = objectMatrixInverse * vec4(windowRayStart.x + windowRayDirection.x * 10000.0, windowRayStart.y + windowRayDirection.y * 10000.0, windowRayStart.z + windowRayDirection.z * 10000.0, 1.0);


	double fraction = reverseLinearInterpolate(0.0, modelSpacePosA.z, modelSpacePosB.z);

	return mix(dvec2(modelSpacePosA), dvec2(modelSpacePosB), fraction);
}


dvec2 MJView::localPointToWindow(dvec3 localPoint)
{
	return dvec2(combinedRenderMatrix * vec4(localPoint.x * renderScale, localPoint.y * renderScale, localPoint.z * renderScale, 1.0));
}

dvec2 MJView::locationRelativeToView(dvec2 localPoint, MJView* otherView)
{
	dmat4 parentConversionMatirx = combinedRenderMatrix * inverse(otherView->combinedRenderMatrix);
	dvec4 parentLocalPoint = parentConversionMatirx * (dvec4(localPoint.x * renderScale, localPoint.y * renderScale, 0.0, 1.0));
	return dvec2(parentLocalPoint.x, parentLocalPoint.y) / renderScale;
}

bool MJView::containsPoint(dvec3 windowRayStart, dvec3 windowRayDirection)
{
	if(hidden || invalidated)
	{
		return false;
	}
	dvec2 localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
    dvec2 localSize = getSize() * renderScale;

	if(useCircleHitRadius)
	{
		double distanceFromCenter2 = length2(localPoint - localSize * 0.5);
		return (distanceFromCenter2 < circleHitRadius2);
	}
	else
	{
		if(localPoint.x > 0 && localPoint.x < localSize.x &&
		   localPoint.y > 0 && localPoint.y < localSize.y)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool MJView::getIntersection(dvec3 rayStart, dvec3 rayDirection, double* distance)
{
	if(hidden || invalidated)
	{
		return false;
	}

	mat4 objectMatrixInverse = inverse(combinedRenderMatrix);

	vec3 modelSpacePosA = objectMatrixInverse * vec4(rayStart.x, rayStart.y, rayStart.z, 1.0);
	vec3 modelSpacePosB = objectMatrixInverse * vec4(rayStart.x + rayDirection.x * 10000.0, rayStart.y + rayDirection.y * 10000.0, rayStart.z + rayDirection.z * 10000.0, 1.0);
	double fraction = reverseLinearInterpolate(0.0, modelSpacePosA.z, modelSpacePosB.z);

	dvec2 localPoint = mix(dvec2(modelSpacePosA), dvec2(modelSpacePosB), fraction);
	dvec2 localSize = getSize() * renderScale;

	bool inside = false;

	if(masksEvents)
	{
		if(useCircleHitRadius)
		{
			double distanceFromCenter2 = length2(localPoint - localSize * 0.5);
			inside = (distanceFromCenter2 < circleHitRadius2);
		}
		else
		{
			if(localPoint.x > 0 && localPoint.x < localSize.x &&
				localPoint.y > 0 && localPoint.y < localSize.y)
			{

				inside = true;
			}
		}
	}

	/*if(!inside)
	{
		return false;
	}*/


	if(inside && distance)
	{
		//vec3 modelSpaceVec = (modelSpacePosB - modelSpacePosA) * (float)fraction;
		//vec3 raySpaceVec = combinedRenderMatrix * vec4(modelSpaceVec, 1.0);

		vec3 raySpaceVecB = combinedRenderMatrix * vec4(mix(modelSpacePosA, modelSpacePosB, fraction), 1.0);

		double thisDistance = length(dvec3(raySpaceVecB) - rayStart);

		if(*distance <= 0.0 || thisDistance < *distance)
		{
			*distance = thisDistance;
		}
	}


	for(MJView* subView : subviews)
	{
		inside = subView->getIntersection(rayStart, rayDirection, distance) || inside;
	}

	return inside;
}

bool MJView::mouseMoved3D(dvec3 windowRayStart, dvec3 windowRayDirection, bool obstructed, bool triggeredByUIMovement)
{
	if(hidden || invalidated)
	{
		return false;
	}
    

	bool inside = false;
	dvec2 localPoint;
	bool hasLocalPoint = false;


	if(masksEvents)
	{
		bool containsPointInView = containsPoint(windowRayStart, windowRayDirection);
		localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
		hasLocalPoint = true;

		if(!dragDistanceAboveClickOutsideThreshold)
		{
			double dragDistance2 = length2(localPoint - mouseDownLocalPoint);
			if(dragDistance2 > 4.0)
			{
				dragDistanceAboveClickOutsideThreshold = true;
                
                for(int i = 0; i < MAX_MOUSE_BUTTONS; i++)
                {
                    sendClickEventOnMouseUpInside[i] = false;
                }
			}
		}

		//dvec2 localSize = getSize() * renderScale;
		//MJLog("localMouseLoc: %.4f,%.4f : \tmax:%.4f,%.4f", localMouseLoc.x, localMouseLoc.y, localSize.x, localSize.y);

		if(containsPointInView && !obstructed)
		{
			if(!mouseInside)
			{
				if(!triggeredByUIMovement || allowViewMovementUnderCursorToTriggerHoverStartEvents)
				{
					mouseInside = true;
                    //MJLog("mouseInside:(%.4f,%.4f,%.4f)(%.4f,%.4f,%.4f) - %d", windowRayStart.x, windowRayStart.y, windowRayStart.z, windowRayDirection.x, windowRayDirection.y, windowRayDirection.z, triggeredByUIMovement);

					if(hoverStartFunction)
					{
                        TuiRef* localPointRef = new TuiVec2(localPoint / renderScale);
                        hoverStartFunction->call("hoverStartFunction", localPointRef);
                        localPointRef->release();
					}
				}
			}
			else
			{
				if(hoverMovedFunction)
				{
                    TuiRef* localPointRef = new TuiVec2(localPoint / renderScale);
                    hoverMovedFunction->call("hoverMovedFunction", localPointRef);
                    localPointRef->release();
				}
			}

			if(masksEvents)
			{
				inside = true;
			}
		}
		else
		{
			if(mouseInside)
			{
				mouseInside = false;
                //MJLog("outseide:(%.4f,%.4f,%.4f)(%.4f,%.4f,%.4f) - %d", windowRayStart.x, windowRayStart.y, windowRayStart.z, windowRayDirection.x, windowRayDirection.y, windowRayDirection.z, triggeredByUIMovement);

				if(hoverEndFunction)
				{
					//dvec2 localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
                    hoverEndFunction->call("hoverEndFunction");
				}
			}
			for(int i = 0; i < MAX_MOUSE_BUTTONS; i++)
			{
				sendClickEventOnMouseUpInside[i] = false; //once the cursor leaves, dont send a click event
			}
		}
	}


	if(mouseDraggedFunction)
	{
		for(int i = 0; i < MAX_MOUSE_BUTTONS; i++)
		{
			if(mouseDownWasInside[i])
			{
				if(!hasLocalPoint)
				{
					localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
					hasLocalPoint = true;
				}
                
                //mouseDraggedFunction(localPoint / renderScale);
                TuiDebugInfo debugInfo;
                debugInfo.fileName = "mouseDraggedFunction";
                TuiTable* args = new TuiTable(nullptr);
                
                TuiVec2* localPointRef = new TuiVec2(localPoint / renderScale);
                args->arrayObjects.push_back(localPointRef);
                
                mouseDraggedFunction->call(args, nullptr, &debugInfo);
                
                localPointRef->release();
			}
		}
	}

	if(invalidated)
	{
		return false;
	}

	std::vector<MJView*> subviewsCopy = subviews;

	bool insideAnySubview = false;

	for(int i = ((int)subviewsCopy.size()) - 1; i >=0; i--)
	{
		MJView* subView = subviewsCopy[i];
		bool insideSubview = subView->mouseMoved3D(windowRayStart, windowRayDirection, obstructed || insideAnySubview, triggeredByUIMovement);

		if(invalidated)
		{
			return false;
		}

		inside = inside || insideSubview;
		insideAnySubview = insideAnySubview || insideSubview;
	}

	return inside;
}

bool MJView::mouseMoved(dvec2 mousePos, bool obstructed)
{
    if(hidden || invalidated)
    {
        return false;
    }
    
	return mouseMoved3D(dvec3(mousePos.x, mousePos.y, 100.0), dvec3(0.0,0.0,-1.0), obstructed, false);
}

bool MJView::mouseDown3D(dvec3 windowRayStart, dvec3 windowRayDirection, bool obstructed, int buttonIndex)
{
	if(hidden || invalidated)
	{
		return false;
	}

	bool inside = false;

	bool containsPointInView = containsPoint(windowRayStart, windowRayDirection);


	dvec2 localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
	mouseDownLocalPoint = localPoint;
	dragDistanceAboveClickOutsideThreshold = false;

	if(containsPointInView && !obstructed)
	{
		mouseDownWasInside[buttonIndex] = true;
		sendClickEventOnMouseUpInside[buttonIndex] = true;
		if(mouseDownFunction)
		{
            //mouseDownFunction(buttonIndex, localPoint / renderScale);
            
            TuiRef* buttonIndexRef = new TuiNumber(buttonIndex);
            TuiRef* localPointRef = new TuiVec2(localPoint / renderScale);
            mouseDownFunction->call("mouseDownFunction", buttonIndexRef, localPointRef);
            buttonIndexRef->release();
            localPointRef->release();
            
            /*TuiDebugInfo debugInfo;
            debugInfo.fileName = "mouseDown3D";
            TuiTable* args = new TuiTable(nullptr);
            
            TuiNumber* buttonIndexRef = new TuiNumber(buttonIndex);
            args->arrayObjects.push_back(buttonIndexRef);
            
            TuiVec2* localPointRef = new TuiVec2(localPoint / renderScale);
            args->arrayObjects.push_back(localPointRef);
            
            mouseDownFunction->call(args, nullptr, &debugInfo);
            
            localPointRef->release();*/
		}

		if(masksEvents)
		{
			inside = true;
		}
	}
	else
	{
		mouseDownWasInside[buttonIndex] = false;
		sendClickEventOnMouseUpInside[buttonIndex] = false;
	}
	if(invalidated)
	{
		return false;
	}

	receivedMouseDown[buttonIndex] = true;

	std::vector<MJView*> subviewsCopy = subviews;

	bool insideAnySubview = false;

	for(int i = ((int)subviewsCopy.size()) - 1; i >=0; i--)
	{
		MJView* subView = subviewsCopy[i];
		bool insideSubview = subView->mouseDown3D(windowRayStart, windowRayDirection, obstructed || insideAnySubview, buttonIndex);

		if(invalidated)
		{
			return false;
		}

		if(insideSubview)
		{
			inside = true;
			//break;
		}

		insideAnySubview = insideAnySubview || insideSubview;
	}

	//if(!inside)
	{
		if(clickDownOutsideFunction)
		{
			if(!visibleUIContainsPointIncludingSubViews3D(windowRayStart, windowRayDirection))
			{ 
				dvec2 localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
                
                TuiRef* buttonIndexRef = new TuiNumber(buttonIndex);
                TuiRef* localPointRef = new TuiVec2(localPoint / renderScale);
                clickDownOutsideFunction->call("clickDownOutsideFunction", buttonIndexRef, localPointRef);
                buttonIndexRef->release();
                localPointRef->release();
			}
		}
	}

	return inside;
}

bool MJView::mouseWheel3D(dvec3 windowRayStart, dvec3 windowRayDirection, dvec2 scrollChange)
{
	if(hidden || invalidated)
	{
		return false;
	}

	bool inside = false;

	bool containsPointInView = containsPoint(windowRayStart, windowRayDirection);
	dvec2 localPoint = windowPointToLocal(windowRayStart, windowRayDirection);

	if(containsPointInView)
	{
		if(mouseWheelFunction)
		{
            //mouseWheelFunction(localPoint / renderScale, scrollChange);
            
            TuiDebugInfo debugInfo;
            debugInfo.fileName = "mouseWheel3D";
            TuiTable* args = new TuiTable(nullptr);
            
            TuiVec2* localPointRef = new TuiVec2(localPoint / renderScale);
            args->arrayObjects.push_back(localPointRef);
            
            TuiVec2* scrollChangeRef = new TuiVec2(scrollChange);
            args->arrayObjects.push_back(scrollChangeRef);
            
            mouseWheelFunction->call(args, nullptr, &debugInfo);
		}

		if(masksEvents)
		{
			inside = true;
		}
	}

	if(invalidated)
	{
		return false;
	}

	std::vector<MJView*> subviewsCopy = subviews;

	for(int i = ((int)subviewsCopy.size()) - 1; i >=0; i--)
	{
		MJView* subView = subviewsCopy[i];
		bool insideSubview = subView->mouseWheel3D(windowRayStart, windowRayDirection, scrollChange);

		if(invalidated)
		{
			return false;
		}

		if(insideSubview)
		{
			inside = true;
			break;
		}
	}

	return inside;
}

bool MJView::mouseDown(dvec2 mousePos, int buttonIndex, bool obstructed)
{
    if(hidden || invalidated)
    {
        return false;
    }
    
    return mouseDown3D(dvec3(mousePos.x, mousePos.y, 100.0), dvec3(0.0,0.0,-1.0), obstructed, buttonIndex);
}

bool MJView::visibleUIContainsPointIncludingSubViews3D(dvec3 windowRayStart, dvec3 windowRayDirection)
{
	if(hidden || invalidated || alpha < 0.001)
	{
		return false;
	}

	if(masksEvents)
	{
		if(containsPoint(windowRayStart, windowRayDirection))
		{
			return true;
		}
	}

	for(MJView* subView : subviews)
	{
		if(subView->visibleUIContainsPointIncludingSubViews3D(windowRayStart, windowRayDirection))
		{
			return true;
		}
	}

	return false;

}

bool MJView::visibleUIContainsPointIncludingSubViews(dvec2 mousePos)
{

	return visibleUIContainsPointIncludingSubViews3D(dvec3(mousePos.x, mousePos.y, 100.0), dvec3(0.0,0.0,-1.0));
}


bool MJView::mouseUp3D(dvec3 windowRayStart, dvec3 windowRayDirection, int buttonIndex)
{
	if(hidden || invalidated)
	{
		return false;
	}

	bool inside = false;

	if(mouseDownWasInside[buttonIndex])
	{
		if(sendClickEventOnMouseUpInside[buttonIndex])
		{
			if(containsPoint(windowRayStart, windowRayDirection))
			{
				if(buttonIndex == 0 && clickFunction)
				{
					dvec2 localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
                    TuiRef* localPointRef = new TuiVec2(localPoint / renderScale);
                    clickFunction->call("clickFunction", localPointRef);
                    localPointRef->release();
				}
                else if(buttonIndex == 1 && rightClickFunction)
				{
					dvec2 localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
                    TuiRef* localPointRef = new TuiVec2(localPoint / renderScale);
                    clickFunction->call("rightClickFunction", localPointRef);
                    localPointRef->release();
				}
			}
		}

		if(mouseUpFunction)
		{
			dvec2 localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
            //mouseUpFunction(buttonIndex, localPoint / renderScale);//mouseDownWasInside[buttonIndex]);
            
            TuiDebugInfo debugInfo;
            debugInfo.fileName = "mouseUp3D";
            TuiTable* args = new TuiTable(nullptr);
            
            TuiNumber* buttonIndexRef = new TuiNumber(buttonIndex);
            args->arrayObjects.push_back(buttonIndexRef);
            
            TuiVec2* localPointRef = new TuiVec2(localPoint / renderScale);
            args->arrayObjects.push_back(localPointRef);
            
            mouseUpFunction->call(args, nullptr, &debugInfo);
		}

		if(!masksEvents)
		{
			if(clickOutsideFunction && !dragDistanceAboveClickOutsideThreshold && receivedMouseDown[buttonIndex])
			{
				if(!visibleUIContainsPointIncludingSubViews3D(windowRayStart, windowRayDirection))
				{ 
					dvec2 localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
                    TuiRef* buttonIndexRef = new TuiNumber(buttonIndex);
                    TuiRef* localPointRef = new TuiVec2(localPoint / renderScale);
                    mouseDownFunction->call("clickOutsideFunction", buttonIndexRef, localPointRef);
                    buttonIndexRef->release();
                    localPointRef->release();
				}
			}
		}

		inside = true;
	}
	else 
	{
		if(clickOutsideFunction && !dragDistanceAboveClickOutsideThreshold && receivedMouseDown[buttonIndex])
		{
			if(!visibleUIContainsPointIncludingSubViews3D(windowRayStart, windowRayDirection))
			{ 
				dvec2 localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
                TuiRef* buttonIndexRef = new TuiNumber(buttonIndex);
                TuiRef* localPointRef = new TuiVec2(localPoint / renderScale);
                mouseDownFunction->call("clickOutsideFunction", buttonIndexRef, localPointRef);
                buttonIndexRef->release();
                localPointRef->release();
			}
		}

		if(mouseUpFunction && containsPoint(windowRayStart, windowRayDirection))
		{
			dvec2 localPoint = windowPointToLocal(windowRayStart, windowRayDirection);
            //mouseUpFunction(buttonIndex, localPoint / renderScale);//, mouseDownWasInside[buttonIndex]);
            
            TuiDebugInfo debugInfo;
            debugInfo.fileName = "mouseUp3D";
            TuiTable* args = new TuiTable(nullptr);
            
            TuiNumber* buttonIndexRef = new TuiNumber(buttonIndex);
            args->arrayObjects.push_back(buttonIndexRef);
            
            TuiVec2* localPointRef = new TuiVec2(localPoint / renderScale);
            args->arrayObjects.push_back(localPointRef);
            
            mouseUpFunction->call(args, nullptr, &debugInfo);
		}
	}



	if(invalidated)
	{
		return false;
	}


	std::vector<MJView*> subviewsCopy = subviews;

	for(MJView* subView : subviewsCopy)
	{
		inside = subView->mouseUp3D(windowRayStart, windowRayDirection, buttonIndex) || inside;
		if(invalidated)
		{
			return false;
		}
	}

	mouseDownWasInside[buttonIndex] = false;
	sendClickEventOnMouseUpInside[buttonIndex] = false;
	receivedMouseDown[buttonIndex] = false;

	return inside;
}

bool MJView::mouseUp(dvec2 mousePos, int buttonIndex)
{
    if(hidden || invalidated)
    {
        return false;
    }
    

	return mouseUp3D(dvec3(mousePos.x, mousePos.y, 100.0), dvec3(0.0,0.0,-1.0), buttonIndex);
}

bool MJView::keyChanged(bool isDown, int code, int modKey, bool isRepeat)
{
    if(hidden || invalidated)
    {
        return false;
    }
    
    std::vector<MJView*> subviewsCopy = subviews;
    for(auto subViewI = subviewsCopy.rbegin(); subViewI != subviewsCopy.rend(); ++subViewI)
    {
        if((*subViewI)->keyChanged(isDown, code, modKey, isRepeat))
        {
            return true;
        }
        if(invalidated)
        {
            return false;
        }
    }
    
    if(keyChangedFunction)
    {
        bool result = keyChangedFunction(isDown, code, modKey, isRepeat);
        if(result == true)
        {
            return true;
        }
        if(invalidated)
        {
            return false;
        }
    }
    return false;
}

void MJView::removeSubview(MJView* subView)
{
    if(subView)
    {
        std::vector<MJView*>::iterator found = std::find(subviews.begin(), subviews.end(), subView);
        
        if(found != subviews.end())
        {
            subviews.erase(found);
            subView->invalidate();
            oldSubviewsToDelete.insert(subView);
        }
        else
        {
            MJLog("View to remove not found in parent:%p", (void*)subView);
        }
    }
}

void MJView::removeFromParentView()
{
	if(parentView)
	{
		parentView->removeSubview(this);
	}
}

void MJView::removeAllSubviews()
{
    while(subviews.size() > 0)
    {
        MJView* subview = subviews.back();
        removeSubview(subview);
    }
}

std::string MJView::getDescription()
{
    std::string result = Tui::string_format("MJView %p (%.2f,%.2f)\nsubviews:\n", (void*)this, size.x, size.y);
    for(MJView* subView : subviews)
    {
        result = result + subView->getDescription();
    }
    return result;
}


void MJView::setCircleHitRadius(double radius)
{
	useCircleHitRadius = true;
	circleHitRadius2 = radius * radius;
}

void MJView::removeCircleHitRadius()
{
	useCircleHitRadius = false;
}

void MJView::setWindowZOffset(double windowZOffset)
{
	windowZRenderOffset = windowZOffset;
	windowZ = windowZOffset;
    updateMatrix();
}


double MJView::getWindowZOffset() const
{
	return windowZRenderOffset;
}

void MJView::setWindowRotation(dmat3 windowRotation_)
{
	windowRotation = windowRotation_;
	updateMatrix();
}


dmat3 MJView::getWindowRotation() const
{
	return windowRotation;
}

void MJView::setWindowPosition(dvec3 windowPosition_)
{
	windowPosition = windowPosition_;
	updateMatrix();
}

dvec3 MJView::getWindowPosition() const
{
	return windowPosition;
}

void MJView::reccursivelySetClipParent(MJView* clippingParent_)
{
	if((clippingParent_ == nullptr) != (clippingParent == nullptr))
	{
		destroyDrawables();
	}
	clippingParent = clippingParent_;

	if(!clipChildren)
	{
		for(MJView* subView : subviews)
		{
			subView->reccursivelySetClipParent(clippingParent_);
		}
	}
}

void MJView::setClipChildren(bool clipChildren_)
{
	if(clipChildren != clipChildren_)
	{
		clipChildren = clipChildren_;

		for(MJView* subView : subviews)
		{
			subView->reccursivelySetClipParent(clipChildren ? this : nullptr);
		}
	}
}

void MJView::setDisableClipping(bool disableClipping_)
{
	if(disableClipping != disableClipping_)
	{
		disableClipping = disableClipping_;
		if(clippingParent != nullptr)
		{
			destroyDrawables();
		}
	}
}


void MJView::resetAnimationTimer()
{
    needsAnimationTimerReset = true;
}


void MJView::loadFromFile(std::string filePath, TuiTable* parentTable)
{
    TuiTable* table = (TuiTable*)TuiRef::load(filePath, parentTable);
    if(table)
    {
        loadFromTable(table, true);
        table->release();
    }
}


void MJView::setRelativePosition(std::string alignmentX, std::string alignmentY)
{
    if(!alignmentX.empty() || !alignmentY.empty())
    {
        MJViewPosition newPos = relativePosition;
        if(!alignmentX.empty())
        {
            if(alignmentX == "left" || alignmentX == "innerLeft")
            {
                newPos.h = MJPositionInnerLeft;
            }
            else if(alignmentX == "outerLeft")
            {
                newPos.h = MJPositionOuterLeft;
            }
            else if(alignmentX == "right" || alignmentX == "innerRight")
            {
                newPos.h = MJPositionInnerRight;
            }
            else if(alignmentX == "outerRight")
            {
                newPos.h = MJPositionOuterRight;
            }
        }
        if(!alignmentY.empty())
        {
            if(alignmentY == "top" || alignmentX == "innerTop")
            {
                newPos.v = MJPositionTop;
            }
            else if(alignmentY == "outerTop" || alignmentY == "above")
            {
                newPos.v = MJPositionAbove;
            }
            else if(alignmentY == "bottom" || alignmentX == "innerBottom")
            {
                newPos.v = MJPositionBottom;
            }
            else if(alignmentY == "outerBottom" || alignmentY == "below")
            {
                newPos.v = MJPositionBelow;
            }
        }
        setRelativePosition(newPos);
    }
}

void MJView::loadFromTable(TuiTable* table, bool isRoot)
{
    //MJLog("MJView::loadFromTable:%s", table->getDebugString().c_str());
    
    
    if(table->hasKey("layoutParentID"))
    {
        relativeViewToLoadID = table->getString("layoutParentID");
    }
    
    if(table->hasKey("size"))
    {
        TuiRef* ref = table->objectsByStringKey["size"];
        if(ref->type() == Tui_ref_type_FUNCTION)
        {
            parentSizeChangedFunction = (TuiFunction*)ref;
            parentSizeChangedFunction->retain();
            
            TuiRef* inSizeRef = new TuiVec2(parentView->size);
            
            TuiRef* sizeRef = parentSizeChangedFunction->call("parentSizeChangedFunction", inSizeRef);
            stateTable->setVec2("size", ((TuiVec2*)sizeRef)->value);
            
            inSizeRef->release();
            sizeRef->release();
        }
        else
        {
            stateTable->setVec2("size", table->getVec2("size"));
        }
    }
    else
    {
        parentSizeChangedFunction = new TuiFunction([this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
            if(args && !args->arrayObjects.empty())
            {
                return args->arrayObjects[0]->retain();
            }
            return TUI_NIL;
        });
        
        TuiRef* inSizeRef = new TuiVec2(parentView->size);
        
        TuiRef* sizeRef = parentSizeChangedFunction->call("parentSizeChangedFunction", inSizeRef);
        stateTable->setVec2("size", ((TuiVec2*)sizeRef)->value);
        
        inSizeRef->release();
        sizeRef->release();
    }
    
    if(table->hasKey("pos"))
    {
        stateTable->set("pos", table->objectsByStringKey["pos"]);
    }
    
    if(table->hasKey("hidden"))
    {
        stateTable->set("hidden", table->objectsByStringKey["hidden"]);
    }
    
    idString = table->getString("id");
    
    if(!idString.empty())
    {
        (*viewsByID)[idString] = this;
    }
    
    setRelativePosition(table->getString("alignmentX"), table->getString("alignmentY"));
    
    TuiTable* subviewTable = table->getTable("subviews");
    if(subviewTable)
    {
        for(TuiRef* subViewRef : subviewTable->arrayObjects)
        {
            if(subViewRef->type() == Tui_ref_type_TABLE)
            {
                MJView::loadUnknownViewFromTable((TuiTable*)subViewRef, this, false);
            }
        }
    }
    
    if(isRoot)
    {
        doRelativeViewLayoutsForTablePostLoad();
    }
}

void MJView::doRelativeViewLayoutsForTablePostLoad()
{
    if(!relativeViewToLoadID.empty())
    {
        if(viewsByID->count(relativeViewToLoadID) != 0)
        {
            setRelativeView(viewsByID->at(relativeViewToLoadID));
        }
        else
        {
            MJError("In doRelativeViewLayoutsForTablePostLoad unable to find relative view with id:%s", relativeViewToLoadID.c_str());
        }
    }
    
    for(MJView* subView : subviews)
    {
        subView->doRelativeViewLayoutsForTablePostLoad();
    }
}

void MJView::tableKeyChanged(const std::string& key, TuiRef* value)
{
    if(key == "pos")
    {
        switch (value->type()) {
            case Tui_ref_type_VEC2:
                setBaseOffset(dvec3(((TuiVec2*)value)->value, 0.0));
                break;
            case Tui_ref_type_VEC3:
                setBaseOffset(((TuiVec3*)value)->value);
                break;
            default:
                MJError("Expected vec2 or vec3");
                break;
        }
    }
    else if(key == "size")
    {
        switch (value->type()) {
            case Tui_ref_type_VEC2:
                setSize(((TuiVec2*)value)->value);
                break;
            case Tui_ref_type_FUNCTION:
            {
                if(parentSizeChangedFunction)
                {
                    parentSizeChangedFunction->release();
                    parentSizeChangedFunction = nullptr;
                }
                parentSizeChangedFunction = (TuiFunction*)value;
                parentSizeChangedFunction->retain();
                
                TuiRef* inSizeRef = new TuiVec2(parentView->size);
                TuiRef* sizeRef = parentSizeChangedFunction->call("parentSizeChangedFunction", inSizeRef);
                stateTable->setVec2("size", ((TuiVec2*)sizeRef)->value);
                inSizeRef->release();
                sizeRef->release();
            }
                
                break;
            default:
                MJError("Expected vec2 or function");
                break;
        }
    }
    else if(key == "layoutParent")
    {
        switch (value->type()) {
            case Tui_ref_type_TABLE:
            {
                MJView* viewToAttachTo = (MJView*)((TuiTable*)value)->getUserData("_view");
                if(viewToAttachTo)
                {
                    setRelativeView(viewToAttachTo);
                }
            }
                break;
            case Tui_ref_type_NIL:
            {
                setRelativeView(nullptr);
            }
                break;
            default:
                MJError("Expected table or nil");
                break;
        }
    }
    else if(key == "alignmentX" || key == "alignmentY")
    {
        setRelativePosition(stateTable->getString("alignmentX"), stateTable->getString("alignmentY"));
    }
    else if(key == "hidden")
    {
        setHidden(stateTable->getBool("hidden"));
    }
    else if(key == "update")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(updateFunction)
                {
                    updateFunction->release();
                }
                updateFunction = (TuiFunction*)value;
                updateFunction->retain();
                childHasUpdateFunctionChanged(true);
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(updateFunction)
                {
                    updateFunction->release();
                    updateFunction = nullptr;
                }
                childHasUpdateFunctionChanged(false);
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
    else if(key == "mouseDown")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(mouseDownFunction)
                {
                    mouseDownFunction->release();
                }
                mouseDownFunction = (TuiFunction*)value;
                mouseDownFunction->retain();
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(mouseDownFunction)
                {
                    mouseDownFunction->release();
                    mouseDownFunction = nullptr;
                }
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
    else if(key == "mouseUp")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(mouseUpFunction)
                {
                    mouseUpFunction->release();
                }
                mouseUpFunction = (TuiFunction*)value;
                mouseUpFunction->retain();
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(mouseUpFunction)
                {
                    mouseUpFunction->release();
                    mouseUpFunction = nullptr;
                }
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
    else if(key == "mouseDragged")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(mouseDraggedFunction)
                {
                    mouseDraggedFunction->release();
                }
                mouseDraggedFunction = (TuiFunction*)value;
                mouseDraggedFunction->retain();
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(mouseDraggedFunction)
                {
                    mouseDraggedFunction->release();
                    mouseDraggedFunction = nullptr;
                }
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
    else if(key == "mouseWheel")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(mouseWheelFunction)
                {
                    mouseWheelFunction->release();
                }
                mouseWheelFunction = (TuiFunction*)value;
                mouseWheelFunction->retain();
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(mouseWheelFunction)
                {
                    mouseWheelFunction->release();
                    mouseWheelFunction = nullptr;
                }
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
    else if(key == "hoverStart")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(hoverStartFunction)
                {
                    hoverStartFunction->release();
                }
                hoverStartFunction = (TuiFunction*)value;
                hoverStartFunction->retain();
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(hoverStartFunction)
                {
                    hoverStartFunction->release();
                    hoverStartFunction = nullptr;
                }
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
    else if(key == "hoverEnd")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(hoverEndFunction)
                {
                    hoverEndFunction->release();
                }
                hoverEndFunction = (TuiFunction*)value;
                hoverEndFunction->retain();
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(hoverEndFunction)
                {
                    hoverEndFunction->release();
                    hoverEndFunction = nullptr;
                }
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
    else if(key == "click")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(clickFunction)
                {
                    clickFunction->release();
                }
                clickFunction = (TuiFunction*)value;
                clickFunction->retain();
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(clickFunction)
                {
                    clickFunction->release();
                    clickFunction = nullptr;
                }
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
}
