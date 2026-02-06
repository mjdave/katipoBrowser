
#include "MJTimer.h"

#include <set>

MJTimer::MJTimer()
{
    timerIDCounter = 0;
}


MJTimer::~MJTimer()
{
    for(auto& idAndTimer : callbackTimers)
    {
        delete idAndTimer.second;
    }
    for(auto& idAndTimer : deltaTimers)
    {
        delete idAndTimer.second;
    }
}

void MJTimer::update(float dt)
{
    std::set<uint32_t> removeTimers;
    for(auto& idAndTimer : callbackTimers)
    {
        CallbackTimer* timer = idAndTimer.second;
        timer->delay -= dt;
        if(timer->delay <= 0.0f)
        {
            timer->func(idAndTimer.first, timer->initialDelay - timer->delay);
            if(timer->recurring)
            {
                timer->delay += timer->initialDelay;
                if(timer->delay <= 0.0f)
                {
                    timer->delay = timer->initialDelay;
                }
            }
            else
            {
                removeTimers.insert(idAndTimer.first);
            }
        }
    }
    
	for(auto& idAndTimer : updateTimers)
	{
        idAndTimer.second(idAndTimer.first, dt);
	}
    
    for(uint32_t timerID : removeTimers)
    {
        removeTimer(timerID);
    }
}

uint32_t MJTimer::addCallbackTimer(double delay, bool recurring, std::function<void(uint32_t timerID, float dt)> func)
{
    timerIDCounter = timerIDCounter + 1;
    uint32_t timerID = timerIDCounter;
    
    CallbackTimer* timer = new CallbackTimer();
    timer->func = func;
    timer->delay = delay;
    timer->initialDelay = delay;
    timer->recurring = recurring;
    
    callbackTimers[timerID] = timer;
    
    return timerID;
}

uint32_t MJTimer::addUpdateTimer(std::function<void(uint32_t timerID, float dt)> func)
{
	timerIDCounter = timerIDCounter + 1;
	uint32_t timerID = timerIDCounter;

	updateTimers[timerID] = func;

	return timerID;
}


uint32_t MJTimer::addDeltaTimer()
{
    timerIDCounter = timerIDCounter + 1;
    uint32_t timerID = timerIDCounter;
    
    DeltaTimer* timer = new DeltaTimer();
    timer->startTime = timer->lastTime = std::chrono::steady_clock::now();
    deltaTimers[timerID] = timer;

    return timerID;
}


double MJTimer::getDt(uint32_t timerID)
{
    if(deltaTimers.count(timerID) != 0)
    {
        DeltaTimer* timer = deltaTimers[timerID];
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::duration<double> dt = now-timer->lastTime;
        timer->lastTime = now;
        
        return dt.count();
    }
    
    return 0.0;
}

double MJTimer::getElapsed(uint32_t timerID)
{
    if(deltaTimers.count(timerID) != 0)
    {
        DeltaTimer* timer = deltaTimers[timerID];
        
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::duration<double> dt = now-timer->startTime;
        
        return dt.count();
    }
    
    return 0.0;
}


void MJTimer::removeTimer(uint32_t timerID)
{
    if(callbackTimers.count(timerID) != 0)
    {
        CallbackTimer* timer = callbackTimers[timerID];
        delete timer;
        callbackTimers.erase(timerID);
    }
	else if(updateTimers.count(timerID) != 0)
	{
		updateTimers.erase(timerID);
	}
    else if(deltaTimers.count(timerID) != 0)
    {
        delete deltaTimers[timerID];
        deltaTimers.erase(timerID);
    }
}
