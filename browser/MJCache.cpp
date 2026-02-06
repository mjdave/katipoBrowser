//
//  MJCache.cpp
//  World
//
//  Created by David Frampton on 4/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#include "MJCache.h"
#include "TuiScript.h"
#include "MJRawImageTexture.h"
#include "Camera.h"
#include "GCommandBuffer.h"
#include "KatipoUtilities.h"
#include "ObjectVert.h"
//#include "Timer.h"

MJCache::MJCache(Vulkan* vulkan_,
    Database* appDatabase_,
	Camera* camera_,
	MJDataTexture* noiseTexture_)
{
    vulkan = vulkan_;
    appDatabase = appDatabase_;
	camera = camera_;
	noiseTexture = noiseTexture_;

	//debugTimer = new Timer();


    std::string fontDirname = Katipo::getResourcePath("app/common/fonts/fontFiles");

    std::vector<std::string> availableFontFileNamesVec = Tui::getDirectoryContents(fontDirname);
    for(auto& name : availableFontFileNamesVec)
    {
		std::string nameWithoutExtension = Tui::removeExtensionForPath(name);
		availableFontFileNames.insert(nameWithoutExtension);
    }
}

MJCache::~MJCache()
{

    for(auto const& textureI : textures)
    {
        delete textureI.second;
    }
    for(auto const& fontI : fonts)
    {
        delete fontI.second;
    }
	for(auto& keyAndMap : pipelines)
	{
		for(auto const& pipelineI : keyAndMap.second)
		{
			delete pipelineI.second;
		}
	}
    for(auto const& imageDrawQuadDescriptorSetsI : imageDrawQuadDescriptorSets)
    {
        vulkan->destroyDescriptorPool(imageDrawQuadDescriptorSetsI.second.descriptorPool);
    }

	for(auto const& modelViewBufferI : modelViewBuffers)
	{
		if(modelViewBufferI.second.vertCount > 0)
		{
			MJVMABuffer vertBuffer = modelViewBufferI.second.vertexBuffer;
			vulkan->destroySingleBuffer(vertBuffer);
			
		}
		if(modelViewBufferI.second.edgeDecalCount > 0)
		{
			MJVMABuffer edgeDecalBuffer = modelViewBufferI.second.edgeDecalBuffer;
			vulkan->destroySingleBuffer(edgeDecalBuffer);

		}
	}
	for(auto const& modelViewBufferI : gameObjectViewBuffers)
	{
		if(modelViewBufferI.second.vertCount > 0)
		{
			MJVMABuffer vertBuffer = modelViewBufferI.second.vertexBuffer;
			vulkan->destroySingleBuffer(vertBuffer);

		}
		if(modelViewBufferI.second.edgeDecalCount > 0)
		{
			MJVMABuffer edgeDecalBuffer = modelViewBufferI.second.edgeDecalBuffer;
			vulkan->destroySingleBuffer(edgeDecalBuffer);

		}
	}
}

void MJCache::loadUnloadedTextures()
{
	if(recordingCommandBuffer && !unloadedTextures.empty())
	{
		for(MJImageTexture* tex : unloadedTextures)
		{
			tex->load(recordingCommandBuffer);
		}
		unloadedTextures.clear();
	}
}

void MJCache::recordStarted(VkCommandBuffer commandBuffer_)
{
	recordingCommandBuffer = commandBuffer_;
	loadUnloadedTextures();
}

void MJCache::recordEnded()
{
	recordingCommandBuffer = nullptr;
}


MJImageTexture* MJCache::getTextureAbsolutePathWithOptions(std::string path, MJImageTextureOptions options, std::string alphaChannel_, bool disableCache)
{
    if(disableCache)
    {
        MJImageTexture* texture = new MJImageTexture(vulkan, path, options, alphaChannel_);
        unloadedTextures.insert(texture);
        loadUnloadedTextures();
        return texture;
    }
    
	std::string key = Tui::string_format("%s_%d_%d_%d_%d_%d_%d_%s", path.c_str(), options.repeat, options.loadFlipped, options.mipmap, options.minFilter, options.magFilter, options.mipmapMode, alphaChannel_.c_str());
	if(textures.count(key) > 0)
	{
		return textures[key];
	}

	MJImageTexture* texture = new MJImageTexture(vulkan, path, options, alphaChannel_);
	textures[key] = texture;
	unloadedTextures.insert(texture);
	loadUnloadedTextures();
	return texture;
}


MJImageTexture* MJCache::getTextureAbsolutePath(std::string path, bool repeat, bool loadFlipped, bool mipmap, bool disableCache)
{
	MJImageTextureOptions options;
	options.repeat = repeat;
	options.mipmap = mipmap;
	options.loadFlipped = loadFlipped;
	return getTextureAbsolutePathWithOptions(path, options, "", disableCache);
}

static inline std::string getKatipoResourcePath(const std::string& inPath, TuiTable* rootTable)
{
    TuiRef* getResourcePathFunc = ((TuiTable*)rootTable->get("file"))->get("getResourcePath");
    if(getResourcePathFunc)
    {
        TuiString* inPathRef = new TuiString(inPath);
        TuiRef* pathResult = ((TuiFunction*)getResourcePathFunc)->call("getResourcePathFunc", inPathRef);
        inPathRef->release();
        if(pathResult)
        {
            std::string returnResult = pathResult->getStringValue();
            pathResult->release();
            return returnResult;
        }
    }
    return Katipo::getResourcePath(inPath);
}

MJImageTexture* MJCache::getTexture(std::string name, TuiTable* rootTable, bool repeat, bool loadFlipped, bool mipmap, bool disableCache)
{
    return getTextureAbsolutePath(getKatipoResourcePath(name, rootTable), repeat, loadFlipped, mipmap, disableCache);
}

MJImageTexture* MJCache::getTextureWithOptions(std::string name, TuiTable* rootTable, MJImageTextureOptions options, std::string alphaChannel_, bool disableCache)
{
	std::string absoluteAlphaPath = "";
	if(!alphaChannel_.empty())
	{
		absoluteAlphaPath = getKatipoResourcePath(alphaChannel_, rootTable);
	}
	return getTextureAbsolutePathWithOptions(getKatipoResourcePath(name, rootTable), options, absoluteAlphaPath, disableCache);
}

MJFont* MJCache::getFont(std::string name, int pointSize, double* resultScale)
{
    *resultScale = 1.0;

	std::string combinedFontName = Tui::string_format("%s%d", name.c_str(), pointSize);

	std::string key = combinedFontName;

    if(fonts.count(key) > 0)
    {
        return fonts[key];
    }

	dvec2 offset = dvec2(0.0,0.0);
	bool reversed = false;
	if(fontOffsets.count(name) != 0)
	{
		offset = fontOffsets[name];
	}
	if(reversedFonts.count(name) != 0)
	{
		reversed = true;
	}

    if(availableFontFileNames.count(combinedFontName) != 0)
    {
        MJFont* font = new MJFont(vulkan, this, combinedFontName, offset, reversed);
        fonts[key] = font;
        return font;
    }

    for(int i = pointSize + 1; i <= pointSize * 2; i++)
    {
        std::string combinedFontName = Tui::string_format("%s%d", name.c_str(), i);
		std::string key = combinedFontName;
        if(fonts.count(key) > 0)
        {
            *resultScale = ((double)pointSize) / i;
            return fonts[key];
        }

        if(availableFontFileNames.count(combinedFontName) != 0)
        {
            MJFont* font = new MJFont(vulkan, this, combinedFontName, offset, reversed);
            fonts[key] = font;
            *resultScale = ((double)pointSize) / i;
            MJLog("WARNING: using larger substitute font of size:%d for:%s at size:%d", i, name.c_str(), pointSize);
            return font;
        }
    }

    for(int i = pointSize - 1; i >= pointSize / 2; i--)
    {
        std::string combinedFontName = Tui::string_format("%s%d", name.c_str(), i);
		std::string key = combinedFontName;
        if(fonts.count(key) > 0)
        {
            *resultScale = ((double)pointSize) / i;
            return fonts[key];
        }

        if(availableFontFileNames.count(combinedFontName) != 0)
        {
            MJFont* font = new MJFont(vulkan, this, combinedFontName, offset, reversed);
            fonts[key] = font;
            *resultScale = ((double)pointSize) / i;
            MJLog("WARNING: using smaller substitute font of size:%d for:%s at size:%d", i, name.c_str(), pointSize);
            return font;
        }
    }



	for(int i = pointSize * 2; i <= 216; i++)
	{
		std::string combinedFontName = Tui::string_format("%s%d", name.c_str(), i);
		std::string key = combinedFontName;
		if(fonts.count(key) > 0)
		{
			*resultScale = ((double)pointSize) / i;
			return fonts[key];
		}

		if(availableFontFileNames.count(combinedFontName) != 0)
		{
			MJFont* font = new MJFont(vulkan, this, combinedFontName, offset, reversed);
			fonts[key] = font;
			*resultScale = ((double)pointSize) / i;
			MJLog("WARNING: using larger substitute font of size:%d for:%s at size:%d", i, name.c_str(), pointSize);
			return font;
		}
	}

	for(int i = pointSize / 2; i >= 8; i--)
	{
		std::string combinedFontName = Tui::string_format("%s%d", name.c_str(), i);
		std::string key = combinedFontName;
		if(fonts.count(key) > 0)
		{
			*resultScale = ((double)pointSize) / i;
			return fonts[key];
		}

		if(availableFontFileNames.count(combinedFontName) != 0)
		{
			MJFont* font = new MJFont(vulkan, this, combinedFontName, offset, reversed);
			fonts[key] = font;
			*resultScale = ((double)pointSize) / i;
			MJLog("WARNING: using smaller substitute font of size:%d for:%s at size:%d", i, name.c_str(), pointSize);
			return font;
		}
	}

    MJError("no suitable font found for:%s at size:%d", name.c_str(), pointSize);
    return nullptr;
}


void MJCache::setFontOffset(std::string name, dvec2 offset)
{
	fontOffsets[name] = offset;
}

void MJCache::setFontReversed(std::string name, bool reversed)
{
	if(reversed)
	{
		reversedFonts.insert(name);
	}
	else
	{
		reversedFonts.erase(name);
	}
}

GPipeline* MJCache::getPipeline(std::string name,
	MJRenderPass renderPass,
    std::vector<VkVertexInputBindingDescription> bindingDescriptions,
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
    std::vector<PipelineUniform> uniforms,
	int renderPassVarient)
{
    
    std::string key = name;
    
    if(pipelines.count(key) != 0)
    {
        if(pipelines[key].count(renderPassVarient) != 0)
        {
            return pipelines[key][renderPassVarient];
        }
    }
    
    //MJLog("Loading shader:%s", name.c_str());
    std::string shaderPath = Katipo::getResourcePath("app/common/shaders/" + name + ".tui"); //todo allow sites to supply shaders
    //MJLog("shaderPath:%s", shaderPath.c_str());
    
    TuiTable* shaderTable = (TuiTable*)TuiRef::load(shaderPath, nullptr);
    
    //std::string vertPathname = ((TuiString*)shaderTable->objectsByStringKey["vertPath"])->value;
    std::string vertPathname = shaderTable->getString("vertPath");
    std::string fragPathname = shaderTable->getString("fragPath");
    

    PipelineStateOptions options = PipelineStateOptions();
    
    if(shaderTable->hasKey("blendMode"))
    {
        const std::string& luaString = shaderTable->getString("blendMode");
        if(luaString == "premultiplied")
        {
            options.blendMode = MJVK_BLEND_MODE_PREMULTIPLED;
        }
        else if(luaString == "nonPremultiplied")
        {
            options.blendMode = MJVK_BLEND_MODE_NONPREMULTIPLED;
        }
        else if(luaString == "additive")
        {
            options.blendMode = MJVK_BLEND_MODE_ADDITIVE;
        }
        else if(luaString == "subtractive")
        {
            options.blendMode = MJVK_BLEND_MODE_SUBTRACTIVE;
        }
        else if(luaString == "treatAlphaAsColorChannel")
        {
            options.blendMode = MJVK_BLEND_MODE_TREAT_ALPHA_AS_COLOR_CHANNEL;
        }
        else if(luaString == "disabled")
        {
            options.blendMode = MJVK_BLEND_MODE_DISABLED;
        }
    }
    
    if(shaderTable->hasKey("depth"))
    {
        const std::string& luaString = shaderTable->getString("depth");
        if(luaString == "disabled")
        {
            options.depthMode = MJVK_DEPTH_DISABLED;
        }
        else if(luaString == "testOnly")
        {
            options.depthMode = MJVK_DEPTH_TEST_ONLY;
        }
        else if(luaString == "maskOnly")
        {
            options.depthMode = MJVK_DEPTH_MASK_ONLY;
        }
        else if(luaString == "testAndMask")
        {
            options.depthMode = MJVK_DEPTH_TEST_AND_MASK;
        }
        else if(luaString == "testAndMaskEqual")
        {
            options.depthMode = MJVK_DEPTH_TEST_AND_MASK_EQUAL;
        }
    }
    
    if(shaderTable->hasKey("cull"))
    {
        const std::string& luaString = shaderTable->getString("cull");
        if(luaString == "disabled")
        {
            options.cullMode = MJVK_CULL_DISABLED;
        }
        else if(luaString == "front")
        {
            options.cullMode = MJVK_CULL_FRONT;
        }
        else if(luaString == "back")
        {
            options.cullMode = MJVK_CULL_BACK;
        }
        else if(luaString == "testAndMask")
        {
            options.cullMode = MJVK_DEPTH_TEST_AND_MASK;
        }
    }
    
    if(shaderTable->hasKey("topology"))
    {
        const std::string& luaString = shaderTable->getString("topology");
        if(luaString == "triangles")
        {
            options.topology = MJVK_TRIANGLES;
        }
        else if(luaString == "points")
        {
            options.topology = MJVK_POINTS;
        }
        else if(luaString == "fan")
        {
            options.topology = MJVK_FAN;
        }
        else if(luaString == "lines")
        {
            options.topology = MJVK_LINES;
        }
    }
    
    delete shaderTable;
    shaderTable = nullptr;

	std::string vertPathNameToUse = Katipo::getResourcePath("app/common/spv/" + vertPathname); //todo sites
	std::string fragPathNameToUse = Katipo::getResourcePath("app/common/spv/" + fragPathname);

	//debugTimer->getDt();
    GPipeline* pipeline = new GPipeline(vulkan,
        name,
        renderPass,
		vertPathNameToUse,
		fragPathNameToUse,
        bindingDescriptions,
        attributeDescriptions,
        uniforms,
        options);

	//totalLoadTime += debugTimer->getDt();
	//MJLog("total pipiline creation time:%dms", (int)(totalLoadTime * 1000.0));
	pipelines[key][renderPassVarient] = pipeline;
    return pipeline;
}

std::vector<MJVMABuffer> MJCache::getCameraBuffer()
{
	return camera->cameraBuffer;
}
