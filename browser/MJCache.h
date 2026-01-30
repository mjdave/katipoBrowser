//
//  MJCache.h
//  World
//
//  Created by David Frampton on 4/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#ifndef __World__MJCache__
#define __World__MJCache__

#include <map>
#include <string>

#include "MJImageTexture.h"
#include "MJFont.h"
#include "Model.h"

#include "Vulkan.h"
#include "GPipeline.h"

#include "MaterialManager.h"
#include "UILightingManager.h"

class MJRenderTarget;
class Database;
class MJRawImageTexture;
class Camera;
class MJAtmosphere;
class MJDataTexture;

struct CachedModelViewBuffers {
	MJVMABuffer vertexBuffer;
	int vertCount = 0;
	MJVMABuffer edgeDecalBuffer;
	int edgeDecalCount = 0;
};

struct DrawQuadDescriptorSets {
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
};


struct GameObjectViewPlaceholderInfo {
	int modelIndex;
	double scale;
};

class MJCache {
    
    std::map<std::string, MJImageTexture*> textures;
    std::map<std::string, MJFont*> fonts;
    std::map<std::string, Model*> models;
    
    int modelIndexCounter = 0;
	std::map<std::string, std::map<int, GPipeline*>> pipelines;

	std::map<std::string, CachedModelViewBuffers> modelViewBuffers;
	std::map<std::string, CachedModelViewBuffers> gameObjectViewBuffers;

    std::map<MJImageTexture*, DrawQuadDescriptorSets> imageDrawQuadDescriptorSets;

    std::set<std::string> availableFontFileNames;

	std::set<MJImageTexture*> unloadedTextures;

	VkCommandBuffer recordingCommandBuffer = nullptr;

	std::map <std::string, dvec2> fontOffsets;
	std::set <std::string> reversedFonts;
    
public:
    std::map<int, Model*> modelsByIndex;

    Vulkan* vulkan;
    Database* appDatabase;

    MaterialManager* materialManager;
    UILightingManager* uiLightingManager;
    MJRawImageTexture* brdfMap;
	Camera* camera;
	MJAtmosphere* atmosphere;
	MJDataTexture* noiseTexture;

    
public:
    MJCache(Vulkan* vulkan_,
        MaterialManager* materialManager_, 
        Database* appDatabase_,
		Camera* camera_,
		MJDataTexture* noiseTexture_);
    ~MJCache();

	void recordStarted(VkCommandBuffer commandBuffer_);
	void recordEnded();

	MJImageTexture* getTextureWithOptions(std::string name, MJImageTextureOptions options, std::string alphaChannel_ = "", bool disableCache = false);
	MJImageTexture* getTextureAbsolutePathWithOptions(std::string name, MJImageTextureOptions options, std::string alphaChannel_ = "", bool disableCache = false);
    MJImageTexture* getTexture(std::string name, bool repeat = false, bool loadFlipped = false, bool mipmap_ = false, bool disableCache = false);
	MJImageTexture* getTextureAbsolutePath(std::string path, bool repeat = false, bool loadFlipped = false, bool mipmap_ = false, bool disableCache = false);
    
    MJFont* getFont(std::string name, int pointSize, double* resultScale, bool isModel);

    Model* getModel(const std::string& modelPath);
	void cacheModel(const std::string& modelPath);
    
    Model* modelForModelIndex(int modelIndex);

	void setFontOffset(std::string name, dvec2 offset);
	void setFontReversed(std::string name, bool reversed);

	CachedModelViewBuffers getModelViewBuffers(GCommandBuffer* commandBuffer,
		Model* model,
		std::map<std::string, std::string>& materialRemaps, 
                                               const std::string& defualtMaterial);

	std::vector<MJVMABuffer> getCameraBuffer();

    GPipeline* getPipeline(std::string name,
		MJRenderPass renderPass,
        std::vector<VkVertexInputBindingDescription> bindingDescriptions,
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
        std::vector<PipelineUniform> uniforms_,
		int renderPassVarient = 0);
    
private:
	void loadUnloadedTextures();

};

#endif /* defined(__World__MJCache__) */
