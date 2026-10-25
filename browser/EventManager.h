
#ifndef EventManager_hpp
#define EventManager_hpp

#include "WindowInfo.h"
#include "TuiScript.h"
#include "SDL.h"
#include <functional>
#include <map>

class MainController;
class Timer;

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
    
    int addAnyKeyChangedListener(const tui::function& listenerKeyChangedFunc_);
    int addSpecificKeyChangedListener(int keyCode, const tui::function& listenerKeyChangedFunc_);
    void removeKeyChangedListener(int index);
    
    void setTextEntryListener(const tui::function& textEntryListener_, const tui::function& listenerKeyChangedFunc_);
    void removeTextEntryListener();
    
    int setOnEvent(const std::string& key, const tui::function& callback);
    void removeOnEvent(int eventID);
    int setOnEvent(const std::string& key, std::function<tui::ref(const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo)> value);
    
    void bindTui(const tui::table& rootTable);

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
    tui::table mainKatipoTable = nullptr;
    tui::table currentSiteKatipoTable = nullptr;
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
    tui::function textEntryListener = nullptr;
    tui::function listenerKeyChangedFunc = nullptr;
    
    int keyChangedListenerFunctionIndex = 0;
    std::map<int,tui::function> anyKeyChangedListenerFunctions;
    std::map<std::string, std::map<int,std::map<int, tui::function> > > keyChangedByKeyListenerFunctions;
    
    void eventManagerTableKeyChanged(const std::string& key, const tui::ref& value);
    
    int onEventIndexCounter = 0;
    std::map<std::string, std::map<int, tui::function>> onEventsByTypeByID;

    
private:
	void checkEvents();
    void update(float dt);

	void createControllerSets();
};

#endif /* EventManager_hpp */
