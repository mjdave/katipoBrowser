
#include "MJSong.h"
#include "MJAudio.h"

MJSong::MJSong(MJAudio* audio_,
               std::string filePath,
               bool loadInBackground)
: MJSound(audio_)
{
	debugName = filePath;
    /*FMOD_RESULT result;
    
    FMOD_MODE flags = FMOD_2D | FMOD_CREATESTREAM;
    if(loadInBackground)
    {
        flags |= FMOD_NONBLOCKING;
    }


	//MJLog("load sound:%s", filePath.c_str());
    
    result = audio->system->createSound(filePath.c_str(), flags, 0, &sound);
    ERRCHECK(result);*/
}

MJSong::~MJSong()
{
	audio->songWillBeDestroyed(this);
}

void MJSong::play(double volume_)
{
	if(!playing() || fadingOut)
	{
		audio->queueSong(this, volume_);
	}
}

void MJSong::playInternal(double volume)
{
	/*FMOD_RESULT result;

	stop();
	fadingOut = false;

	FMOD::Channel* channel;

	result = audio->system->playSound(sound, 0, true, &channel);
	ERRCHECK(result);

	playCommon(channel, volume, 1.0);*/
}

void MJSong::fadeOut()
{
	if(fadingOut)
	{
		return;
	}
	if(audio->overallVolume * audio->musicVolume < 0.001)
	{
		stop();
		return;
	}
	/*for(auto& idAndChannel : channels)
	{
		if(playing(idAndChannel.first))
		{
			fadingOut = true;
			FMOD::Channel* channel = idAndChannel.second;
			unsigned long long parentclock;
			channel->getDSPClock(nullptr, &parentclock);
			unsigned long long clockEnd = parentclock + 44100 * 20;
			channel->addFadePoint(parentclock, 1.0);
			channel->addFadePoint(clockEnd, 0.0f);

			channel->setDelay(parentclock, clockEnd);
		}
	}*/
}

float MJSong::getCombinedVolume(uint32_t channelID)
{
    return audio->overallVolume * audio->musicVolume;// * channelInfos[channelID].volume;
}
