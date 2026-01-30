//
//  MJSound.cpp
//  Ambience
//
//  Created by David Frampton on 8/06/18.
//Copyright © 2018 Majic Jungle. All rights reserved.
//

#include "MJSound2D.h"
#include "MJAudio.h"


MJSound2D::MJSound2D(MJAudio* audio_,
                     std::string filePath,
                     bool loadInBackground)
: MJSound(audio_)
{
	debugName = filePath;
   
    MJError("todo");
    /* FMOD_RESULT result;
    
    
    FMOD_MODE flags = FMOD_2D;
    if(loadInBackground)
    {
        flags |= FMOD_NONBLOCKING;
    }
    
    result = audio->system->createSound(filePath.c_str(), flags, 0, &sound);
    ERRCHECK(result);

    if (result != FMOD_OK) 
    {
        MJLog("Failed to load sound2D at path:%s", filePath.c_str());
    }*/
}


MJSound2D::~MJSound2D()
{
    audio->sound2DWillBeDestroyed(this);
}


uint32_t MJSound2D::play(double volume, double pitch)
{
	/*if(!looping && !completionLuaFunction && audio->overallVolume * audio->soundVolume < 0.001)
	{
		return 0;
	}
    FMOD_RESULT result;
    
    FMOD::Channel* channel;
    
    result = audio->system->playSound(sound, 0, true, &channel);
    ERRCHECK(result);
    
    return playCommon(channel, volume, pitch);*/
    MJError("todo");
    return 0;
}

float MJSound2D::getCombinedVolume(uint32_t channelID)
{
    return audio->overallVolume * audio->soundVolume;// * channelInfos[channelID].volume;
}
