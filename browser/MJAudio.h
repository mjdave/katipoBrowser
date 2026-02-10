//Todo the audio system is very much a WIP
#ifndef MJAudio_h
#define MJAudio_h

#include <map>
#include <string>
#include <set>
#include "MathUtils.h"
#include "SDL.h"
#include "TuiScript.h"

class TuiTable;

class MJAudio {

public: // members
    
protected: // members
    
    TuiFunction* playingSongChangedFunction = nullptr;
    TuiFunction* playingSongPausedChangedFunction = nullptr;

public: // functions
    
    
    static MJAudio* getInstance() {
        static MJAudio* instance = new MJAudio();
        return instance;
    }
    
    MJAudio();
    ~MJAudio();
    
    void bindTui(TuiTable* rootTable);
    void audioTableKeyChanged(const std::string& key, TuiRef* value);
    void updateCurrentlyPlayingInfo(const std::string& titleString, const std::string& artistString, double duration, void* imageBytes, int imageLength);
    void updatePausedState(bool pauseSate);
    
    void appLostFocus();
    void appGainedFocus();

protected: // functions

};

#endif /* MJAudio_h */
