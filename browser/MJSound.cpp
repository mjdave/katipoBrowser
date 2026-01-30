//
//  MJSong.cpp
//  Ambience
//
//  Created by David Frampton on 8/06/18.
//Copyright © 2018 Majic Jungle. All rights reserved.
//

#include "MJSound.h"
#include "MJAudio.h"

struct ChannelUserData {
    MJSound* sound;
    uint32_t uniqueID;
};

MJSound::MJSound(MJAudio* audio_)
{
    audio = audio_;
    //channelUniqueIDCount = 0;
    looping = false;
}

MJSound::~MJSound()
{
    /*std::map<uint32_t, FMOD::Channel*> channelsCopy = channels;
    for(auto& idAndChannel : channelsCopy)
    {
        cleanupChannel(idAndChannel.first);
    }
    FMOD_RESULT result;
    result = sound->release();
    ERRCHECK(result);

	if(completionLuaFunction)
	{
		delete completionLuaFunction;
		completionLuaFunction = nullptr;
	}*/
}

/*void MJSound::cleanupChannel(uint32_t uniqueID)
{
    if(channels.count(uniqueID) != 0)
    {
        FMOD::Channel *channel = channels[uniqueID];
        void* songPtr;
        channel->getUserData(&songPtr);
        if(songPtr)
        {
            ChannelUserData* userData = (ChannelUserData*)songPtr;
            delete userData;
        }
        channel->setUserData(nullptr);
        channels.erase(uniqueID);
    }
}*/

/*
FMOD_RESULT F_CALLBACK FmodEndSoundCallback(FMOD_CHANNELCONTROL *channelcontrol,
                                            FMOD_CHANNELCONTROL_TYPE controltype,
                                            FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
                                            void *commanddata1,
                                            void *commanddata2)
{
    if(callbacktype == FMOD_CHANNELCONTROL_CALLBACK_END)
    {
        FMOD::Channel *channel = (FMOD::Channel *)channelcontrol;
        void* songPtr;
        channel->getUserData(&songPtr);
        if(songPtr)
        {
            ChannelUserData* userData = (ChannelUserData*)songPtr;
            userData->sound->channelFinishedPlaying(userData->uniqueID, channel);
        }
    }
    return FMOD_OK;
}*/

/*
uint32_t MJSound::playCommon(FMOD::Channel* channel, double volume, double pitch)
{
	if(!channel)
	{
		MJError("MJSound::playCommon no channel");
		return 0;
	}
    FMOD_RESULT result;
    
    channelUniqueIDCount++;
    uint32_t channelID = channelUniqueIDCount;
	channels[channelID] = channel;
	channelInfos[channelID].volume = volume;
    
    ChannelUserData* userData = new ChannelUserData();
    userData->sound = this;
    userData->uniqueID = channelID;
    
    result = channel->setUserData(userData);
    ERRCHECK(result);
    result = channel->setCallback(FmodEndSoundCallback);
    ERRCHECK(result);
    
    float combinedVolume = getCombinedVolume(channelID);
    result = channel->setVolume(combinedVolume);
    ERRCHECK(result);

	result = channel->setPitch(pitch);
	ERRCHECK(result);

	if(priority != 128)
	{
		result = channel->setPriority(priority);
		ERRCHECK(result);
	}

	channelInfos[channelID].pausedDueToZeroVolume = (combinedVolume < 0.001);
	bool newPaused = channelInfos[channelID].pausedDueToAPI || channelInfos[channelID].pausedDueToZeroVolume;
    result = channel->setPaused(newPaused);
    ERRCHECK(result);
    
    return channelID;
}*/

void MJSound::setPitch(double pitch, uint32_t channelID)
{
	/*FMOD_RESULT result;

	if(channelID == 0)
	{
		for(auto& idAndChannel : channels)
		{
			if(idAndChannel.first != 0)
			{
				result = idAndChannel.second->setPitch(pitch);
				ERRCHECK(result);
			}
		}
	}
	else if(channels.count(channelID) != 0)
	{
		FMOD::Channel* channel = channels[channelID];
		result = channel->setPitch(pitch);
		ERRCHECK(result);
	}*/
}

void MJSound::setPriority(int priority_, uint32_t channelID)
{
	/*priority = priority_;
	FMOD_RESULT result;

	if(channelID == 0)
	{
		for(auto& idAndChannel : channels)
		{
			if(idAndChannel.first != 0)
			{
				result = idAndChannel.second->setPriority(priority);
				ERRCHECK(result);
			}
		}
	}
	else if(channels.count(channelID) != 0)
	{
		FMOD::Channel* channel = channels[channelID];
		result = channel->setPriority(priority);
		ERRCHECK(result);
	}*/
}

void MJSound::setLowPassFilter(bool filter, double dryWetMix, uint32_t channelID)
{
	/*FMOD_RESULT result;

	if(channelID == 0)
	{
		for(auto& idAndChannel : channels)
		{
			if(idAndChannel.first != 0)
			{
				setLowPassFilter(filter, dryWetMix);
			}
		}
	}
	else if(channels.count(channelID) != 0)
	{
		FMOD::Channel* channel = channels[channelID];

		MJChannelInfo& channelInfo = channelInfos[channelID];
		if(filter)
		{
			if(!channelInfo.lowPassEffect)
			{
				result = audio->system->createDSPByType(FMOD_DSP_TYPE_LOWPASS_SIMPLE, &channelInfo.lowPassEffect);
				ERRCHECK(result);

			}

			bool active;
			result = channelInfo.lowPassEffect->getActive(&active);
			ERRCHECK(result);
			if(!active)
			{
				result = channel->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, channelInfo.lowPassEffect);
				ERRCHECK(result);
			}

			result = channelInfo.lowPassEffect->setWetDryMix(1.0,dryWetMix, 1.0 - dryWetMix);
			ERRCHECK(result);
		}
		else if(channelInfo.lowPassEffect)
		{
			bool active;
			result = channelInfo.lowPassEffect->getActive(&active);
			ERRCHECK(result);
			if(active)
			{
				result = channel->removeDSP(channelInfo.lowPassEffect);
				ERRCHECK(result);
			}
		}
	}*/
}
/*
void MJSound::channelFinishedPlaying(uint32_t uniqueID, FMOD::Channel *channel)
{
    if(channels.count(uniqueID) != 0 && channels[uniqueID] == channel)
    {
        cleanupChannel(uniqueID);
    }

		if(completionLuaFunction)
		{
			callMJLuaFunction(completionLuaFunction, uniqueID);
		}
}*/

bool MJSound::playing(uint32_t channelID)
{
    /*FMOD_RESULT result;
    
    if(channelID == 0)
    {
        for(auto& idAndChannel : channels)
        {
            if(idAndChannel.first != 0 && playing(idAndChannel.first))
            {
                return true;
            }
        }
    }
    else if(channels.count(channelID) != 0)
    {
        FMOD::Channel* channel = channels[channelID];
        bool isPlaying;
        result = channel->isPlaying(&isPlaying);
        ERRCHECK(result);
        
        return isPlaying;
    }*/
    MJError("todo")
    
    return false;
}

void MJSound::stop(uint32_t channelID)
{
    MJError("todo")
    /*FMOD_RESULT result;
    
    if(channelID == 0)
    {
        std::map<uint32_t, FMOD::Channel*> channelsCopy = channels;
        for(auto& idAndChannel : channelsCopy)
        {
            if(idAndChannel.first != 0)
            {
                stop(idAndChannel.first);
            }
        }
    }
    else if(channels.count(channelID) != 0)
    {
        FMOD::Channel* channel = channels[channelID];
        result = channel->stop();
        ERRCHECK(result);
        cleanupChannel(channelID);
    }*/
}

bool MJSound::paused(uint32_t channelID)
{
    MJError("todo")
    /*
    if(channelID == 0)
    {
        for(auto& idAndChannel : channels)
        {
            if(idAndChannel.first != 0 && paused(idAndChannel.first))
            {
                return true;
            }
        }
    }
    else if(channels.count(channelID) != 0)
    {
        return channelInfos[channelID].pausedDueToAPI;
    }*/
    
    return false;
}

void MJSound::setPaused(bool paused, uint32_t channelID)
{
    MJError("todo")
    /*
    FMOD_RESULT result;
    
    if(channelID == 0)
    {
        std::map<uint32_t, FMOD::Channel*> channelsCopy = channels;
        for(auto& idAndChannel : channels)
        {
            if(idAndChannel.first != 0)
            {
                setPaused(paused, idAndChannel.first);
            }
        }
    }
    else if(channels.count(channelID) != 0)
    {
		channelInfos[channelID].pausedDueToAPI = paused;
		bool newPaused = channelInfos[channelID].pausedDueToAPI || channelInfos[channelID].pausedDueToZeroVolume;

        FMOD::Channel* channel = channels[channelID];
        result = channel->setPaused(newPaused);
        ERRCHECK(result);
    }*/
}

void MJSound::setVolume(double volume, uint32_t channelID)
{
    MJError("todo")
    /*
	if(channelID == 0)
	{
		std::map<uint32_t, FMOD::Channel*> channelsCopy = channels;
		for(auto& idAndChannel : channels)
		{
			if(idAndChannel.first != 0)
			{
				setVolume(volume, idAndChannel.first);
			}
		}
	}
	else if(channels.count(channelID) != 0)
	{
		channelInfos[channelID].volume = volume;
		updateVolume(channelID);
	}*/
}

void MJSound::updateVolume(uint32_t channelID)
{
    MJError("todo")
    /*FMOD_RESULT result;
    
    if(channelID == 0)
    {
        std::map<uint32_t, FMOD::Channel*> channelsCopy = channels;
        for(auto& idAndChannel : channels)
        {
            if(idAndChannel.first != 0)
            {
                updateVolume(idAndChannel.first);
            }
        }
    }
    else if(channels.count(channelID) != 0)
    {
        FMOD::Channel* channel = channels[channelID];
        float combinedVolume = getCombinedVolume(channelID);
        result = channel->setVolume(combinedVolume);
		ERRCHECK(result);

		channelInfos[channelID].pausedDueToZeroVolume = (combinedVolume < 0.001);
		bool newPaused = channelInfos[channelID].pausedDueToAPI || channelInfos[channelID].pausedDueToZeroVolume;
		result = channel->setPaused(newPaused);
		ERRCHECK(result);

    }*/
}

void MJSound::setLooping(bool looping_)
{
    MJError("todo")
    /*
    FMOD_RESULT result;
    if(looping_ != looping)
    {
        looping = looping_;
        FMOD_MODE currentMode;
        result = sound->getMode(&currentMode);
        ERRCHECK(result);
        
        if(looping)
        {
            currentMode = currentMode & ~FMOD_LOOP_OFF;
            currentMode = currentMode | FMOD_LOOP_NORMAL;
        }
        else
        {
            currentMode = currentMode & ~FMOD_LOOP_NORMAL;
            currentMode = currentMode | FMOD_LOOP_OFF;
        }
        
        result = sound->setMode(currentMode);
        ERRCHECK(result);
    }*/
}

