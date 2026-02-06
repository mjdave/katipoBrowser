
#ifndef __World__MJColorView__
#define __World__MJColorView__

#include "MJView.h"

class MJDrawQuad;
class MJDrawable;

class MJColorView : public MJView {
public:
    
    dvec4 color;
	dvec4 shaderUniformA;
	dvec4 shaderUniformB;

private:

	MJDrawable* drawable = nullptr;

	std::string shaderName;
	std::string shaderNameClip;
    
public:
    MJColorView(MJView* parentView_);
    virtual ~MJColorView();
    
    virtual std::string getDescription();
    
    virtual void setSize(dvec2 size_);

	dvec4 getColor() const;
	void setColor(dvec4 color_);

	void setShader(std::string shaderName_);
    
    virtual void loadFromTable(TuiTable* table, bool isRoot = false);
    virtual void tableKeyChanged(const std::string& key, TuiRef* value);
    
protected:
    
	virtual void destroyDrawables();
	virtual void createDrawables(MJRenderPass renderPass, int renderTargetCompatibilityIndex);

	virtual void renderSelf(GCommandBuffer* commandBuffer);

	virtual void preRender(GCommandBuffer* commandBuffer, MJRenderPass renderPass, int renderTargetCompatibilityIndex, double dt, double frameLerp, double animationTimer_);
	virtual void updateUBOs(float parentAlpha, dvec3 camPos, dvec3 viewPos);


	//void updateSizes();
};

#endif /* defined(__World__MJImageView__) */
