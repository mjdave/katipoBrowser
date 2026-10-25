//todo very much a WIP
#ifndef MJAudioApple_h
#define MJAudioApple_h

#include "MathUtils.h"
#include "TuiScript.h"

class MJAudioApple {
public: // members
    
    static MJAudioApple* getInstance() {
        static MJAudioApple* instance = new MJAudioApple();
        return instance;
    }
    
    MJAudioApple();
    ~MJAudioApple();
    
    
    void updatePausedState();
    
    void play(const tui::table& urls);
    void skipToNextTrack();
    void stop();
    
    double currentTrackTime();
    double currentTrackDuration();
    std::string currentTrackPath();
    
    void seekToTime(double seconds);
    void setPlaybackRate(double rate);
    
    void updateCurrentlyPlayingOSInfo(const std::string& titleString, const std::string& artistString, double trackDuration, double elapsedPlaybackTime, void* imageBytes, int imageLength);
    
public:
    //tui::function finishedCallbackFunction = nullptr;
    
public:
   // void audioPlayerDidFinishPlaying(bool success);
    
};

#endif /* MJAudio_h */
