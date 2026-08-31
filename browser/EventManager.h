
#ifndef EventManager_hpp
#define EventManager_hpp

#include "WindowInfo.h"
#include "SDL.h"
#include <functional>
#include <map>

class MainController;
class Timer;
class TuiFunction;
class TuiTable;
class TuiRef;

#define MJ_KEY_MOD_SHIFT 1
#define MJ_KEY_MOD_CTRL 2
#define MJ_KEY_MOD_ALT 3
#define MJ_KEY_MOD_CMD 4

struct TuiFunctionCallData;
struct TuiDebugInfo;

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
    
    void warpMouse(dvec2 pos);
    
    void preventMouseWarpUntilAfterNextShow();
    
    void setTextEntryRect(dvec2 rectPos, dvec2 rectSize, int cursorOffset);
    
    void startTextEntry();
    void stopTextEntry();

	std::string getClipboardText();
	void setClipboardText(std::string);
    
    int addAnyKeyChangedListener(TuiFunction* listenerKeyChangedFunc_);
    int addSpecificKeyChangedListener(int keyCode, TuiFunction* listenerKeyChangedFunc_);
    void removeKeyChangedListener(int index);
    
    void setTextEntryListener(TuiFunction* textEntryListener_, TuiFunction* listenerKeyChangedFunc_);
    void removeTextEntryListener();
    
    int setOnEvent(const std::string& key, TuiFunction* callback);
    void removeOnEvent(int eventID);
    int setOnEvent(const std::string& key, std::function<TuiRef*(TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo)> value);
    
    void bindTui(TuiTable* rootTable);

	int getModKey();
	int getSecondModKey();

	Timer* debugTimer;
    

public:
    MainController* mainController;
    WindowInfo* windowInfo;
    SDL_Window* window;
    

	dvec2 mouseLoc;
    
    dvec4 windowSafeArea;
    
    bool isResizingWindow = false;
    std::string currentHostID = "";
    TuiTable* currentKatipoTable = nullptr;
    bool hasSetTextRect = false;

private:
    
    SDL_TimerID sdlTimer;
    Timer* timer;
    
    bool textEntryActive;
    bool needsToStartTextEntry;
    vec2 textEntryStartWindowOffset;
    bool needsToFinishTextEntry;
    
    bool appHasFocus = true;

	double smoothedTimeStep;
	double accumulatedTimeStepError = 0.0;
    double accumulator = 0.0;
    
    int backgroundedFrameSkipCount = 0;

	bool needsToExit = false;
    bool shouldPreventMouseWarpUntilAfterNextShow = false;
    //dvec2 mouseLocOnLastHide;
    

	dvec2 mouseMovementAccumulation = dvec2(0.0);
	bool mouseMoved = false;
	bool needsToResetMousePosition = false;
    
    bool hasTextEntryListener = false;
    TuiFunction* textEntryListener = nullptr;
    TuiFunction* listenerKeyChangedFunc = nullptr;
    
    int keyChangedListenerFunctionIndex = 0;
    std::map<int,TuiFunction*> anyKeyChangedListenerFunctions;
    std::map<std::string, std::map<int,std::map<int, TuiFunction*> > > keyChangedByKeyListenerFunctions;
    
    void eventManagerTableKeyChanged(const std::string& key, TuiRef* value);
    
    int onEventIndexCounter = 0;
    std::map<std::string, std::map<int, TuiFunction*>> onEventsByTypeByID;

    
private:
	void checkEvents();
    void update(float dt);

	void createControllerSets();
};

#endif /* EventManager_hpp */
