
#include "MJSound3D.h"
#include "MJAudio.h"


MJSound3D::MJSound3D(MJAudio* audio_,
                     std::string filePath,
                     bool loadInBackground)
: MJSound(audio_)
{
	debugName = filePath;
    /*FMOD_RESULT result;
    
    FMOD_MODE flags = FMOD_3D;
    if(loadInBackground)
    {
        flags |= FMOD_NONBLOCKING;
    }
    
    result = audio->system->createSound(filePath.c_str(), flags, 0, &sound);
    ERRCHECK(result);
    if (result != FMOD_OK) 
    {
        MJLog("Failed to load sound3D at path:%s", filePath.c_str());
    }*/
}


MJSound3D::~MJSound3D()
{
    audio->sound3DWillBeDestroyed(this);
}

void MJSound3D::cleanupChannel(uint32_t uniqueID)
{
    //MJSound::cleanupChannel(uniqueID);
    positions.erase(uniqueID);
}


uint32_t MJSound3D::play(dvec3 pos, double volume, double pitch)
{
	/*if(!looping && !completionLuaFunction && audio->overallVolume * audio->soundVolume < 0.001)
	{
		return 0;
	}
    FMOD_RESULT result;
    FMOD::Channel    *channel;

    result = audio->system->playSound(sound, 0, true, &channel);
    ERRCHECK(result);
	if(result != FMOD_OK) 
	{
		return 0;
	}
    
    FMOD_VECTOR posVec = tofmodVec(audio->worldPosToLocal(pos));
    result = channel->set3DAttributes(&posVec, nullptr);
    ERRCHECK(result);
    
    uint32_t channelID = playCommon(channel, volume * 2.0, pitch);
    
    positions[channelID] = pos;
    
    return channelID;*/
    MJError("todo");
    return 0;
}

void MJSound3D::setPos(dvec3 pos)
{
	/*if(!looping && !completionLuaFunction && audio->overallVolume * audio->soundVolume < 0.001)
	{
		return;
	}

	for(auto& idAndChannel : channels)
	{
		uint32_t channelID = idAndChannel.first;
		if(channelID != 0 && positions.count(channelID) != 0)
		{
			positions[channelID] = pos;
			FMOD_VECTOR posVec = tofmodVec(audio->worldPosToLocal(pos));
			idAndChannel.second->set3DAttributes(&posVec, nullptr);
		}
	}*/
    
    MJError("todo");
}

void MJSound3D::originShifted()
{
    /*FMOD_RESULT result;
    
    for(auto& idAndChannel : channels)
    {
        uint32_t channelID = idAndChannel.first;
        if(channelID != 0 && positions.count(channelID) != 0)
        {
            FMOD_VECTOR posVec = tofmodVec(audio->worldPosToLocal(positions[channelID]));
            result = idAndChannel.second->set3DAttributes(&posVec, nullptr);
        }
    }*/
    
    MJError("todo");
}

float MJSound3D::getCombinedVolume(uint32_t channelID)
{
    return audio->overallVolume * audio->soundVolume;// * channelInfos[channelID].volume;
}
