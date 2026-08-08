
#include "MJAudioSDLMixer.h"
#include "TuiScript.h"
#include "MJAudio.h"
#include "MJTimer.h"

#include "SDL_mixer.h"
static const SDL_AudioSpec audioSpec = {SDL_AUDIO_S16, 2, 44100};

void trackFinished(void* userdata, MIX_Track* track)
{
    ((MJAudioSDLMixer*)userdata)->trackFinshedCallback();
}

void MJAudioSDLMixer::trackFinshedCallback()
{
    MJTimer::getInstance()->addCallbackTimer(0.0,false,[this](uint32_t timerID, float dt) { //a simple way to delay this, as we can't actually skip the track from within the trackFinished callback
        skipToNextTrack();
    });
}

MJAudioSDLMixer::MJAudioSDLMixer()
{
    if(SDL_WasInit(SDL_INIT_AUDIO) == SDL_INIT_AUDIO) {
        SDL_Log("SDL_Audio subsystem initialized");
    }
    else if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("Could not initialize audio subsystem, %s", SDL_GetError());
        return;
    }


    audioDeviceId =
        SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audioSpec);
    if(audioDeviceId == 0) {
        SDL_Log("Could not open audio device default output %s", SDL_GetError());
        return;
    }

    MIX_Init();
    mixer = MIX_CreateMixerDevice(audioDeviceId, &audioSpec);

    SDL_Log("AudioPlayer ready");
}


MJAudioSDLMixer::~MJAudioSDLMixer()
{
    if(playQueue)
    {
        playQueue->release();
    }
}

void MJAudioSDLMixer::updateInfo()
{
    if(currentTrack && currentAudio)
    {
        SDL_PropertiesID properties = MIX_GetAudioProperties(currentAudio);
        std::string titleString = SDL_GetStringProperty(properties, MIX_PROP_METADATA_TITLE_STRING, "");
        std::string artistString = SDL_GetStringProperty(properties, MIX_PROP_METADATA_ARTIST_STRING, "");

        double duration = 0.0;
        int64_t frames = SDL_GetNumberProperty(properties, MIX_PROP_METADATA_DURATION_FRAMES_NUMBER, 0);

        if(frames > 0)
        {
            duration = MIX_AudioFramesToMS(currentAudio, frames);
            duration /= 1000;
        }

        MJAudio::getInstance()->updateCurrentlyPlayingInfo(titleString, artistString, duration, nullptr, 0);
    }
    else
    {
        MJAudio::getInstance()->updateCurrentlyPlayingInfo("", "", 0, nullptr, 0);
    }
}

void MJAudioSDLMixer::playSound(const std::string& soundURL, double volume, double pitch)
{
    MJSDLSound& mjSound = sounds[soundURL];
    
    if(!mjSound.audio)
    {
        
        mjSound.audio = MIX_LoadAudio(mixer, soundURL.c_str(), false);
        if (!mjSound.audio) {
            MJError("Could not load audio, %s", SDL_GetError()); //todo cleanup
            return;
        }
    }
    
    bool foundTrack = false;
    for(MIX_Track* track : mjSound.tracks)
    {
        if(!MIX_TrackPlaying(track))
        {
            MIX_SetTrackGain(track, volume);
            MIX_SetTrackFrequencyRatio(track, pitch);
            MIX_PlayTrack(track, 0);
            foundTrack = true;
            break;
        }
    }
    
    if(!foundTrack)
    {
        MIX_Track* track = MIX_CreateTrack(mixer);
        
        if (!MIX_SetTrackAudio(track, mjSound.audio)) {
            MJError("Could not set track io stream, %s", SDL_GetError()); //todo cleanup
            return;
        }
        
        MIX_SetTrackGain(track, volume);
        MIX_SetTrackFrequencyRatio(track, pitch);
        
        if(!MIX_PlayTrack(track, 0)) {
            MJError("Could not play track, %s", SDL_GetError()); //todo cleanup
            return;
        }
        
        mjSound.tracks.push_back(track);
    }
    
    //cachedTracks[]
}

void MJAudioSDLMixer::playSongs(TuiTable* urls)
{
    if(!urls || urls->arrayObjects.empty())
    {
        if(currentTrack)
        {
            MIX_ResumeAllTracks(mixer);
        }
        else
        {
            MJWarn("MJAudioSDLMixer given no queue to play");
        }
        return;
    }

    if(playQueue)
    {
        playQueue->release();
    }


    if(currentTrack)
    {
        MIX_StopTrack(currentTrack, 10);
        MIX_DestroyTrack(currentTrack);
        MIX_DestroyAudio(currentAudio);
        currentTrack = nullptr;
    }

    playQueue = (TuiTable*)urls->retain();
    songIndex = 0;

    std::string urlString = playQueue->arrayObjects[songIndex]->getStringValue();
    SDL_IOStream* stream = SDL_IOFromFile(urlString.c_str(), "r");
    if(!stream) {
        MJError("Could not load stream %s. error: %s", urlString.c_str(), SDL_GetError());
        return;
    }

    currentTrack = MIX_CreateTrack(mixer);
    MIX_SetTrackFrequencyRatio(currentTrack, playbackRate);
    MIX_SetTrackStoppedCallback(currentTrack, trackFinished, this);
    currentAudio = MIX_LoadAudio_IO(mixer, stream, false, false);

    SDL_CloseIO(stream);

    if (!MIX_SetTrackAudio(currentTrack, currentAudio)) {
        MJError("Could not set track io stream, %s", SDL_GetError()); //todo cleanup
        return;
    }

    Sint64 durationFrames = MIX_GetAudioDuration(currentAudio);
    double durationMS = MIX_AudioFramesToMS(currentAudio, durationFrames);

    currentTrackDuration_ = durationMS / 1000.0;
    
    if(!MIX_PlayTrack(currentTrack, 0)) {
        MJError("Could not play track, %s", SDL_GetError()); //todo cleanup
        return;
    }

    updateInfo();
}

void MJAudioSDLMixer::stop()
{
    MIX_PauseAllTracks(mixer);
}


void MJAudioSDLMixer::skipToNextTrack()
{
    if(currentTrack)
    {
        MIX_StopTrack(currentTrack, 10);
        MIX_DestroyTrack(currentTrack);
        MIX_DestroyAudio(currentAudio);
        currentTrack = nullptr;
    }

    if(playQueue && playQueue->arrayObjects.size() > songIndex + 1)
    {
        songIndex = (songIndex + 1) % playQueue->arrayObjects.size();

        std::string urlString = playQueue->arrayObjects[songIndex]->getStringValue();
        SDL_IOStream* stream = SDL_IOFromFile(urlString.c_str(), "r");
        if(!stream) {
            MJError("Could not load stream %s. error: %s", urlString.c_str(), SDL_GetError());
            return;
        }

        currentTrack = MIX_CreateTrack(mixer);
        MIX_SetTrackFrequencyRatio(currentTrack, playbackRate);
        MIX_SetTrackStoppedCallback(currentTrack, trackFinished, this);
        currentAudio = MIX_LoadAudio_IO(mixer, stream, false, false);

        if(!MIX_SetTrackAudio(currentTrack, currentAudio)) {
            MJError("Could not set track io stream, %s", SDL_GetError()); //todo cleanup
            return;
        }

        Sint64 durationFrames = MIX_GetAudioDuration(currentAudio);
        double durationMS = MIX_AudioFramesToMS(currentAudio, durationFrames);

        currentTrackDuration_ = durationMS / 1000.0;

        if(!MIX_PlayTrack(currentTrack, 0)) {
            MJError("Could not play track, %s", SDL_GetError()); //todo cleanup
            return;
        }
    }

    updateInfo();
}

void MJAudioSDLMixer::updatePausedState()
{
    MJWarn("MJAudioSDLMixer::updatePausedState not implemented");
}

void MJAudioSDLMixer::updateCurrentlyPlayingOSInfo(const std::string& titleString, const std::string& artistString, double trackDuration, double elapsedPlaybackTime, void* imageBytes, int imageLength)
{
    //this function needs to update any OS level playback UI, eg. 'now playing' widgets/menues. on Apple this updates MPNowPlayingInfoCenter
    MJWarn("MJAudioSDLMixer::updateCurrentlyPlayingOSInfo not implemented");
}

double MJAudioSDLMixer::currentTrackTime()
{
    if(currentTrack)
    {
        Sint64 posFrame = MIX_GetTrackPlaybackPosition(currentTrack);
        double ms = MIX_TrackFramesToMS(currentTrack, posFrame);
        return ms / 1000.0;
    }
    return 0.0;
}


double MJAudioSDLMixer::currentTrackDuration()
{
    if(currentTrack)
    {
        return currentTrackDuration_;
    }
    return 0.0;
}

std::string MJAudioSDLMixer::currentTrackPath()
{
    if(playQueue && playQueue->arrayObjects.size() > songIndex)
    {
        return playQueue->arrayObjects[songIndex]->getStringValue();
    }
    return "";
}

void MJAudioSDLMixer::seekToTime(double timeSeconds)
{
    if(currentTrack)
    {
        double ms = timeSeconds * 1000.0;
        Sint64 posFrame = MIX_TrackMSToFrames(currentTrack, ms);
        MIX_SetTrackPlaybackPosition(currentTrack, posFrame);
    }
}

void MJAudioSDLMixer::setPlaybackRate(double rate)
{
    playbackRate = rate;
    if(currentTrack)
    {
        MIX_SetTrackFrequencyRatio(currentTrack, playbackRate);
    }
}
