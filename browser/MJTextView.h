//
//  MJTextView.h
//  World
//
//  Created by David Frampton on 4/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#ifndef __World__MJTextView__
#define __World__MJTextView__

#include "MJFont.h"
#include "MJView.h"
#include "Vulkan.h"

class MJFont;
class MJDrawable;

class MJTextView : public MJView {
    FontNameAndSize fontNameAndSize;
    MJFont* font;
    int textAlignment;

	MJDrawable* drawable = nullptr;
	MJDrawable* mainXRayDrawable = nullptr;
    bool bufferNeedsUpdating = false;
    bool additiveBlend = false;
    
    std::string textString;
    std::vector<AttributedText> text;
    dvec4 textColor;
    
    dvec2 textRenderOffset;
    
    int wrapWidth;
    double textRenderScale;
	double fontGeometryScale = 1.0;

    
public:
    MJTextView(MJView* parentView_);
    virtual ~MJTextView();
    
    virtual std::string getDescription();
    
    void setText(std::string text_);
    std::string getText() const;

    void addColoredText(std::string text_, dvec4 color);
    
    void setFontNameAndSize(FontNameAndSize fontNameAndSize_);
    FontNameAndSize getFontNameAndSize() const;
    
    
    void setAdditiveBlend(bool additiveBlend_);
    bool getAdditiveBlend() const;
    
    void setTextColor(dvec4 textColor);
    dvec4 getTextColor() const;
    
    void setTextAlignment(int textAlignment_);
    int getTextAlignment() const;
    
    void setWrapWidth(int wrapWidth_);
    int getWrapWidth() const;

	void setFontGeometryScale(double fontGeometryScale_);
	double getFontGeometryScale() const;

	void setDepthTestEnabled(bool depthTestEnabled_);

	dvec4 getRectForCharAtIndex(int index);
    int getCharIndexForPos(dvec2 pos);
    
    void resetVerticalCursorMovementAnchors();
    int getCursorOffsetForVerticalCursorMovement(int currentCursorOffset, int verticalOffset);
    
    virtual void loadFromTable(TuiTable* table, bool isRoot = false);
    virtual void tableKeyChanged(const std::string& key, TuiRef* value);

private:
    bool hasRenderData = false;
    std::vector<FontVert> vertices;
    MJVMABuffer vertexBuffer;
    
    std::vector<FontVert> nextVertices;
    MJVMABuffer nextVertexBuffer;

	bool usingClipUBO = false;
    std::map<int,int> cursorVerticalMovementAnchors;

    
protected:
	void updateDimensions();
    void updateBuffer(GCommandBuffer* commandBuffer);

	virtual void destroyDrawables();
	virtual void createDrawables(MJRenderPass renderPass, int renderTargetCompatibilityIndex);

	virtual void renderSelf(GCommandBuffer* commandBuffer);

	virtual void preRender(GCommandBuffer* commandBuffer, MJRenderPass renderPass, int renderTargetCompatibilityIndex, double dt, double frameLerp, double animationTimer);
	virtual void updateUBOs(float parentAlpha, dvec3 camPos, dvec3 viewPos);
};

#endif /* defined(__World__MJTextView__) */
