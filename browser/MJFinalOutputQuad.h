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

class MJFinalOutputQuad
{
public:
	MJFinalOutputQuad(MJCache* cache_, MJRenderPass renderPass, MJRenderTarget* renderTarget_, MJRenderTarget* bloomRenderTarget);
	~MJFinalOutputQuad();
    void render(GCommandBuffer* mainCommandBuffer);

private:
    MJCache* cache;

    MJRenderTarget* renderTarget = nullptr;

	MJDrawable* drawable = nullptr;

private:

};

