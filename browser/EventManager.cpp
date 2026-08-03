
#include "EventManager.h"
#include "MJLog.h"
#include "MainController.h"
#include "Timer.h"
#include "TuiScript.h"
#include "KatipoUtilities.h"
#include "MJTimer.h"
#include "MJView.h"
#include <thread>

#define RUN_GAME_LOOP_CODE 1

#define MAIN_THREAD_FIXED_TIME_STEP (1.0 / 60.0)

EventManager::EventManager()
{
    
}


static bool resizingEventWatcher(void* data, SDL_Event* event) {
    switch(event->type)
    {
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:
        {
            MainController::getInstance()->mainWindowChangedSize();
            /*EventManager::getInstance()->isResizingWindow = true;
            EventManager::getInstance()->idle();
            EventManager::getInstance()->isResizingWindow = false;*/
            
            SDL_Rect rect;
            SDL_GetWindowSafeArea(EventManager::getInstance()->window, &rect);
            //MJLog("SDL_GetWindowSafeArea result in SDL_EVENT_WINDOW_RESIZED:(%d, %d, %d, %d)", rect.x, rect.y, rect.w, rect.h);
            double uiScale = MainController::getInstance()->userUIScale;
            EventManager::getInstance()->windowSafeArea = dvec4(rect.x, rect.y, rect.w * uiScale, rect.h * uiScale);
            MainController::getInstance()->mainMJView->needsToUpdateSizeDueToWindowChange = true;
        }
            break;
        case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
        {
            SDL_Rect rect;
            SDL_GetWindowSafeArea(EventManager::getInstance()->window, &rect);
            //MJLog("SDL_GetWindowSafeArea result in SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:(%d, %d, %d, %d)", rect.x, rect.y, rect.w, rect.h);
            double uiScale = MainController::getInstance()->userUIScale;
            EventManager::getInstance()->windowSafeArea = dvec4(rect.x, rect.y, rect.w * uiScale, rect.h * uiScale);
            MainController::getInstance()->mainMJView->needsToUpdateSizeDueToWindowChange = true;
        }
            break;
    }
  return 0;
}


void EventManager::init(MainController* mainController_,
	SDL_Window* window_,
    WindowInfo* windowInfo_)
{
    mainController = mainController_;
	window = window_;
    windowInfo = windowInfo_;
    
    SDL_Rect rect;
    SDL_GetWindowSafeArea(EventManager::getInstance()->window, &rect);
    //MJLog("init setting windowSafeArea:(%d, %d, %d, %d)", rect.x, rect.y, rect.w, rect.h);
    EventManager::getInstance()->windowSafeArea = dvec4(rect.x, rect.y, rect.w * mainController->userUIScale, rect.h * mainController->userUIScale);
    
    SDL_AddEventWatch(resizingEventWatcher, window);
    

	smoothedTimeStep = MAIN_THREAD_FIXED_TIME_STEP;
    
    needsToStartTextEntry = false;
    needsToFinishTextEntry = false;
    textEntryActive = false;
    
    appHasFocus = true;
    
    //sdlTimer = SDL_AddTimer(1, gameLoopTimer, this);
    timer = new Timer();
#if LOG_DEBUG_TIMINGS
	debugTimer = new Timer();
#endif

}

EventManager::~EventManager()
{
   // SDL_RemoveTimer(sdlTimer);

    delete timer;
    
    if(textEntryListener)
    {
        textEntryListener->release();
    }
    if(listenerKeyChangedFunc)
    {
        listenerKeyChangedFunc->release();
    }
    for(auto& kv : anyKeyChangedListenerFunctions)
    {
        kv.second->release();
    }
    for(auto& kv : keyChangedByKeyListenerFunctions)
    {
        for(auto& kvb : kv.second)
        {
            for(auto& kvc : kvb.second)
            {
                kvc.second->release();
            }
        }
    }
}


int EventManager::getModKey()
{
	int modKey = 0;

	SDL_Keymod modState = SDL_GetModState();

	if(modState & SDL_KMOD_SHIFT)
	{
		modKey = MJ_KEY_MOD_SHIFT;
	}
	else if(modState & SDL_KMOD_CTRL)
	{
		modKey = MJ_KEY_MOD_CTRL;
	}
	else if(modState & SDL_KMOD_ALT)
	{
		modKey = MJ_KEY_MOD_ALT;
	}
	else if(modState & SDL_KMOD_GUI)
	{
		modKey = MJ_KEY_MOD_CMD;
	}

	return modKey;
}

int EventManager::getSecondModKey()
{
	int modKey = 0;

	SDL_Keymod modState = SDL_GetModState();

	
	if(modState & SDL_KMOD_SHIFT)
	{
		if(modKey != 0)
		{
			return MJ_KEY_MOD_SHIFT;
		}
		modKey = MJ_KEY_MOD_SHIFT;
	}
	if(modState & SDL_KMOD_CTRL)
	{
		if(modKey != 0)
		{
			return MJ_KEY_MOD_CTRL;
		}
		modKey = MJ_KEY_MOD_CTRL;
	}
	if(modState & SDL_KMOD_ALT)
	{
		if(modKey != 0)
		{
			return MJ_KEY_MOD_ALT;
		}
		modKey = MJ_KEY_MOD_ALT;
	}
	if(modState & SDL_KMOD_GUI)
	{
		if(modKey != 0)
		{
			return MJ_KEY_MOD_CMD;
		}
		modKey = MJ_KEY_MOD_CMD;
	}

	return 0;
}

void EventManager::doCPUWork()
{
#if LOG_DEBUG_TIMINGS
	debugTimer->getDt();
#endif
	double dt = clamp(timer->getDt(), 0.0, 0.1);
	double dtToUse = dt;//smoothedTimeStep;

	//MJLog("doCPUWork:dt%.2fms\t smoothed:%.2fms\t accumulatedTimeStepError:%.2fms", dt * 1000.0, smoothedTimeStep * 1000.0, accumulatedTimeStepError * 1000.0);

	accumulator += dtToUse;

	while(accumulator >= MAIN_THREAD_FIXED_TIME_STEP)
	{
        /*if(!isResizingWindow) //now handled in sdl3 main
        {
            checkEvents();
        }*/

		if(mouseMoved)
		{
			//MJLog("calling mouse moved lua functon (%.2f, %.2f)", mouseMovementAccumulation.x, mouseMovementAccumulation.y);
			/*luaModule->callFunction("mouseMoved",
				mouseLoc,
				mouseMovementAccumulation,
				MAIN_THREAD_FIXED_TIME_STEP);*/
            //TODO ---------------------------------------------- call mouseMoved
            
            mainController->mouseMoved();
            
			mouseMoved = false;
			mouseMovementAccumulation = dvec2(0.0);
		}

		//MJLog("update");
		update(MAIN_THREAD_FIXED_TIME_STEP);
		accumulator -= MAIN_THREAD_FIXED_TIME_STEP;
	}

	//accumulator = MAX(accumulator, -MAIN_THREAD_FIXED_TIME_STEP);

#if LOG_DEBUG_TIMINGS
	MJLog("EventManager doCPUWork:%.2f dtToUse:%.2f", (debugTimer->getDt() * 1000.0), dtToUse * 1000.0);
#endif
}

void EventManager::idle()
{
    
    if(!appHasFocus)
    {
#if TARGET_OS_IPHONE
        return; // we can't render in the background or we will get killed
#endif
        backgroundedFrameSkipCount++;
        if(backgroundedFrameSkipCount > 5)
        {
            backgroundedFrameSkipCount = 0;
        }
        else
        {
            return;
        }
    }
    mainController->draw(accumulator / MAIN_THREAD_FIXED_TIME_STEP);
    
}



dvec2 convertSDLToMouseLoc(dvec2 inLoc, WindowInfo* windowInfo)
{
	return dvec2((inLoc.x - windowInfo->halfWindowWidth) / windowInfo->halfWindowWidth * windowInfo->halfScreenWidth,  
		-(inLoc.y- windowInfo->halfWindowHeight) / windowInfo->halfWindowHeight * windowInfo->halfScreenHeight);
}

dvec2 convertMouseLocToSDL(dvec2 inLoc, WindowInfo* windowInfo)
{
	return dvec2(((inLoc.x / windowInfo->halfScreenWidth) * windowInfo->halfWindowWidth) + windowInfo->halfWindowWidth,  
		((-inLoc.y / windowInfo->halfScreenHeight) * windowInfo->halfWindowHeight) + windowInfo->halfWindowHeight);
}


void EventManager::setTextEntryRect(dvec2 rectPos, dvec2 rectSize, int cursorOffset)
{
    dvec2 sdlPos = dvec2(((rectPos.x / windowInfo->halfScreenWidth) * windowInfo->halfWindowWidth) + windowInfo->halfWindowWidth,
                         ((-rectPos.y / windowInfo->halfScreenHeight) * windowInfo->halfWindowHeight) + windowInfo->windowHeight);
    dvec2 sdlSize = rectSize / dvec2(windowInfo->halfScreenWidth, windowInfo->halfScreenHeight) * dvec2(windowInfo->halfWindowWidth, windowInfo->halfWindowHeight);
    
    SDL_Rect rect;
    rect.x = sdlPos.x;
    rect.y = sdlPos.y;
    rect.w = sdlSize.x;
    rect.h = sdlSize.y;
    SDL_SetTextInputArea(window, &rect, cursorOffset);
    hasSetTextRect = true;
}

void EventManager::update(float dt)
{
    if(needsToStartTextEntry)
    {
        needsToStartTextEntry = false;
        if(!hasSetTextRect)
        {
            MJWarn("setTextEntryRect should be called before starting text entry.");
        }
        
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetBooleanProperty(props, SDL_PROP_TEXTINPUT_AUTOCORRECT_BOOLEAN, false);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTINPUT_CAPITALIZATION_NUMBER, SDL_CAPITALIZE_NONE);
        SDL_StartTextInputWithProperties(window, props);
        SDL_DestroyProperties(props);
        
        textEntryActive = true;
    }
    if(needsToFinishTextEntry)
    {
        needsToFinishTextEntry = false;
        SDL_StopTextInput(window);
        textEntryActive = false;
    }

    
    mainController->update(dt);
}

void EventManager::handleEvent(SDL_Event* event)
{
    switch (event->type) {

    case SDL_EVENT_TEXT_INPUT:

        if(textEntryActive)
        {
            /*if( !( ( event->text.text[ 0 ] == 'c' || event->text.text[ 0 ] == 'C' ) && ( event->text.text[ 0 ] == 'v' || event->text.text[ 0 ] == 'V' ) && SDL_GetModState() & KMOD_CTRL ) )*/
            {
                std::string text = event->text.text;
                if(!text.empty())
                {
                    //luaModule->callFunction("textEntry",text);
                    if(hasTextEntryListener)
                    {
                        TuiString* textRef = new TuiString(text);
                        textEntryListener->call("textEntryListener", textRef);
                        textRef->release();
                    }
                }
            }
        }
        break;

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    {
        bool keyDown = ((event->type == SDL_EVENT_KEY_DOWN) ? true : false);

        //luaModule->callFunction("keyChanged",keyDown, event->key.keysym.sym, getModKey(), (event->key.repeat != 0));
        
        for(auto& indexAndFunc : anyKeyChangedListenerFunctions)
        {
            //indexAndFunc.second(keyDown, event->key.key, getModKey(), (event->key.repeat != 0));
            TuiRef* keyRef = new TuiNumber(event->key.key);
            TuiRef* modRef = new TuiNumber(getModKey());
            indexAndFunc.second->call("keyChangedListenerFunction", TUI_BOOL(keyDown), keyRef, modRef, TUI_BOOL(event->key.repeat != 0));
            keyRef->release();
            modRef->release();
        }
        
        if(keyChangedByKeyListenerFunctions.count("") != 0 && keyChangedByKeyListenerFunctions[""].count(event->key.key) != 0) //bit hacky, "" is what happens when we set from the browser before a site has been loaded, eg cmd-R for reload
        {
            for(auto& indexAndFunc : keyChangedByKeyListenerFunctions[""][event->key.key])
            {
                TuiRef* keyRef = new TuiNumber(event->key.key);
                TuiRef* modRef = new TuiNumber(getModKey());
                indexAndFunc.second->call("keyChangedListenerFunction", TUI_BOOL(keyDown), keyRef, modRef, TUI_BOOL(event->key.repeat != 0));
                keyRef->release();
                modRef->release();
            }
        }
        
        if(keyChangedByKeyListenerFunctions.count(currentHostID) != 0 && keyChangedByKeyListenerFunctions[currentHostID].count(event->key.key) != 0)
        {
            for(auto& indexAndFunc : keyChangedByKeyListenerFunctions[currentHostID][event->key.key])
            {
                TuiRef* keyRef = new TuiNumber(event->key.key);
                TuiRef* modRef = new TuiNumber(getModKey());
                indexAndFunc.second->call("keyChangedListenerFunction", TUI_BOOL(keyDown), keyRef, modRef, TUI_BOOL(event->key.repeat != 0));
                keyRef->release();
                modRef->release();
            }
        }
        
        if(hasTextEntryListener)
        {
            TuiRef* keyRef = new TuiNumber(event->key.key);
            TuiRef* modRef = new TuiNumber(getModKey());
            listenerKeyChangedFunc->call("listenerKeyChangedFunc", TUI_BOOL(keyDown), keyRef, modRef, TUI_BOOL(event->key.repeat != 0));
            keyRef->release();
            modRef->release();
        }

        if(event->type == SDL_EVENT_KEY_DOWN && event->key.repeat == 0)
        {
            if(event->key.key == SDLK_V && (SDL_GetModState() & SDL_KMOD_GUI || SDL_GetModState() & SDL_KMOD_CTRL))
            {
                std::string text = SDL_GetClipboardText();
                if(!text.empty())
                {
                    //luaModule->callFunction("textEntry",text);
                    if(hasTextEntryListener)
                    {
                        //textEntryListener(text);
                        TuiRef* textRef = new TuiString(text);
                        textEntryListener->call("textEntryListener", textRef);
                        textRef->release();
                    }
                }
            }
        }
    }
    break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        //MJLog("mouseUpEvent")
        if(event->button.button == SDL_BUTTON_LEFT)
        {
            mainController->mouseUp(convertSDLToMouseLoc(dvec2(event->motion.x, event->motion.y), windowInfo), 0, getModKey());
        }
        else if(event->button.button == SDL_BUTTON_RIGHT)
        {
            mainController->mouseUp(convertSDLToMouseLoc(dvec2(event->motion.x, event->motion.y), windowInfo), 1, getModKey());
        }
        else if(event->button.button == SDL_BUTTON_MIDDLE)
        {
            mainController->mouseUp(convertSDLToMouseLoc(dvec2(event->motion.x, event->motion.y), windowInfo), 2, getModKey());
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        //MJLog("mnouseDownEvent")
        mouseLoc = convertSDLToMouseLoc(dvec2(event->motion.x, event->motion.y), windowInfo);
        if(event->button.button == SDL_BUTTON_LEFT)
        {
            mainController->mouseDown(convertSDLToMouseLoc(dvec2(event->motion.x, event->motion.y), windowInfo), 0, getModKey());
            /*MJLog("%d, %d (%d)", event->motion.x, event->motion.y, (int)windowInfo->screenHeight);
            MJLog("%.2f, %.2f - %.2f, %.2f", windowInfo->screenWidth, windowInfo->screenHeight, windowInfo->windowWidth, windowInfo->windowHeight);
            dvec2 debugVal = convertMouseLoc(dvec2(event->motion.x, event->motion.y), windowInfo);
            MJLog("%.2f, %.2f", debugVal.x, debugVal.y);*/
        }
        else if(event->button.button == SDL_BUTTON_RIGHT)
        {
            mainController->mouseDown(convertSDLToMouseLoc(dvec2(event->motion.x, event->motion.y), windowInfo), 1, getModKey());
        }
        else if(event->button.button == SDL_BUTTON_MIDDLE)
        {
            mainController->mouseDown(convertSDLToMouseLoc(dvec2(event->motion.x, event->motion.y), windowInfo), 2, getModKey());
        }
        break;

    case SDL_EVENT_MOUSE_MOTION:
        {

            mouseMoved = true;
            mouseMovementAccumulation += dvec2(event->motion.xrel, event->motion.yrel);
            mouseLoc = convertSDLToMouseLoc(dvec2(event->motion.x, event->motion.y), windowInfo);
        }

        break;
    case SDL_EVENT_MOUSE_WHEEL:
            mainController->mouseWheel(convertSDLToMouseLoc(dvec2(event->motion.x, event->motion.y), windowInfo), dvec2(event->wheel.x, event->wheel.y));
        //luaModule->callFunction("mouseWheel",
            //dvec2(event->motion.x, event->motion.y), dvec2(event->wheel.x, event->wheel.y), getModKey());
        break;
            
    /*case SDL_MULTIGESTURE:
            
            //luaModule->callFunction("multiGesture",
                //dvec2(event->motion.x, event->motion.y), event->mgesture.dTheta, event->mgesture.dDist, getModKey());
            break;*/

    case SDL_EVENT_QUIT:
        {
            needsToExit = true;
            return;
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        {
            mainController->appLostFocus();
            appHasFocus = false;
            if(currentKatipoTable)
            {
                TuiFunction* onBackgroundChange = currentKatipoTable->getFunction("onBackgroundChange");
                if(onBackgroundChange)
                {
                    onBackgroundChange->call("onBackgroundChange", TUI_TRUE);
                }
            }
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        {
            mainController->appGainedFocus();
            appHasFocus = true;
            if(currentKatipoTable)
            {
                TuiFunction* onBackgroundChange = currentKatipoTable->getFunction("onBackgroundChange");
                if(onBackgroundChange)
                {
                    onBackgroundChange->call("onBackgroundChange", TUI_FALSE);
                }
            }
        }
        break;
    case SDL_EVENT_WINDOW_MOVED:
        mainController->mainWindowChangedPosition();
        appHasFocus = true;
        break;
    //case SDL_EVENT_WINDOW_RESIZED: //handled above
        //mainController->mainWindowChangedSize();
        //runLoopRunning = true;
        //break;
    default:
        break;
    }
}

void EventManager::checkEvents()
{

#if LOG_DEBUG_TIMINGS
	debugTimer->getDt();
#endif
	/*if(runLoopRunning)
	{
		MJLog("checkEvents");
	}*/
	SDL_Event event;
	while(SDL_PollEvent(&event))
    {
        handleEvent(&event);
    }

#if LOG_DEBUG_TIMINGS
	MJLog("checkEvents():%.2f", (debugTimer->getDt() * 1000.0));
#endif
}

void EventManager::preventMouseWarpUntilAfterNextShow()
{
    shouldPreventMouseWarpUntilAfterNextShow = true;
}

void EventManager::warpMouse(dvec2 mouseLoc)
{
    //MJLog("warpMouse called externally pos:(%.2f, %.2f)", mouseLoc.x, mouseLoc.y)
    dvec2 sdlMouseLoc = convertMouseLocToSDL(mouseLoc, windowInfo);
    SDL_WarpMouseInWindow(window, sdlMouseLoc.x, sdlMouseLoc.y);
}

dvec2 EventManager::getMouseScreenFractionNonClamped()
{
    float testX, testY;
    SDL_GetGlobalMouseState(&testX, &testY);
    //MJLog("mouse :%d,%d  size:(%.1f,%.1f)", testX - windowInfo->posX, testY - windowInfo->posY, windowInfo->windowWidth, windowInfo->windowHeight)
    dvec2 pixels = dvec2(testX - windowInfo->posX, testY - windowInfo->posY);
    return dvec2(pixels.x / windowInfo->windowWidth, pixels.y / windowInfo->windowHeight);
}

void EventManager::runEventLoop()
{
    while(!needsToExit)
    {
		idle();
    }
    exit(0);
}


void EventManager::startTextEntry()
{
    needsToFinishTextEntry = false;
    if(!textEntryActive)
    {
        needsToStartTextEntry = true;
    }
}

void EventManager::stopTextEntry()
{
    needsToStartTextEntry = false;
    if(textEntryActive)
    {
        needsToFinishTextEntry = true;
    }
}


std::string EventManager::getClipboardText()
{
	char* clipboardText = SDL_GetClipboardText();
	if(clipboardText)
	{
		return clipboardText;
	}
	return "";
}

void EventManager::setClipboardText(std::string text)
{
	SDL_SetClipboardText(text.c_str());
}


void EventManager::setTextEntryListener(TuiFunction* textEntryListener_, TuiFunction* listenerKeyChangedFunc_)
{
    //finishActiveEvents()
    if(textEntryListener)
    {
        textEntryListener->release();
    }
    textEntryListener = textEntryListener_;
    textEntryListener->retain();
    
    if(listenerKeyChangedFunc)
    {
        listenerKeyChangedFunc->release();
    }
    listenerKeyChangedFunc = listenerKeyChangedFunc_;
    listenerKeyChangedFunc->retain();
    if(!hasTextEntryListener)
    {
        startTextEntry();
    }
    hasTextEntryListener = true;
}

void EventManager::removeTextEntryListener()
{
    hasTextEntryListener = false;
    stopTextEntry();
}


int EventManager::addAnyKeyChangedListener(TuiFunction* listenerKeyChangedFunc_)
{
    int index = keyChangedListenerFunctionIndex++;
    listenerKeyChangedFunc_->retain();
    anyKeyChangedListenerFunctions[index] = listenerKeyChangedFunc_;
    return index;
}

int EventManager::addSpecificKeyChangedListener(int keyCode, TuiFunction* listenerKeyChangedFunc_)
{
    int index = keyChangedListenerFunctionIndex++;
    listenerKeyChangedFunc_->retain();
    keyChangedByKeyListenerFunctions[currentHostID][keyCode][index] = listenerKeyChangedFunc_;
    return index;
}

void EventManager::removeKeyChangedListener(int index)
{
    anyKeyChangedListenerFunctions.erase(index);
    for(auto& hostIDAndStuff : keyChangedByKeyListenerFunctions)
    {
        for(auto& keyCodeAndSet : hostIDAndStuff.second)
        {
            if(keyCodeAndSet.second.count(index) != 0)
            {
                keyCodeAndSet.second[index]->release();
                keyCodeAndSet.second.erase(index);
            }
        }
    }
}

void EventManager::eventManagerTableKeyChanged(const std::string& key, TuiRef* value)
{
    if(key == "uiScale")
    {
        switch (value->type()) {
            case Tui_ref_type_NUMBER:
            {
                if(mainController->userUIScale != ((TuiNumber*)value)->value)
                {
                    mainController->userUIScale = ((TuiNumber*)value)->value;
                    SDL_Rect rect;
                    SDL_GetWindowSafeArea(EventManager::getInstance()->window, &rect);
                    EventManager::getInstance()->windowSafeArea = dvec4(rect.x, rect.y, rect.w * mainController->userUIScale, rect.h * mainController->userUIScale);
                    MainController::getInstance()->mainMJView->needsToUpdateSizeDueToWindowChange = true;
                    MainController::getInstance()->saveWindowInfoToDatabase();
                }
            }
                break;
            default:
                MJError("Expected number");
                break;
        }
    }
}

void EventManager::bindTui(TuiTable* rootTable)
{
    TuiTable* eventManagerTable = new TuiTable(rootTable);
    rootTable->setTable("eventManager", eventManagerTable);
    eventManagerTable->release();
    
    eventManagerTable->setDouble("uiScale", mainController->userUIScale);
    
    eventManagerTable->onSet = [this](TuiRef* table, const std::string& key, TuiRef* value) {
        eventManagerTableKeyChanged(key, value);
    };
    
    eventManagerTable->setFunction("addKeyEventListener", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 2)
        {
            TuiRef* keyCodeRef = args->arrayObjects[0];
            TuiRef* funcRef = args->arrayObjects[1];
            if(keyCodeRef->type() == Tui_ref_type_NUMBER && funcRef->type() == Tui_ref_type_FUNCTION)
            {
                int result = addSpecificKeyChangedListener(((TuiNumber*)keyCodeRef)->value, (TuiFunction*)funcRef);
                return new TuiNumber(result);
            }
            else
            {
                TuiParseError(callingDebugInfo, "Incorrect types passed to function. expected number, function");
            }
        }
        return TUI_NIL;
    });
    
    
    //void removeKeyChangedListener(int index);
    
   // void setTextEntryListener(TuiFunction* textEntryListener_, TuiFunction* listenerKeyChangedFunc_);
   // void removeTextEntryListener();
    
    eventManagerTable->setFunction("removeKeyEventListener", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* idRef = args->arrayObjects[0];
            if(idRef->type() == Tui_ref_type_NUMBER)
            {
                removeKeyChangedListener(((TuiNumber*)idRef)->value);
            }
            else
            {
                TuiParseError(callingDebugInfo, "Incorrect type");
            }
        }
        return TUI_NIL;
    });
    
    
    eventManagerTable->setFunction("setTextEntryRectFromView", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 2)
        {
            TuiRef* viewTableRef = args->arrayObjects[0];
            TuiRef* cursorOffsetRef = args->arrayObjects[1];
            
            if(viewTableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "Incorrect type for first arg, view");
            }
            if(cursorOffsetRef->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(callingDebugInfo, "Incorrect type for second arg, cursor offset (number)");
            }
            
            MJView* textBoundsView = (MJView*)((TuiTable*)viewTableRef)->getUserData("_view");
            
            dvec2 bottomLeft = dvec2(0,0); //todo handle other alignment?
            dvec2 topRight = textBoundsView->size; //todo handle other alignment?
            
            dvec2 windowPos = textBoundsView->locationRelativeToView(bottomLeft, mainController->mainMJView);
            dvec2 cursorWindowPos = textBoundsView->locationRelativeToView(bottomLeft + dvec2(((TuiNumber*)viewTableRef)->value, 0), mainController->mainMJView);
            dvec2 extentPos = textBoundsView->locationRelativeToView(topRight, mainController->mainMJView);
            
            /*
             
             #windowPos = backgroundView.localPointToWindow(vec2(0,0))
             #cursorWindowPos = textView.localPointToWindow(localCursorOffset)
             #extentPos = backgroundView.localPointToWindow(backgroundView.size)
         #
             #print("windowPos:", windowPos)
             #print("cursorWindowPos:", cursorWindowPos)
             #print("extentPos:", extentPos)
             #eventManager.setTextEntryRect(windowPos, extentPos - windowPos, cursorWindowPos.x - windowPos.x)
             */
            
            
            setTextEntryRect(windowPos, extentPos - windowPos, cursorWindowPos.x - windowPos.x);
        }
        return TUI_NIL;
    });
    
    
    eventManagerTable->setFunction("setTextEntryListener", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 2)
        {
            TuiRef* textEntryListenerFuncRef = args->arrayObjects[0];
            TuiRef* listenerKeyChangedFuncRef = args->arrayObjects[1];
            if(textEntryListenerFuncRef->type() == Tui_ref_type_FUNCTION && listenerKeyChangedFuncRef->type() == Tui_ref_type_FUNCTION)
            {
                setTextEntryListener((TuiFunction*)textEntryListenerFuncRef, (TuiFunction*)listenerKeyChangedFuncRef);
            }
            else
            {
                TuiParseError(callingDebugInfo, "Incorrect type");
            }
        }
        return TUI_NIL;
    });
    
    eventManagerTable->setFunction("removeTextEntryListener", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        removeTextEntryListener();
        return TUI_NIL;
    });
    
    
    eventManagerTable->setFunction("getWindowSafeArea", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        return new TuiVec4(windowSafeArea);
    });
    
    
}
