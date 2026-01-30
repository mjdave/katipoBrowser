//
//  MJTextView.h
//  World
//
//  Created by David Frampton on 4/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#ifndef __World__MJImageView__
#define __World__MJImageView__

#include "MJView.h"

class MJImageTexture;
class GPipeline;

class MJImageView : public MJView {
public:
    MJDrawQuad* drawQuad;
    
    std::string shaderName;
    
    MJImageTexture* imageTexture;
    dvec4 color;
    
    dvec2 imageSize;
    dvec2 imageOffset;
    
    bool cropImage = false;
    vec4 shaderUserData = vec4(0.0,0.0,0.0,0.0);

private:

	bool usingClipUBO = false;
    
public:
    MJImageView(MJView* parentView_);
    virtual ~MJImageView();
    
    virtual std::string getDescription();
    
    dvec2 getImageSize() const;
    void setImageSize(dvec2 imageSize_);
    
    dvec2 getImageOffset() const;
    void setImageOffset(dvec2 imageOffset_);
    
    MJImageTexture* getImageTexture() const;
    void setImageTexture(MJImageTexture* imageTexture_);
    
    std::string getShaderName() const;
    void setShaderName(std::string shaderName_);
    
    void setCropImage(bool cropImage_);
    
    virtual void setSize(dvec2 size_);
    
    virtual void loadFromTable(TuiTable* table, bool isRoot = false);
    virtual void tableKeyChanged(const std::string& key, TuiRef* value);
    
protected:

	virtual void destroyDrawables();
	virtual void createDrawables(MJRenderPass renderPass, int renderTargetCompatibilityIndex);

	virtual void updateUBOs(float parentAlpha, dvec3 camPos, dvec3 viewPos);
	virtual void renderSelf(GCommandBuffer* commandBuffer);
    
    void updateTexCoords();
};

#endif /* defined(__World__MJImageView__) */
