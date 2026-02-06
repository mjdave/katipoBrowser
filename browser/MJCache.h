
#ifndef __World__MJCache__
#define __World__MJCache__

#include <map>
#include <string>

#include "MJImageTexture.h"
#include "MJFont.h"

#include "Vulkan.h"
#include "GPipeline.h"

class Database;
class Camera;
class MJDataTexture;
class TuiTable;

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
    
    int modelIndexCounter = 0;
	std::map<std::string, std::map<int, GPipeline*>> pipelines;

    std::map<MJImageTexture*, DrawQuadDescriptorSets> imageDrawQuadDescriptorSets;

    std::set<std::string> availableFontFileNames;

	std::set<MJImageTexture*> unloadedTextures;

	VkCommandBuffer recordingCommandBuffer = nullptr;

	std::map <std::string, dvec2> fontOffsets;
	std::set <std::string> reversedFonts;
    
public:
    Vulkan* vulkan;
    Database* appDatabase;

	Camera* camera;

    
public:
    MJCache(Vulkan* vulkan_,
        Database* appDatabase_,
		Camera* camera_);
    ~MJCache();

	void recordStarted(VkCommandBuffer commandBuffer_);
	void recordEnded();

	MJImageTexture* getTextureWithOptions(std::string name, TuiTable* rootTable, MJImageTextureOptions options, std::string alphaChannel_ = "", bool disableCache = false);
	MJImageTexture* getTextureAbsolutePathWithOptions(std::string name, MJImageTextureOptions options, std::string alphaChannel_ = "", bool disableCache = false);
    MJImageTexture* getTexture(std::string name, TuiTable* rootTable, bool repeat = false, bool loadFlipped = false, bool mipmap_ = false, bool disableCache = false);
	MJImageTexture* getTextureAbsolutePath(std::string path, bool repeat = false, bool loadFlipped = false, bool mipmap_ = false, bool disableCache = false);
    
    MJFont* getFont(std::string name, int pointSize, double* resultScale);

	void setFontOffset(std::string name, dvec2 offset);
	void setFontReversed(std::string name, bool reversed);

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
