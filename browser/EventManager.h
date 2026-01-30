//
//  EventManager.hpp
//  World
//
//  Created by David Frampton on 20/10/17.
//  Copyright © 2017 Majic Jungle. All rights reserved.
//

#ifndef EventManager_hpp
#define EventManager_hpp


#include "GameConstants.h"
#include "WindowInfo.h"
#include <functional>

class MainController;
class Timer;
class TuiFunction;
class TuiTable;

#define MJ_KEY_MOD_SHIFT 1
#define MJ_KEY_MOD_CTRL 2
#define MJ_KEY_MOD_ALT 3
#define MJ_KEY_MOD_CMD 4

class EventManager {
    
public:
    
public:
    
    static EventManager* getInstance() {
        static EventManager* instance = new EventManager();
        return instance;
    }
    
    EventManager();
    ~EventManager();
    
    void init(MainController* mainController_,
              SDL_Window* window_,
              WindowInfo* windowInfo_);

    dvec2 getMouseScreenFractionNonClamped();
    
    //void setUpLuaEnvironment();
    void runEventLoop();
    void idle();
    void handleEvent(SDL_Event* event);

	void doCPUWork();
    
    void setMouseHidden(bool mouseHidden_);
    bool getMouseHidden() const;
    
    void warpMouse(dvec2 pos);
    
    void preventMouseWarpUntilAfterNextShow();
    
    void startTextEntry();
    void stopTextEntry();

	std::string getClipboardText();
	void setClipboardText(std::string);
    
    int addAnyKeyChangedListener(TuiFunction* listenerKeyChangedFunc_);
    int addSpecificKeyChangedListener(int keyCode, TuiFunction* listenerKeyChangedFunc_);
    void removeKeyChangedListener(int index);
    
    void setTextEntryListener(TuiFunction* textEntryListener_, TuiFunction* listenerKeyChangedFunc_);
    void removeTextEntryListener();
    
    void bindTui(TuiTable* rootTable);

	int getModKey();
	int getSecondModKey();

	Timer* debugTimer;

public:

	dvec2 mouseLoc;
	dvec2 mouseHideLoc = dvec2(0.0,0.0);

private:
    MainController* mainController;
    WindowInfo* windowInfo;
	SDL_Window* window;
    
    SDL_TimerID sdlTimer;
    Timer* timer;
    
    bool mouseHidden;
    bool textEntryActive;
    bool needsToStartTextEntry;
    bool needsToFinishTextEntry;
    
    bool runLoopRunning;

	double smoothedTimeStep;
	double accumulatedTimeStepError = 0.0;
    double accumulator = 0.0;

	bool needsToExit = false;
    bool shouldPreventMouseWarpUntilAfterNextShow = false;
    //dvec2 mouseLocOnLastHide;

	dvec2 mouseMovementAccumulation = dvec2(0.0);
	bool mouseMoved = false;
	bool needsToUpdateSDLMouseHidden = false;
	bool needsToResetMousePosition = false;
    
    bool hasTextEntryListener = false;
    TuiFunction* textEntryListener;
    TuiFunction* listenerKeyChangedFunc;
    
    int keyChangedListenerFunctionIndex = 0;
    std::map<int,TuiFunction*> anyKeyChangedListenerFunctions;
    std::map<int,std::map<int, TuiFunction*>> keyChangedByKeyListenerFunctions;

    
private:
	void checkEvents();
    void update(float dt);

	void createControllerSets();
};

#endif /* EventManager_hpp */
