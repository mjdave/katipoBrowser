//todo this class is a stub

#ifndef MJSong_h
#define MJSong_h

#include "MJSound.h"
#include "MathUtils.h"

class MJSong : public MJSound {

public: // members
	bool fadingOut = false;
    
protected: // members

public: // functions
    MJSong(MJAudio* audio_,
           std::string filePath,
           bool loadInBackground);
    ~MJSong();
    
    void play(double volume);
	void playInternal(double volume);
	void fadeOut();

protected: // functions
    virtual float getCombinedVolume(uint32_t channelID = 0);
    
};

#endif /* MJSong_h */
