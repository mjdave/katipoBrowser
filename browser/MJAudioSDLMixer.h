//todo very much a WIP
#ifndef MJAudioSDLMixer_h
#define MJAudioSDLMixer_h

#include "MathUtils.h"
#include "SDL.h"

#include <map>
#include <vector>
#include <string>

class TuiFunction;
class TuiTable;
struct MIX_Mixer;
struct MIX_Track;
struct MIX_Audio;

struct MJSDLSound {
    std::vector<MIX_Track*> tracks;
    MIX_Audio* audio = nullptr;
};

class MJAudioSDLMixer {
public: // members

    SDL_AudioDeviceID audioDeviceId;
    MIX_Mixer* mixer;
    MIX_Track* currentTrack = nullptr;
    MIX_Audio* currentAudio = nullptr;
    double currentTrackDuration_ = 0.0;
    double playbackRate = 1.0;
    
    std::map<std::string, MJSDLSound> sounds; //todo clean these up, maybe reference count or something

    TuiTable* playQueue = nullptr;
    uint32_t songIndex = 0;

    static MJAudioSDLMixer* getInstance() {
        static MJAudioSDLMixer* instance = new MJAudioSDLMixer();
        return instance;
    }
    
    MJAudioSDLMixer();
    ~MJAudioSDLMixer();

    void updateInfo();
    
    void updatePausedState();
    
    void playSound(const std::string& soundURL, double volume = 1.0, double pitch = 1.0); //for one shot sounds, play simulaneous over music
    
    void playSongs(TuiTable* urls); //for a queue of songs
    void skipToNextTrack();
    void stop();

    void trackFinshedCallback();
    
    void updateCurrentlyPlayingOSInfo(const std::string& titleString, const std::string& artistString, double trackDuration, double elapsedPlaybackTime, void* imageBytes, int imageLength);

    double currentTrackTime();
    double currentTrackDuration();
    std::string currentTrackPath();

    void seekToTime(double timeSeconds);
    void setPlaybackRate(double rate);
    
public:
    //TuiFunction* finishedCallbackFunction = nullptr;
    
public:
   // void audioPlayerDidFinishPlaying(bool success);
    
};

#endif /* MJAudioSDLMixer_h */
