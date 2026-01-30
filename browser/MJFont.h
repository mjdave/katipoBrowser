//
//  BMFont.h
//  World
//
//  Created by David Frampton on 3/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#ifndef __World__BMFont__
#define __World__BMFont__

#include "MathUtils.h"
#include <vector>
#include <map>
#include <string>
#include "Vulkan.h"

class MJCache;

struct AttributedText {
    std::string text;
    vec4 color;
	u8vec4 material;
};

class MJImageTexture;

struct FontVert {
    vec2 pos;
    vec2 tex;
    vec4 color;
    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(FontVert);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return std::vector<VkVertexInputBindingDescription>(1, bindingDescription);
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(FontVert, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(FontVert, tex);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(FontVert, color);

        return attributeDescriptions;
    }
};

struct ModelFontVert {
	vec2 pos;
	vec2 tex;
	u8vec4 material;

	static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(ModelFontVert);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return std::vector<VkVertexInputBindingDescription>(1, bindingDescription);
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(ModelFontVert, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(ModelFontVert, tex);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R8G8B8A8_UINT;
		attributeDescriptions[2].offset = offsetof(ModelFontVert, material);

		return attributeDescriptions;
	}
};

struct FontPrintResult { //huge hack but fuck C++ honestly. I aint making a subclass for this.
	std::vector<FontVert> fontVerts;
	std::vector<ModelFontVert> modelFontVerts;
};



struct FontNameAndSize
{
    std::string name;
    int size;
    
    
    FontNameAndSize()
    {
        this->size = 0;
    }
    
    FontNameAndSize(std::string name, int size)
    {
        this->name = name;
        this->size = size;
    }
};

class KearningInfo
{
public:
    int first;
    int second;
    int amount;
    
    KearningInfo() :  first( 0 ), second( 0 ), amount( 0 )	{ }
};


class CharDescriptor
{
    
public:
    int x, y;
    int w;
    int h;
    int xOffset;
    int yOffset;
    int xAdvance;
    int page;
    
    CharDescriptor() : x( 0 ), y( 0 ), w( 0 ), h( 0 ), xOffset( 0 ), yOffset( 0 ),
    xAdvance( 0 ), page( 0 )
    { }
};


struct AlignedChar {
	CharDescriptor* charD;
	float xAdvance;
	vec4 color;
	u8vec4 material;
};

struct FontLine{
	std::vector<AlignedChar> chars;
	float lineLength;
};

class MJFont {
public:
    
    MJImageTexture* texture;
	MJImageTexture* normalTexture;
    
    int lineHeight;
    
    MJFont(Vulkan* vulkan, MJCache* cache, std::string fontName, bool isModel_ = false, dvec2 offset = dvec2(0.0,0.0), bool reversed_ = false);
    ~MJFont();
    
	FontPrintResult print(std::vector<AttributedText>& attributedText,
		int alignment,
		int wrapWidth,
		irect* enclosingRect = nullptr,
		float scale = 1.0f);

    irect calculateEnclosingRect(std::vector<AttributedText>& attributedText, 
        int alignment,
        int wrapWidth,
        float scale = 1.0f);

	irect calculateRectOfCharAtIndex(std::vector<AttributedText>& attributedText, 
		int charIndex,
		int alignment,
		int wrapWidth,
		float scale = 1.0f);
    
    int calculateIndexOfCharAtPos(std::vector<AttributedText>& attributedText,
        dvec2 pos,
        int alignment,
        int wrapWidth,
        float scale = 1.0f);

private:
	std::vector<FontLine> getLines(std::vector<AttributedText>& attributedText, 
		int alignment,
		int wrapWidth,
		float scale = 1.0f);
    
    
private:
    std::map<int,CharDescriptor> chars;
    std::vector<KearningInfo> kearn;
    
    int base;
    int width;
    int height;
    float advX;
    float advY;
    int pages;
    int outline;
    int kernCount;
    bool useKern;

	bool isModel = false;
	bool reversed = false;
    
    int getKerningPair(int first, int second);


};

#endif /* defined(__World__BMFont__) */
