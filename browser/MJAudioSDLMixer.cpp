
#include "MJAudioSDLMixer.h"
#include "TuiScript.h"

#include "SDL_mixer.h"
static const SDL_AudioSpec audioSpec = {SDL_AUDIO_S16, 2, 44100};

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

void MJAudioSDLMixer::play(TuiTable* urls)
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
        MIX_StopAllTracks(mixer, 100);
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
    currentAudio = MIX_LoadAudio_IO(mixer, stream, false, false);

    if (!MIX_SetTrackAudio(currentTrack, currentAudio)) {
        MJError("Could not set track io stream, %s", SDL_GetError()); //todo cleanup
        return;
    }

    if(!MIX_PlayTrack(currentTrack, 0)) {
        MJError("Could not play track, %s", SDL_GetError()); //todo cleanup
        return;
    }
}

void MJAudioSDLMixer::stop()
{
    MIX_PauseAllTracks(mixer);
}


void MJAudioSDLMixer::skipToNextTrack()
{
    if(currentTrack)
    {
        MIX_StopAllTracks(mixer, 100);
        MIX_DestroyTrack(currentTrack);
        MIX_DestroyAudio(currentAudio);
        currentTrack = nullptr;
    }

    if(playQueue && !playQueue->arrayObjects.empty())
    {
        songIndex = (songIndex + 1) % playQueue->arrayObjects.size();

        std::string urlString = playQueue->arrayObjects[songIndex]->getStringValue();
        SDL_IOStream* stream = SDL_IOFromFile(urlString.c_str(), "r");
        if(!stream) {
            MJError("Could not load stream %s. error: %s", urlString.c_str(), SDL_GetError());
            return;
        }

        currentTrack = MIX_CreateTrack(mixer);
        currentAudio = MIX_LoadAudio_IO(mixer, stream, false, false);

        if(!MIX_SetTrackAudio(currentTrack, currentAudio)) {
            MJError("Could not set track io stream, %s", SDL_GetError()); //todo cleanup
            return;
        }

        if(!MIX_PlayTrack(currentTrack, 0)) {
            MJError("Could not play track, %s", SDL_GetError()); //todo cleanup
            return;
        }
    }
}

void MJAudioSDLMixer::updatePausedState()
{
    //this function needs to update any OS level playback UI, eg. 'now playing' widgets/menues. on Apple this updates MPNowPlayingInfoCenter
    MJError("MJAudioSDLMixer::updatePausedState not implemented");
}

void MJAudioSDLMixer::updateCurrentlyPlayingOSInfo(const std::string& titleString, const std::string& artistString, double trackDuration, double elapsedPlaybackTime, void* imageBytes, int imageLength)
{
    //this function needs to update any OS level playback UI, eg. 'now playing' widgets/menues. on Apple this updates MPNowPlayingInfoCenter
    MJError("MJAudioSDLMixer::updateCurrentlyPlayingOSInfo not implemented");
}
