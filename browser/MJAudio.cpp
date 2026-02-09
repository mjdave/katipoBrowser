
#include "MJAudio.h"
#include "MJSound2D.h"
#include "MJSound3D.h"
#include "MJSong.h"
#include "TuiScript.h"
#include "KatipoUtilities.h"

#ifdef __APPLE__
#include "MJAudioApple.h"
#else
#include "MJAudioSDLMixer.h"
#endif


MJAudio::MJAudio()
{
    overallVolume = 1.0;
    musicVolume = 1.0;
    soundVolume = 1.0;
}


MJAudio::~MJAudio()
{
    emptyCache(false);
    //system->close();
}


void MJAudio::bindTui(TuiTable* rootTable)
{
    
    TuiTable* audioTable = new TuiTable(rootTable);
    rootTable->setTable("audio", audioTable);
    audioTable->release();
    
    audioTable->onSet = [this](TuiRef* table, const std::string& key, TuiRef* value) {
        audioTableKeyChanged(key, value);
    };
    
    /*serverTable->onSet = [this](TuiRef* table, const std::string& key, TuiRef* value) {
        if(key == "clientConnected")
        {
            if(value->type() == Tui_ref_type_FUNCTION)
            {
                clientConnectedFunction = (TuiFunction*)value;
            }
            else if(value->type() == Tui_ref_type_NIL)
            {
                clientConnectedFunction = nullptr;
            }
        }
    };*/
    
    audioTable->setFunction("stop", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#ifdef __APPLE__
        MJAudioApple::getInstance()->stop();
#else
        MJAudioSDLMixer::getInstance()->stop();
#endif
        return TUI_NIL;
    });
    
    
    audioTable->setFunction("play", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#ifdef __APPLE__
        MJAudioApple::getInstance()->play(nullptr);
#else
        MJAudioSDLMixer::getInstance()->play(nullptr);
#endif
        return TUI_NIL;
    });
    
    
    audioTable->setFunction("next", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        //MIX_StopAllTracks(mixer, 1000);
#ifdef __APPLE__
        MJAudioApple::getInstance()->skipToNextTrack();
#else
        MJAudioSDLMixer::getInstance()->skipToNextTrack();
#endif
        
        return TUI_NIL;
    });
    
    audioTable->setFunction("queueSongs", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* arrayRef = args->arrayObjects[0];
            if(arrayRef->type() == Tui_ref_type_TABLE)
            {
#ifdef __APPLE__
                MJAudioApple::getInstance()->play((TuiTable*)arrayRef);
#else
                MJAudioSDLMixer::getInstance()->play((TuiTable*)arrayRef);
#endif
                MJLog("playing song");
            }
            else
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "Incorrect argument type");
            }
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "Missing args");
        }
        return TUI_NIL;
    });
}

void MJAudio::audioTableKeyChanged(const std::string& key, TuiRef* value)
{
    if(key == "playingSongChanged")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(playingSongChangedFunction)
                {
                    playingSongChangedFunction->release();
                }
                playingSongChangedFunction = (TuiFunction*)value;
                playingSongChangedFunction->retain();
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(playingSongChangedFunction)
                {
                    playingSongChangedFunction->release();
                    playingSongChangedFunction = nullptr;
                }
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
    else if(key == "playingSongPausedChanged")
    {
        switch (value->type()) {
            case Tui_ref_type_FUNCTION:
            {
                if(playingSongPausedChangedFunction)
                {
                    playingSongPausedChangedFunction->release();
                }
                playingSongPausedChangedFunction = (TuiFunction*)value;
                playingSongPausedChangedFunction->retain();
            }
                break;
            case Tui_ref_type_NIL:
            {
                if(playingSongPausedChangedFunction)
                {
                    playingSongPausedChangedFunction->release();
                    playingSongPausedChangedFunction = nullptr;
                }
            }
                break;
            default:
                MJError("Expected function");
                break;
        }
    }
}

void MJAudio::updateCurrentlyPlayingInfo(const std::string& titleString, const std::string& artistString, double duration, void* imageBytes, int imageLength)
{
    if(playingSongChangedFunction)
    {
        TuiRef* titleRef = new TuiString(titleString);
        TuiRef* artistRef = new TuiString(artistString);
        TuiRef* durationRef = new TuiNumber(duration);
        
        playingSongChangedFunction->call("updateCurrentlyPlayingTrackInfo", titleRef, artistRef, durationRef);
        
        titleRef->release();
        artistRef->release();
        durationRef->release();
    }
#ifdef __APPLE__
    MJAudioApple::getInstance()->updateCurrentlyPlayingOSInfo(titleString, artistString, duration, 0, imageBytes, imageLength);
#endif
}

void MJAudio::updatePausedState(bool pauseSate)
{
    if(playingSongPausedChangedFunction)
    {
        playingSongPausedChangedFunction->call("updatePausedState", TUI_BOOL(pauseSate));
    }
}
 
void MJAudio::emptyCache(bool fadeOut)
{
    for(auto const& iter : sounds2D)
    {
        delete iter.second;
    }
    sounds2D.clear();
    for(auto const& iter : sounds3D)
    {
        delete iter.second;
    }
    sounds3D.clear();


	//for(auto const& iter : songs)
	{
		//iter.second->removeCompletionLuaFunction();
		/*if(fadeOut)
		{
			iter.second->fadeOut();
		}*/
	}
}

void MJAudio::songWillBeDestroyed(MJSong* song)
{
    pausedSongsDueToAppLosingFocus.erase(song);
}

void MJAudio::sound2DWillBeDestroyed(MJSound2D* sound)
{
    pausedSounds2DDueToAppLosingFocus.erase(sound);
}

void MJAudio::sound3DWillBeDestroyed(MJSound3D* sound)
{
    pausedSounds3DDueToAppLosingFocus.erase(sound);
}

void MJAudio::deleteSound(MJSound* sound)
{
	for(auto const& iter : sounds2D)
	{
		if((MJSound*)iter.second == sound)
		{
			delete sound;
			sounds2D.erase(iter.first);
			return;
		}
	}
	for(auto const& iter : sounds3D)
	{
		if((MJSound*)iter.second == sound)
		{
			delete sound;
			sounds3D.erase(iter.first);
			return;
		}
	}
	for(auto const& iter : songs)
	{
		if((MJSound*)iter.second == sound)
		{
			delete sound;
			songs.erase(iter.first);
			return;
		}
	}
}

void MJAudio::appLostFocus()
{
    for(auto const& iter : songs)
    {
        MJSong* song = iter.second;
        if(song->playing() && !song->paused())
        {
            song->setPaused(true);
            pausedSongsDueToAppLosingFocus.insert(song);
        }
    }
    for(auto const& iter : sounds2D)
    {
        MJSound2D* sound = iter.second;
        if(sound->playing() && !sound->paused())
        {
            sound->setPaused(true);
            pausedSounds2DDueToAppLosingFocus.insert(sound);
        }
    }
    for(auto const& iter : sounds3D)
    {
        MJSound3D* sound = iter.second;
        if(sound->playing() && !sound->paused())
        {
            sound->setPaused(true);
            pausedSounds3DDueToAppLosingFocus.insert(sound);
        }
    }
}

void MJAudio::appGainedFocus()
{
    for(MJSong* song : pausedSongsDueToAppLosingFocus)
    {
        song->setPaused(false);
    }
    pausedSongsDueToAppLosingFocus.clear();
    for(MJSound2D* sound : pausedSounds2DDueToAppLosingFocus)
    {
        sound->setPaused(false);
    }
    pausedSounds2DDueToAppLosingFocus.clear();
    for(MJSound3D* sound : pausedSounds3DDueToAppLosingFocus)
    {
        sound->setPaused(false);
    }
    pausedSounds3DDueToAppLosingFocus.clear();
}

MJSound2D* MJAudio::loadSound2D(std::string name, bool loadInBackground)
{
    if(sounds2D.count(name) != 0)
    {
        return sounds2D[name];
    }
    MJSound2D* sound = new MJSound2D(this, name, loadInBackground);
    sounds2D[name] = sound;
    return sound;
}

MJSound3D* MJAudio::loadSound3D(std::string name, bool loadInBackground)
{
    if(sounds3D.count(name) != 0)
    {
        return sounds3D[name];
    }
    MJSound3D* sound = new MJSound3D(this, name, loadInBackground);
    sounds3D[name] = sound;
    return sound;
}


MJSong* MJAudio::loadSong(std::string name, bool loadInBackground)
{
    if(songs.count(name) != 0)
    {
        return songs[name];
    }
    MJSong* song = new MJSong(this, name, loadInBackground);
    songs[name] = song;
    return song;
}


double MJAudio::getOverallVolume() const
{
    return overallVolume;
}

void MJAudio::updateSoundVolumes()
{
    for(auto const& iter : sounds2D)
    {
        iter.second->updateVolume();
    }
    
    for(auto const& iter : sounds3D)
    {
        iter.second->updateVolume();
    }
}

void MJAudio::updateSongVolumes()
{
    
    for(auto const& iter : songs)
    {
        iter.second->updateVolume();
    }
}

void MJAudio::setOverallVolume(double overallVolume_)
{
    if(!approxEqual(overallVolume, overallVolume_))
    {
        overallVolume = overallVolume_;
        updateSoundVolumes();
        updateSongVolumes();
    }
}

double MJAudio::getMusicVolume() const
{
    return musicVolume;
}

void MJAudio::setMusicVolume(double musicVolume_)
{
    if(!approxEqual(musicVolume, musicVolume_))
    {
        musicVolume = musicVolume_;
        updateSongVolumes();
    }
}

double MJAudio::getSoundVolume() const
{
    return soundVolume;
}

void MJAudio::setSoundVolume(double soundVolume_)
{
    if(!approxEqual(soundVolume, soundVolume_))
    {
        soundVolume = soundVolume_;
        updateSoundVolumes();
    }
}

void MJAudio::update()
{
	if(hasQueuedSong)
	{
		bool foundPlaying = false;
		for(auto const& iter : songs)
		{
			MJSong* song = iter.second;
			if(song->playing())
			{
				foundPlaying = true;
				break;
			}
		}
		if(!foundPlaying)
		{
			queuedSong.song->playInternal(queuedSong.volume);
			hasQueuedSong = false;
		}
	}
}

void MJAudio::fadeOutAnyCurrentSong()
{
	for(auto const& iter : songs)
	{
		MJSong* song = iter.second;
		if(song->paused())
		{
			song->stop();
		}
		else if(song->playing())
		{
			song->fadeOut();
		}
	}

	hasQueuedSong = false;
}

void MJAudio::updatePlayerPos(dvec3 playerPos_,
                           dvec3 playerDirection_,
                           dvec3 playerUp)
{
    if(!approxEqualVec(playerPos, playerPos_) || !approxEqualVec(playerDirection, playerDirection_))
    {
        playerPos = playerPos_;
        playerDirection = playerDirection_;
        
        if(distance(playerPos_, origin) > 0.0001)
        {
            setOrigin(playerPos_);
        }
    }
}

void MJAudio::setOrigin(dvec3 origin_)
{
    origin = origin_;
    
    for(auto const& iter : sounds3D)
    {
        iter.second->originShifted();
    }
}

dvec3 MJAudio::worldPosToLocal(dvec3 worldPos)
{
    return (worldPos - origin);
}


void MJAudio::queueSong(MJSong* songToQueue, double volume)
{
	bool foundPlaying = false;
	for(auto const& iter : songs)
	{
		MJSong* song = iter.second;
		if((song->playing() || song->paused()) && songToQueue != song)
		{
			foundPlaying = true;
			song->fadeOut();
		}
	}

	if(foundPlaying)
	{
		queuedSong.song = songToQueue;
		queuedSong.volume = volume;
		hasQueuedSong = true;
	}
	else
	{
		songToQueue->playInternal(volume);
	}
}
