//
//  MJSound.h
//  Ambience
//
//  Created by David Frampton on 8/06/18.
//Copyright © 2018 Majic Jungle. All rights reserved.
//

#ifndef MJSound3D_h
#define MJSound3D_h

#include "MJSound.h"
#include "MathUtils.h"
#include <map>

class MJSound3D : public MJSound {

public: // members
    std::map <uint32_t, dvec3> positions;
    
protected: // members

public: // functions
    MJSound3D(MJAudio* audio_,
              std::string filePath,
              bool loadInBackground);
    ~MJSound3D();
    
    uint32_t play(dvec3 pos, double volume, double pitch);
	void setPos(dvec3 pos);
    
    void originShifted();

protected: // functions
    virtual float getCombinedVolume(uint32_t channelID = 0);
    virtual void cleanupChannel(uint32_t uniqueID);

};

#endif /* MJSound_h */
