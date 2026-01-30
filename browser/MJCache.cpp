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
    MaterialManager* materialManager_, 
    Database* appDatabase_,
	Camera* camera_,
	MJDataTexture* noiseTexture_)
{
    vulkan = vulkan_;
    appDatabase = appDatabase_;
    materialManager = materialManager_;
	camera = camera_;
	noiseTexture = noiseTexture_;

	//debugTimer = new Timer();


    std::string fontDirname = Katipo::getResourcePath("common/fonts/fontFiles");

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
    for(auto const& modelI : models)
    {
        delete modelI.second;
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


MJImageTexture* MJCache::getTexture(std::string name, bool repeat, bool loadFlipped, bool mipmap, bool disableCache)
{
    return getTextureAbsolutePath(Katipo::getResourcePath(name), repeat, loadFlipped, mipmap, disableCache);
}

MJImageTexture* MJCache::getTextureWithOptions(std::string name, MJImageTextureOptions options, std::string alphaChannel_, bool disableCache)
{
	std::string absoluteAlphaPath = "";
	if(!alphaChannel_.empty())
	{
		absoluteAlphaPath = Katipo::getResourcePath(alphaChannel_);
	}
	return getTextureAbsolutePathWithOptions(Katipo::getResourcePath(name), options, absoluteAlphaPath, disableCache);
}

MJFont* MJCache::getFont(std::string name, int pointSize, double* resultScale, bool isModel)
{
    *resultScale = 1.0;

	std::string combinedFontName = Tui::string_format("%s%d", name.c_str(), pointSize);

	std::string key = Tui::string_format("%s_%d", combinedFontName.c_str(), isModel);

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
        MJFont* font = new MJFont(vulkan, this, combinedFontName, isModel, offset, reversed);
        fonts[key] = font;
        return font;
    }

    for(int i = pointSize + 1; i <= pointSize * 2; i++)
    {
        std::string combinedFontName = Tui::string_format("%s%d", name.c_str(), i);
		std::string key = Tui::string_format("%s_%d", combinedFontName.c_str(), isModel);
        if(fonts.count(key) > 0)
        {
            *resultScale = ((double)pointSize) / i;
            return fonts[key];
        }

        if(availableFontFileNames.count(combinedFontName) != 0)
        {
            MJFont* font = new MJFont(vulkan, this, combinedFontName, isModel, offset, reversed);
            fonts[key] = font;
            *resultScale = ((double)pointSize) / i;
            MJLog("WARNING: using larger substitute font of size:%d for:%s at size:%d", i, name.c_str(), pointSize);
            return font;
        }
    }

    for(int i = pointSize - 1; i >= pointSize / 2; i--)
    {
        std::string combinedFontName = Tui::string_format("%s%d", name.c_str(), i);
		std::string key = Tui::string_format("%s_%d", combinedFontName.c_str(), isModel);
        if(fonts.count(key) > 0)
        {
            *resultScale = ((double)pointSize) / i;
            return fonts[key];
        }

        if(availableFontFileNames.count(combinedFontName) != 0)
        {
            MJFont* font = new MJFont(vulkan, this, combinedFontName, isModel, offset, reversed);
            fonts[key] = font;
            *resultScale = ((double)pointSize) / i;
            MJLog("WARNING: using smaller substitute font of size:%d for:%s at size:%d", i, name.c_str(), pointSize);
            return font;
        }
    }



	for(int i = pointSize * 2; i <= 216; i++)
	{
		std::string combinedFontName = Tui::string_format("%s%d", name.c_str(), i);
		std::string key = Tui::string_format("%s_%d", combinedFontName.c_str(), isModel);
		if(fonts.count(key) > 0)
		{
			*resultScale = ((double)pointSize) / i;
			return fonts[key];
		}

		if(availableFontFileNames.count(combinedFontName) != 0)
		{
			MJFont* font = new MJFont(vulkan, this, combinedFontName, isModel, offset, reversed);
			fonts[key] = font;
			*resultScale = ((double)pointSize) / i;
			MJLog("WARNING: using larger substitute font of size:%d for:%s at size:%d", i, name.c_str(), pointSize);
			return font;
		}
	}

	for(int i = pointSize / 2; i >= 8; i--)
	{
		std::string combinedFontName = Tui::string_format("%s%d", name.c_str(), i);
		std::string key = Tui::string_format("%s_%d", combinedFontName.c_str(), isModel);
		if(fonts.count(key) > 0)
		{
			*resultScale = ((double)pointSize) / i;
			return fonts[key];
		}

		if(availableFontFileNames.count(combinedFontName) != 0)
		{
			MJFont* font = new MJFont(vulkan, this, combinedFontName, isModel, offset, reversed);
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
    std::string shaderPath = Katipo::getResourcePath("app/common/shaders/" + name + ".tui");
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

	std::string vertPathNameToUse = Katipo::getResourcePath("app/common/spv/" + vertPathname);
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


void MJCache::cacheModel(const std::string& modelPath)
{
	if(models.count(modelPath) == 0)
	{
        Model* model = new Model(modelPath,
			materialManager);
        
        model->index = ++modelIndexCounter;
        modelsByIndex[model->index] = model;

		models[modelPath] = model;
	}
}

Model* MJCache::getModel(const std::string& modelPath)
{
    if(models.count(modelPath) == 0)
	{
		Model* model = new Model(modelPath,
			materialManager);
        
        model->index = ++modelIndexCounter;
        modelsByIndex[model->index] = model;
        
        models[modelPath] = model;
        return model;
    }
    
    return models[modelPath];
}


Model* MJCache::modelForModelIndex(int modelIndex)
{
    if(modelsByIndex.count(modelIndex != 0))
    {
        return modelsByIndex[modelIndex];
    }
    
    return nullptr;
}



std::string getModelViewBufferHash(Model* model, std::map<std::string, std::string>& materialRemaps, std::string defaultMaterial)
{
	std::string hash = Tui::string_format("%s_%s_", model->modelPath.c_str(), defaultMaterial.c_str());

	for(auto& remap : materialRemaps)
	{
		hash = hash + Tui::string_format("%s_%s_", remap.first.c_str(), remap.second.c_str());
	}
	return hash;
}


CachedModelViewBuffers MJCache::getModelViewBuffers(GCommandBuffer* commandBuffer,
	Model* model, 
	std::map<std::string, std::string>& materialRemaps,
                                                    const std::string& defaultMaterial)
{
	CachedModelViewBuffers buffers;

	std::string hash = getModelViewBufferHash(model, materialRemaps, defaultMaterial);
	if(modelViewBuffers.count(hash) != 0)
	{
		return modelViewBuffers[hash];
	}

	std::vector<ObjectVert> renderVerts;
	std::vector<ObjectDecalVert> decalVerts;


	bool hasMaterialRemaps = false;
	if((defaultMaterial != "") || !materialRemaps.empty())
	{
		hasMaterialRemaps = true;
	}

	std::vector<ModelFace>& faces = model->faces;
	buffers.vertCount = (int)faces.size() * 3;

	renderVerts.resize(buffers.vertCount);
	for(int f = 0; f < faces.size(); f++)
	{
		u8vec4 shaderMaterial = faces[f].shaderMaterial;
        u8vec4 shaderMaterialB = faces[f].shaderMaterialB;
		if(hasMaterialRemaps)
		{
			int materialTypeIndex = faces[f].materialTypeIndex;
            const std::string& materialName = materialManager->materialNamesByID[materialTypeIndex];
            if(materialRemaps.count(materialName) != 0)
			{
				shaderMaterial = materialManager->getShaderMaterial(materialRemaps[materialName]);
                shaderMaterialB = materialManager->getShaderMaterialB(materialRemaps[materialName]);
			}
			else if(defaultMaterial != "")
			{
				shaderMaterial = materialManager->getShaderMaterial(defaultMaterial);
                shaderMaterialB = materialManager->getShaderMaterialB(defaultMaterial);
			}
		}

		for(int v = 0; v < 3; v++)
		{
			renderVerts[f * 3 + v].pos = faces[f].verts[v];
			renderVerts[f * 3 + v].uv = faces[f].uvs[v];
			renderVerts[f * 3 + v].normal = vec4(faces[f].normals[v] * 127.0f, 0);
			renderVerts[f * 3 + v].tangent = vec4(faces[f].tangents[v] * 127.0f, 0);
			renderVerts[f * 3 + v].material = shaderMaterial;
            renderVerts[f * 3 + v].materialB = shaderMaterialB;
		}
	}

	std::vector<ModelEdgeDecal>& edgeDecals = model->edgeDecals;
	if(!edgeDecals.empty())
	{
		buffers.edgeDecalCount = edgeDecals.size() * 6;
		decalVerts.resize(buffers.edgeDecalCount);
		for(int d = 0; d < edgeDecals.size(); d++)
		{
			for(int t = 0; t < 2; t++)
			{
				for(int v = 0; v < 3; v++)
				{
					int quadVertIndex = (v + (t == 0 ? 0 : 1)) % 4;
					int normalIndex = (v + (t == 0 ? 0 : 1)) % 2;
					int offsetIndex = (v + (t == 0 ? 0 : 1)) / 2;

					decalVerts[d * 6 + t * 3 + v].pos = edgeDecals[d].verts[quadVertIndex];
					decalVerts[d * 6 + t * 3 + v].normal = vec4(edgeDecals[d].normals[normalIndex] * vec3(127.0), 0);
					decalVerts[d * 6 + t * 3 + v].faceNormal = vec4(edgeDecals[d].faceNormal * vec3(127.0), 0);
					decalVerts[d * 6 + t * 3 + v].uv = vec3(edgeDecals[d].uvs[quadVertIndex], offsetIndex);
					decalVerts[d * 6 + t * 3 + v].material = edgeDecals[d].shaderMaterial;
                    decalVerts[d * 6 + t * 3 + v].materialB = edgeDecals[d].shaderMaterialB;
					decalVerts[d * 6 + t * 3 + v].decalLocalOrigin = (edgeDecals[d].verts[0] + edgeDecals[d].verts[1]) * vec3(0.5);
				}
			}
		}
	}


	std::string debugName = "MJCache::getModelViewBuffers_";
	debugName = debugName + model->debugName;

	VkDeviceSize vertexBufferSize = sizeof(renderVerts[0]) * renderVerts.size();
	vulkan->setupStaticBuffer(*commandBuffer->getBuffer(), &buffers.vertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize, renderVerts.data(), debugName.c_str());

	if(!model->edgeDecals.empty())
	{
		std::string debugName = "MJCache::getModelViewBuffers_edgeDecal_";
		debugName = debugName + model->debugName;

		VkDeviceSize edgeDecalBufferSize = sizeof(decalVerts[0]) * decalVerts.size();
		vulkan->setupStaticBuffer(*commandBuffer->getBuffer(), &buffers.edgeDecalBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, edgeDecalBufferSize, decalVerts.data(), debugName.c_str());
	}

	modelViewBuffers[hash] = buffers;
	return buffers;
}

std::vector<MJVMABuffer> MJCache::getCameraBuffer()
{
	return camera->cameraBuffer;
}
