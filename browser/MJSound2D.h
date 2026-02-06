
#ifndef MJSound2D_h
#define MJSound2D_h

#include "MJSound.h"

class MJSound2D : public MJSound {

public: // members
    
protected: // members

public: // functions
    MJSound2D(MJAudio* audio_,
              std::string filePath,
              bool loadInBackground);
    ~MJSound2D();
    
    uint32_t play(double volume, double pitch);

protected: // functions
    virtual float getCombinedVolume(uint32_t channelID = 0);

};

#endif /* MJSound_h */
