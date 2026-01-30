//
//  BMFont.cpp
//  World
//
//  Created by David Frampton on 3/08/15.
//  Copyright (c) 2015 Majic Jungle. All rights reserved.
//

#include "MJFont.h"

#include "FileUtils.h"
#include "MJImageTexture.h"
#include "MJCache.h"
#include <fstream>
#include <sstream>
#include "utf8.h"

MJFont::MJFont(Vulkan* vulkan, MJCache* cache, std::string fontName, bool isModel_, dvec2 offset, bool reversed_)
{
    kernCount = 0;
    useKern = true;
	isModel = isModel_;
	reversed = reversed_;
    
    std::string fontFilename = getResourcePath("common/fonts/fontFiles/" + fontName + ".fnt");
    std::string fontImageFilename = getResourcePath("common/fonts/maps/" + fontName + ".png");
    
    std::ifstream stream((fontFilename).c_str());
    if ( !stream.is_open() )
    {
        MJLog("ERROR: Cannot find Font File %s", fontFilename.c_str());
        return;
    }
    
    bool loadFlipped = true;
    bool repeat = false;
    texture = cache->getTextureAbsolutePath(fontImageFilename, repeat, loadFlipped, false);

	if(isModel)
	{
		std::string fontNormalImageFilename = getResourcePath("common/fonts/normalMaps/" + fontName + ".png");
		normalTexture = cache->getTextureAbsolutePath(fontNormalImageFilename, repeat, loadFlipped, true);
		if(!normalTexture->valid)
		{
			MJWarn("Failed to load normal texture for font:%s, falling back to flat default", fontName.c_str());
			std::string defaultFontNormalImageFilename = getResourcePath("common/fonts/normalMaps/default.png");
			normalTexture = cache->getTextureAbsolutePath(defaultFontNormalImageFilename, repeat, loadFlipped, true);
		}
	}
    
    std::string line;
    std::string read, key, value;
    std::size_t i;
    int first,second,amount;

	int size = 1;
    
    KearningInfo kearningInfo;
    CharDescriptor charDescriptor;
    
    while( !stream.eof() )
    {
        std::stringstream lineStream;
        std::getline( stream, line );
        lineStream << line;
        
        //read the line's type
        lineStream >> read;

		if( read == "info" )
		{
			//this holds common data
			while( !lineStream.eof() )
			{
				std::stringstream converter;
				lineStream >> read;
				i = read.find( '=' );
				key = read.substr( 0, i );
				value = read.substr( i + 1 );

				//assign the correct value
				converter << value;
				if( key == "size" )
				{converter >> size;}
			}
		}
        else if( read == "common" )
        {
            //this holds common data
            while( !lineStream.eof() )
            {
                std::stringstream converter;
                lineStream >> read;
                i = read.find( '=' );
                key = read.substr( 0, i );
                value = read.substr( i + 1 );
                
                //assign the correct value
                converter << value;
                if( key == "lineHeight" )
                {converter >> lineHeight;}
                else if( key == "base" )
                {converter >> base;}
                else if( key == "scaleW" )
                {converter >> width;}
                else if( key == "scaleH" )
                {converter >> height;}
                else if( key == "pages" )
                {converter >> pages;}
                else if( key == "outline" )
                {converter >> outline;}
            }
        }
        else if( read == "char" )
        {
            //This is data for each specific character.
            int charID = 0;
            
            while( !lineStream.eof() )
            {
                std::stringstream converter;
                lineStream >> read;
                i = read.find( '=' );
                key = read.substr( 0, i );
                value = read.substr( i + 1 );
                
                //Assign the correct value
                converter << value;
                if( key == "id" )
                {converter >> charID;}
                else if( key == "x" )
                {	converter >> charDescriptor.x;}
                else if( key == "y" )
                {	converter >> charDescriptor.y;}
                else if( key == "width" )
                {	converter >> charDescriptor.w;}
                else if( key == "height" )
                {	converter >> charDescriptor.h;}
                else if( key == "xoffset" )
                {	converter >> charDescriptor.xOffset;}
                else if( key == "yoffset" )
                {	converter >> charDescriptor.yOffset;}
                else if( key == "xadvance" )
                {	converter >> charDescriptor.xAdvance;}
                else if( key == "page" )
                {	converter >> charDescriptor.page;}
            }

			charDescriptor.yOffset = charDescriptor.yOffset - ((int)(offset.y * ((double)size)));
			charDescriptor.xOffset = charDescriptor.xOffset - ((int)(offset.x * ((double)size)));
            
            chars.insert(std::map<int,CharDescriptor>::value_type( charID,charDescriptor ));
            
        }
        else if( read == "kernings" )
        {
            while( !lineStream.eof() )
            {
                std::stringstream converter;
                lineStream >> read;
                i = read.find( '=' );
                key = read.substr( 0, i );
                value = read.substr( i + 1 );
                
                //assign the correct value
                converter << value;
                if( key == "count" )
                {converter >> kernCount; }
            }
        }
        else if( read == "kerning" )
        {
            while( !lineStream.eof() )
            {
                std::stringstream converter;
                lineStream >> read;
                i = read.find( '=' );
                key = read.substr( 0, i );
                value = read.substr( i + 1 );
                
                //assign the correct value
                converter << value;
                if( key == "first" )
                {converter >> kearningInfo.first; converter >> first; }
                
                else if( key == "second" )
                {converter >> kearningInfo.second; converter >> second; }
                
                else if( key == "amount"  )
                {converter >> kearningInfo.amount; converter >> amount;}
            }
            //LOG_DEBUG("Done with this pass");
            kearn.push_back(kearningInfo);
        }
    }
    
    stream.close();
    
    
    advX = 1.0f/width;
    advY = 1.0f/height;
}

MJFont::~MJFont()
{
}

int MJFont::getKerningPair(int first, int second)
{
    if ( kernCount && useKern ) //Only process if there actually is kerning information
    {
        //Kearning is checked for every character processed. This is expensive in terms of processing time.
        for (int j = 0; j < kernCount;  j++ )
        {
            if (kearn[j].first == first && kearn[j].second == second)
            {
                return kearn[j].amount;
            }
        }
    }
    return 0;
}


FontPrintResult MJFont::print(std::vector<AttributedText>& attributedText,
	int alignment,
	int wrapWidth,
	irect* enclosingRect,
	float scale)
{
	FontPrintResult result;
    
    std::vector<FontLine> lines;
    int lineIndex = 0;
    
    FontLine line;
    line.lineLength = 0.0f;
    lines.push_back(line);

    bool wordIsTooLongForLine = false;

    for(int groupBase = 0; groupBase < attributedText.size(); groupBase++)
    {
		int group = groupBase;
		int nextGroup = group + 1;
		if(reversed)
		{
			group = attributedText.size() - groupBase - 1;
			nextGroup = group - 1;
		}
		std::string& text = attributedText[group].text;

		auto start_it = text.begin();
		auto end_it = utf8::find_invalid(text.begin(), text.end());
		if (end_it != text.end()) {
			MJWarn("invalid utf8 string:%s", text.c_str());
		}

		for(auto it = start_it; it != end_it; )
		{
			auto thisIt = it;
			uint32_t textCode = utf8::next(it, end_it);
			FontLine* line = &(lines[lineIndex]);
			bool skipTrailingSpace = false;
			if(textCode == '\n')
			{
				lineIndex++;
			}
			else
			{
				if(wrapWidth > 0)
				{
					if(wordIsTooLongForLine)
					{
						CharDescriptor* nextChar = &chars[textCode];
						if(line->lineLength + ((float)(nextChar->xAdvance)) / scale >= wrapWidth)
						{
							wordIsTooLongForLine = false;
							lineIndex++;
							if(textCode == ' ')
							{
								skipTrailingSpace = true;
							}
						}
					}
					else
					{
						float wordWidth = 0;
						bool first = true;
						for(auto otherIt = thisIt; otherIt != end_it;)
						{
							uint32_t otherTextCode = utf8::next(otherIt, end_it);
							if((otherTextCode == ' ' && !first) || otherTextCode == '\n')
							{
								break;
							}
							else
							{
								CharDescriptor* nextChar = &chars[otherTextCode];
								wordWidth += ((float)(nextChar->xAdvance)) / scale;
							}
							first = false;
						}

						if(line->lineLength + wordWidth >= wrapWidth)
						{
							if(line->chars.size() <= 1)
							{
								wordIsTooLongForLine = true;
							}
							else
							{
								lineIndex++;
								if(textCode == ' ')
								{
									skipTrailingSpace = true;
								}
							}
						}
					}
				}
			}

			if(lines.size() == lineIndex)
			{
				FontLine newLine;
				newLine.lineLength = 0.0f;
				lines.push_back(newLine);
			}

			if(chars.count(textCode) != 0 && !skipTrailingSpace)
			{
				line = &(lines[lineIndex]);

				CharDescriptor* charD = &chars[textCode];
				AlignedChar alignedChar;
				alignedChar.charD = charD;

				alignedChar.xAdvance = ((float)charD->xAdvance) / scale;

				if(isModel)
				{
					alignedChar.material = attributedText[group].material;
				}
				else
				{
					alignedChar.color = attributedText[group].color;
				}

				if(text.size() > 1 && it != end_it)
				{
					if(reversed)
					{
						alignedChar.xAdvance += ((float)getKerningPair(utf8::peek_next(it, end_it), textCode)) / scale;
					}
					else
					{
						alignedChar.xAdvance += ((float)getKerningPair(textCode,utf8::peek_next(it, end_it))) / scale;
					}
				}
				else if(nextGroup < attributedText.size() && nextGroup >= 0 && attributedText[nextGroup].text.size() > 0) //probably broken for reversed text
				{
					std::string& nextText = attributedText[nextGroup].text;
					uint32_t nextCode = utf8::peek_next(nextText.begin(), nextText.end());

					if(reversed)
					{
						alignedChar.xAdvance += ((float)getKerningPair(nextCode, textCode)) / scale;
					}
					else
					{
						alignedChar.xAdvance += ((float)getKerningPair(textCode, nextCode)) / scale;
					}
				}

				line->lineLength += alignedChar.xAdvance;
				line->chars.push_back(alignedChar);
			}

		}
	}
    
    float minX = FLT_MAX;
    float maxWidth = 0.0f;
    
    float y = 0.0f;
    for(const FontLine& line : lines)
    {
        float x = 0.0f;
        if(alignment == MJHorizontalAlignmentCenter)
        {
            x = floorf(line.lineLength * -0.5);
        }
        else if(alignment == MJHorizontalAlignmentRight)
        {
            x = line.lineLength * -1.0;
        }
        
        minX = MIN(minX, x);
        maxWidth = MAX(maxWidth, line.lineLength);
        
        
        for(int i = 0; i < line.chars.size(); i++)
        {
			int indexToUse = i;
			if(reversed)
			{
				indexToUse = line.chars.size() - i - 1;
			}
			const AlignedChar& alignedChar = line.chars[indexToUse];
            CharDescriptor* charD = alignedChar.charD;
            
            vec2 charMin = vec2(x + charD->xOffset / scale, y - charD->h / scale - charD->yOffset / scale);
            vec2 charMax = vec2(charMin.x + charD->w / scale, y - charD->yOffset / scale);
            
			if(isModel)
			{
				ModelFontVert bl = {{charMin.x, charMin.y}, {advX*charD->x, (advY*(charD->y+charD->h))}, alignedChar.material };
				ModelFontVert tl = {{charMin.x, charMax.y}, {advX*charD->x,  (advY*charD->y)}, alignedChar.material};
				ModelFontVert tr = {{charMax.x, charMax.y}, {advX*(charD->x+charD->w),(advY*charD->y)}, alignedChar.material};
				ModelFontVert br = {{charMax.x, charMin.y}, {advX*(charD->x+charD->w),(advY*(charD->y+charD->h))}, alignedChar.material };

				result.modelFontVerts.push_back(bl);
				result.modelFontVerts.push_back(tl);
				result.modelFontVerts.push_back(tr);
				result.modelFontVerts.push_back(tr);
				result.modelFontVerts.push_back(br);
				result.modelFontVerts.push_back(bl);
			}
			else
			{
            
				FontVert bl = {{charMin.x, charMin.y}, {advX*charD->x, (advY*(charD->y+charD->h))}, alignedChar.color };
				FontVert tl = {{charMin.x, charMax.y}, {advX*charD->x,  (advY*charD->y)}, alignedChar.color};
				FontVert tr = {{charMax.x, charMax.y}, {advX*(charD->x+charD->w),(advY*charD->y)}, alignedChar.color};
				FontVert br = {{charMax.x, charMin.y}, {advX*(charD->x+charD->w),(advY*(charD->y+charD->h))}, alignedChar.color };
            
				result.fontVerts.push_back(bl);
				result.fontVerts.push_back(tl);
				result.fontVerts.push_back(tr);
				result.fontVerts.push_back(tr);
				result.fontVerts.push_back(br);
				result.fontVerts.push_back(bl);
			}
            
            x += alignedChar.xAdvance;
        }
        
        y -= lineHeight / scale;
    }
    
    if(enclosingRect)
    {
        enclosingRect->origin.x = minX;
        enclosingRect->origin.y = y;
        enclosingRect->size.x = maxWidth;
        enclosingRect->size.y = -y;
    }
    
    return result;
}

std::vector<FontLine> MJFont::getLines(std::vector<AttributedText>& attributedText, 
	int alignment,
	int wrapWidth,
	float scale)
{
	std::vector<FontLine> lines;
	FontLine line;
	line.lineLength = 0.0f;
	lines.push_back(line);

	int lineIndex = 0;

	bool wordIsTooLongForLine = false;
	for(int groupBase = 0; groupBase < attributedText.size(); groupBase++)
	{

		int group = groupBase;
		int nextGroup = group + 1;
		if(reversed)
		{
			group = attributedText.size() - groupBase - 1;
			nextGroup = group - 1;
		}

		std::string& text = attributedText[group].text;


		auto start_it = text.begin();
		auto end_it = utf8::find_invalid(text.begin(), text.end());
		if (end_it != text.end()) {
			MJWarn("invalid utf8 string:%s", text.c_str());
		}

		for(auto it = start_it; it != end_it; )
		{
			auto thisIt = it;
			uint32_t textCode = utf8::next(it, end_it);

			FontLine* line = &(lines[lineIndex]);

			bool skipTrailingSpace = false;

			if(textCode == '\n')
			{
				lineIndex++;
			}
			else
			{
				if(wrapWidth > 0)
				{
					if(wordIsTooLongForLine)
					{
						CharDescriptor* nextChar = &chars[textCode];
						if(line->lineLength + ((float)(nextChar->xAdvance)) / scale >= wrapWidth)
						{
							wordIsTooLongForLine = false;
							lineIndex++;
							if(textCode == ' ')
							{
								skipTrailingSpace = true;
							}
						}
					}
					else
					{
						float wordWidth = 0;
						bool first = true;
						for(auto otherIt = thisIt; otherIt != end_it;)
						{
							uint32_t otherTextCode = utf8::next(otherIt, end_it);
							if((otherTextCode == ' ' && !first) || otherTextCode == '\n')
							{
								break;
							}
							else
							{
								CharDescriptor* nextChar = &chars[otherTextCode];
								wordWidth += ((float)(nextChar->xAdvance)) / scale;
							}
							first = false;
						}

						if(line->lineLength + wordWidth >= wrapWidth)
						{
							if(line->chars.size() <= 1)
							{
								wordIsTooLongForLine = true;
							}
							else
							{
								lineIndex++;
								if(textCode == ' ')
								{
									skipTrailingSpace = true;
								}
							}
						}
					}
				}
			}

			if(lines.size() == lineIndex)
			{
				FontLine newLine;
				newLine.lineLength = 0.0f;
				lines.push_back(newLine);
			}

			if(chars.count(textCode) != 0 && !skipTrailingSpace)
			{
				line = &(lines[lineIndex]);

				CharDescriptor* charD = &chars[textCode];
				AlignedChar alignedChar;
				alignedChar.charD = charD;

				alignedChar.xAdvance = charD->xAdvance / scale;

				alignedChar.color = attributedText[group].color;

				if(text.size() > 1 && it != end_it)
				{
					alignedChar.xAdvance += ((float)getKerningPair(textCode,utf8::peek_next(it, end_it))) / scale;
				}
				else if(nextGroup < attributedText.size() && nextGroup >= 0 && attributedText[nextGroup].text.size() > 0)
				{
					std::string& nextText = attributedText[nextGroup].text;
					uint32_t nextCode = utf8::peek_next(nextText.begin(), nextText.end());
					alignedChar.xAdvance += ((float)getKerningPair(textCode, nextCode)) / scale;
				}

				line->lineLength += alignedChar.xAdvance;
				line->chars.push_back(alignedChar);
			}
			else
			{
				line = &(lines[lineIndex]);
				AlignedChar alignedChar;
				alignedChar.charD = nullptr;
				alignedChar.xAdvance = 0;


				if(text.size() > 1 && it != end_it)
				{
					alignedChar.xAdvance += ((float)getKerningPair(textCode,utf8::peek_next(it, end_it))) / scale;
				}
				else if(nextGroup < attributedText.size() && nextGroup >= 0 && attributedText[nextGroup].text.size() > 0)
				{
					std::string& nextText = attributedText[nextGroup].text;
					uint32_t nextCode = utf8::peek_next(nextText.begin(), nextText.end());
					alignedChar.xAdvance += ((float)getKerningPair(textCode, nextCode)) / scale;
				}

				line->lineLength += alignedChar.xAdvance;
				line->chars.push_back(alignedChar);
			}
		}
	}

	return lines;
}

irect MJFont::calculateEnclosingRect(std::vector<AttributedText>& attributedText, 
    int alignment,
    int wrapWidth,
    float scale)
{
	std::vector<FontLine> lines = getLines(attributedText,  alignment, wrapWidth, scale);

    float minX = FLT_MAX;
    float maxWidth = 0.0f;

    float y = 0.0f;
    for(const FontLine& line : lines)
    {
        float x = 0.0f;
        if(alignment == MJHorizontalAlignmentCenter)
        {
            x = floorf(line.lineLength * -0.5);
        }
        else if(alignment == MJHorizontalAlignmentRight)
        {
            x = line.lineLength * -1.0;
        }

        minX = MIN(minX, x);
        maxWidth = MAX(maxWidth, line.lineLength);

        y -= lineHeight / scale;
    }

	irect enclosingRect;

    enclosingRect.origin.x = minX;
    enclosingRect.origin.y = y;
    enclosingRect.size.x = maxWidth;
    enclosingRect.size.y = -y;

    return enclosingRect;
}


irect MJFont::calculateRectOfCharAtIndex(std::vector<AttributedText>& attributedText, 
	int charIndex_,
	int alignment,
	int wrapWidth,
	float scale)
{
	int indexToUse = charIndex_;

	std::vector<FontLine> lines = getLines(attributedText,  alignment, wrapWidth, scale);

	int charI = 0;

	float y = 0.0f;
	int lineIndex = 0;
	for(const FontLine& line : lines)
	{
		float x = 0.0f;
		if(alignment == MJHorizontalAlignmentCenter)
		{
			x = floorf(line.lineLength * -0.5);
		}
		else if(alignment == MJHorizontalAlignmentRight)
		{
			x = line.lineLength * -1.0;
		}

		int charCount = 0;
		for(const AlignedChar& alignedChar : line.chars)
		{
			if(charI == indexToUse || ((lineIndex == (lines.size() - 1)) && (charCount == (line.chars.size() - 1))))
			{
				CharDescriptor* charD = alignedChar.charD;

				float charWidth = 0;
				if(charD)
				{
					charWidth = charD->xAdvance;
				}

				irect enclosingRect;

				enclosingRect.origin.x = x;
				enclosingRect.origin.y = y;
				enclosingRect.size.x = charWidth / scale;
				enclosingRect.size.y = lineHeight / scale;

				return enclosingRect;
			}

			x += alignedChar.xAdvance;
			charI++;
			charCount++;
		}

		y -= lineHeight / scale;
		lineIndex++;
	}

	return irect();
}


int MJFont::calculateIndexOfCharAtPos(std::vector<AttributedText>& attributedText,
    dvec2 pos,
    int alignment,
    int wrapWidth,
    float scale)
{
    std::vector<FontLine> lines = getLines(attributedText,  alignment, wrapWidth, scale);

   // MJLog("calculateIndexOfCharAtPos:(%.2f, %.2f)", pos.x, pos.y)
    int charI = 0;

    float y = 0.0f;
    int lineIndex = 0;
    int minCharIndex = 0;
    for(const FontLine& line : lines)
    {
        if(y - lineHeight < pos.y)
        {
            float x = 0.0f;
            if(alignment == MJHorizontalAlignmentCenter)
            {
                x = floorf(line.lineLength * -0.5);
            }
            else if(alignment == MJHorizontalAlignmentRight)
            {
                x = line.lineLength * -1.0;
            }

            //int charCount = 0;
            //for(const AlignedChar& alignedChar : line.chars)
            for(int i = 0; i < line.chars.size(); i++)
            {
                const AlignedChar& thisChar = line.chars[i];
                
                CharDescriptor* charD = thisChar.charD;

                float charWidth = 0;
                if(charD)
                {
                    charWidth = charD->xAdvance;
                }
                
                if(x + charWidth * 0.5 > pos.x - 2) //-2 seems to be required, can't find a bug, so let's offset for now
                {
                    //MJLog("char x:%.1f advance:%.1f charWidth:%.1f i:%d charI:%d", x, thisChar.xAdvance, charWidth, i, charI)
                    return max(charI,minCharIndex);
                }
                x += thisChar.xAdvance;
                charI++;
            }
            
            return charI;
        }
        
        charI += line.chars.size();
        minCharIndex = charI + 1;
        
        y -= lineHeight / scale;
        lineIndex++;

    }

    return charI;
}
