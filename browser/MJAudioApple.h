//
//  MJAudio.h
//  Ambience
//
//  Created by David Frampton on 8/06/18.
//Copyright © 2018 Majic Jungle. All rights reserved.
//

#ifndef MJAudioApple_h
#define MJAudioApple_h

#include "MathUtils.h"
class TuiFunction;
class TuiTable;

class MJAudioApple {
public: // members
    
    static MJAudioApple* getInstance() {
        static MJAudioApple* instance = new MJAudioApple();
        return instance;
    }
    
    MJAudioApple();
    ~MJAudioApple();
    
    
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

#endif /* MJAudio_h */
