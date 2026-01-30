#pragma once

#include "MathUtils.h"
#include "Vulkan.h"
#include <array>

class MJCache;
class GPipeline;
class MJImageTexture;
class MJRenderTarget;
class MJVRRenderTarget;
class MJDrawable;

class MJDrawQuad
{
public:
    
    vec2 size;
    vec4 userData = vec4(0.0,0.0,0.0,0.0);
    
public:
    MJDrawQuad(MJCache* cache_, MJRenderPass renderPass, int renderTargetCompatibilityIndex, bool clip, vec2 size_);
    MJDrawQuad(MJCache* cache_, MJRenderPass renderPass, int renderTargetCompatibilityIndex, bool clip, MJImageTexture* imageTexture_, vec2 size_, vec2 texMin_ = vec2(0.0,0.0), vec2 texMax_ = vec2(1.0,1.0), std::string shaderName = "");
    MJDrawQuad(MJCache* cache_, MJRenderPass renderPass, int renderTargetCompatibilityIndex, bool clip, MJRenderTarget* renderTarget_, vec2 size_, vec2 texMin_ = vec2(0.0,0.0), vec2 texMax_ = vec2(1.0,1.0), std::string shaderName = "");
	MJDrawQuad(MJCache* cache_, MJRenderPass renderPass, int renderTargetCompatibilityIndex, bool clip, MJRenderTarget* renderTarget_, MJImageTexture* maskTexture_, vec2 size_, vec2 texMin_ = vec2(0.0,0.0), vec2 texMax_ = vec2(1.0,1.0), std::string shaderName = "");
	MJDrawQuad(MJCache* cache_, MJRenderPass renderPass, int renderTargetCompatibilityIndex, bool clip, MJVRRenderTarget* renderTarget_, vec2 size_, vec2 texMin_ = vec2(0.0,0.0), vec2 texMax_ = vec2(1.0,1.0), std::string shaderName = "");
    ~MJDrawQuad();

    void render(GCommandBuffer* mainCommandBuffer);
	void updateUBOs(mat4 mvpMatrix, dmat4 clipMatrix, vec4 color, bool greyScale = false);
    void setImageTeture(MJImageTexture* imageTexture_);
    void setSize(vec2 newSize);
    void setTexCoords(vec2 texMin_, vec2 texMax_);

private:
    MJCache* cache;
    //GPipeline* pipeline;
    bool textured = false;
	bool usingClipUBO = false;
    vec2 texMin;
    vec2 texMax;


	bool needsToReloadTexture = false;

    MJImageTexture* imageTexture = nullptr;
	MJImageTexture* maskTexture = nullptr;
    MJRenderTarget* renderTarget = nullptr;
	MJVRRenderTarget* vrRenderTarget = nullptr;

    //VkDescriptorPool descriptorPool;
    //std::vector<VkDescriptorSet> descriptorSets;

	MJDrawable* drawable = nullptr;

private:

};

