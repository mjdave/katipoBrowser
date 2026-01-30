//
//  MJTimer.h
//  Ambience
//
//  Created by David Frampton on 12/06/18.
//Copyright © 2018 Majic Jungle. All rights reserved.
//

#ifndef MJTimer_h
#define MJTimer_h


#include <map>
#include <functional>
#include <cstdint>
#include <stdio.h>
#include <iostream>
#include <chrono>
#include <ctime>

struct CallbackTimer {
    std::function<void(uint32_t timerID, float dt)> func;
    float delay;
    float initialDelay;
    bool recurring;
};

struct DeltaTimer {
    std::chrono::steady_clock::time_point lastTime;
    std::chrono::steady_clock::time_point startTime;
};

class MJTimer {

public: // members
    
protected: // members
    uint32_t timerIDCounter;
    
    std::map<uint32_t, CallbackTimer*> callbackTimers;
    std::map<uint32_t, DeltaTimer*> deltaTimers;
	std::map<uint32_t, std::function<void(uint32_t timerID, float dt)>> updateTimers;

public: // functions
    
    MJTimer();
    ~MJTimer();
    
    static MJTimer* getInstance() {
        static MJTimer* instance = new MJTimer();
        return instance;
    }
    
    
    void update(float dt);
    
    uint32_t addCallbackTimer(double delay, bool recurring, std::function<void(uint32_t timerID, float dt)> func);
	uint32_t addUpdateTimer(std::function<void(uint32_t timerID, float dt)> func);
    uint32_t addDeltaTimer();
    
    void removeTimer(uint32_t timerID);
    
    double getDt(uint32_t timerID);
    double getElapsed(uint32_t timerID);
    
    

protected: // functions

};

#endif /* MJTimer_h */
