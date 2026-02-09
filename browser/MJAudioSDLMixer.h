//todo very much a WIP
#ifndef MJAudioSDLMixer_h
#define MJAudioSDLMixer_h

#include "MathUtils.h"
#include "SDL.h"

class TuiFunction;
class TuiTable;
struct MIX_Mixer;
struct MIX_Track;
struct MIX_Audio;

class MJAudioSDLMixer {
public: // members

    SDL_AudioDeviceID audioDeviceId;
    MIX_Mixer* mixer;
    MIX_Track* currentTrack = nullptr;
    MIX_Audio* currentAudio = nullptr;

    TuiTable* playQueue = nullptr;
    uint32_t songIndex = 0;

    static MJAudioSDLMixer* getInstance() {
        static MJAudioSDLMixer* instance = new MJAudioSDLMixer();
        return instance;
    }
    
    MJAudioSDLMixer();
    ~MJAudioSDLMixer();
    
    
    void updatePausedState();
    
    void play(TuiTable* urls);
    void skipToNextTrack();
    void stop();
    
    void updateCurrentlyPlayingOSInfo(const std::string& titleString, const std::string& artistString, double trackDuration, double elapsedPlaybackTime, void* imageBytes, int imageLength);
    
public:
    //TuiFunction* finishedCallbackFunction = nullptr;
    
public:
   // void audioPlayerDidFinishPlaying(bool success);
    
};

#endif /* MJAudioSDLMixer_h */
