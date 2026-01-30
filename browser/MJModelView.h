//
//  MJTextView.h
//  World
//
//  Created by David Frampton on 4/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#ifndef __World__MJModelView__
#define __World__MJModelView__

#include "MJView.h"

#include "Model.h"
//#include "ClientModel.h"
#include "MJCache.h"

class MJDrawable;


struct ModelViewAnimationFrame {
	int frameIndex;
	float interpolationSpeed;
};


class MJModelView : public MJView {
public:
    Model* model = nullptr;

    MJDrawable* mainDrawable = nullptr;
    MJDrawable* decalDrawable = nullptr;

	MJDrawable* mainXRayDrawable = nullptr;

	std::vector<MJVMABuffer>* mainDrawableUBOBuffers;

	std::map<std::string, std::string> materialRemaps;
    std::string defaultMaterial = "";

	CachedModelViewBuffers modelViewBuffers;

    //MJVMABuffer vertexBuffer;
    //MJVMABuffer edgeDecalBuffer;
   // bool vertexBufferHasBeenCreated = false;
   // bool edgeDecalBufferHasBeenCreated = false;

    float easeIn;
    float easeOut;
	ModelViewAnimationFrame frames[4];
    float interpolationFraction;

    dvec4 color;
    dvec3 scale3D = dvec3(1.0,1.0,1.0);

	bool usingClipUBO = false;
    
public:
	MJModelView(MJView* parentView_);
    virtual ~MJModelView();
    
    virtual std::string getDescription();

    virtual void preRender(GCommandBuffer* commandBuffer, MJRenderPass renderPass, int renderTargetCompatibilityIndex, double dt, double frameLerp, double animationTimer);
	virtual bool containsPoint(dvec3 windowRayStart, dvec3 windowRayDirection);
    
    dvec3 getScale3D() const;
    void setScale3D(dvec3 scale3D_);

	void setUsesModelHitTest(bool useModelHitTest_);
	void setDepthTestEnabled(bool depthTestEnabled_);
	void setRadialMaskFraction(double raidalMaskFraction_);
    
    void setModel(const std::string& modelPath_, std::map<std::string, std::string>& remaps, std::string defaultMaterial_);
    void setModel(const std::string& modelPath_);
	void removeModel();
    
    
    virtual void loadFromTable(TuiTable* table, bool isRoot = false);


protected:

	bool useModelHitTest = false;
	bool radialMask = false;
	float radialMaskFraction = 0.0;
    std::string modelPath;
	bool needsToLoadNewModel = false;
    
protected:


	virtual void destroyDrawables();
	virtual void createDrawables(MJRenderPass renderPass, int renderTargetCompatibilityIndex);

	virtual void renderSelf(GCommandBuffer* commandBuffer);
	virtual void updateUBOs(float parentAlpha, dvec3 camPos, dvec3 viewPos);
};

#endif /* defined(__World__MJImageView__) */
