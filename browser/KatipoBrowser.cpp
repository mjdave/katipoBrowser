#include "KatipoBrowser.h"
#include "KatipoUtilities.h"
#include "MainController.h"
#include "EventManager.h"
#include "MJTimer.h"
#include "TuiFileUtils.h"
#include "ClientNetInterface.h"
#include "MJTimer.h"
#include "MJAudio.h"

#include "MJView.h"

void KatipoBrowser::init()
{
    MainController::getInstance()->init("KatipoBrowser");
    cache = MainController::getInstance()->cache;
    
    TuiTable* rootTable = Tui::getRootTable();
    
    
    rootTable->getTable("file")->setFunction("getSavePath", [rootTable](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return new TuiString(Katipo::getSavePath(args->arrayObjects[0]->getStringValue()));
        }
        return new TuiString(Katipo::getSavePath());
    });
    
    
    rootTable->setFunction("require", [rootTable](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return TuiRef::load(Katipo::getResourcePath(args->arrayObjects[0]->getStringValue()), rootTable); //override require to use our resource path, not tui's
        }
        return nullptr;
    });
    
    rootTable->setFunction("updateTimer", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_FUNCTION)
        {
            TuiRef* arg = args->arrayObjects[0];
            MJTimer::getInstance()->addUpdateTimer([arg, callingDebugInfo](uint32_t timerID, float dt) {
                TuiTable* callbackArgs = new TuiTable(nullptr);
                
                TuiNumber* dtNum = new TuiNumber(dt);
                callbackArgs->push(dtNum);
                dtNum->release();
                
                ((TuiFunction*)arg)->call(callbackArgs, nullptr, callingDebugInfo);
                
                callbackArgs->release();
            });
        }
        return nullptr;
    });
    
    TuiTable* appTable = new TuiTable(rootTable);
    rootTable->setTable("app", appTable);
    appTable->release();
    rootTable->setVec2("screenSize", dvec2(MainController::getInstance()->windowInfo->screenWidth, MainController::getInstance()->windowInfo->screenHeight));
    
    EventManager::getInstance()->bindTui(rootTable);
    MJAudio::getInstance()->bindTui(rootTable);
    
    /*rootTable->setFunction("addKeyEventListener", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 2)
        {
            TuiRef* keyCodeRef = args->arrayObjects[0];
            TuiRef* funcRef = args->arrayObjects[1];
            if(keyCodeRef->type() == Tui_ref_type_NUMBER && funcRef->type() == Tui_ref_type_FUNCTION)
            {
                int charCode = ((TuiNumber*)keyCodeRef)->value;
                TuiCallback callback;
                callback.callingDebugInfo = *callingDebugInfo;
                callback.function = (TuiFunction*)funcRef;
                keyCallbackFunctionArraysByKeyCodes[charCode].push_back(callback);
            }
        }
        return nullptr;
    });
    
    
    EventManager::getInstance()->addKeyChangedListener([this](bool isDown, int code, int modKey, bool isRepeat) {
        if(keyCallbackFunctionArraysByKeyCodes.count(code) != 0)
        {
            TuiTable* callbackArgs = new TuiTable(nullptr);
            
            TuiBool* isDownT = new TuiBool(isDown);
            callbackArgs->push(isDownT);
            isDownT->release();
            
            TuiBool* isRepeatT = new TuiBool(isRepeat);
            callbackArgs->push(isRepeatT);
            isRepeatT->release();
            
            TuiNumber* codeNum = new TuiNumber(code);
            callbackArgs->push(codeNum);
            codeNum->release();
            
            TuiNumber* modKeyNum = new TuiNumber(modKey);
            callbackArgs->push(modKeyNum);
            modKeyNum->release();
            
            for(auto& callback : keyCallbackFunctionArraysByKeyCodes[code])
            {
                callback.function->call(callbackArgs, nullptr, &callback.callingDebugInfo);
            }
            
            callbackArgs->release();
        }
    });*/
    
    TuiTable* sceneTable = (TuiTable*)TuiRef::load(Katipo::getResourcePath("scripts/scene.tui"), rootTable);
    rootTable->setTable("scene", sceneTable);
    sceneTable->release();
    
    sceneTable->setFunction("getView", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* viewNameRef = args->arrayObjects[0];
            if(viewNameRef->type() == Tui_ref_type_STRING)
            {
                MJView* subView = mainView->getSubViewWithID(((TuiString*)viewNameRef)->value);
                return subView->stateTable->retain();
            }
        }
        return nullptr;
    });
    
    mainView = MJView::loadUnknownViewFromTable(sceneTable->getTable("mainView"), MainController::getInstance()->mainMJView, true);
    
    /*ClientNetInterface* netInterface = new ClientNetInterface("playerID",
                                                              "playerSessionID",
                                                              "192.168.0.75",
                                                              //"127.0.0.1",
                                                              "7121",
                                                              "dave");
    
    netInterface->bindTui(rootTable);*/
    
    
    scriptState = TuiRef::runScriptFile(Katipo::getResourcePath("scripts/code.tui"), rootTable);
    
    
    /*updateTimerID = MJTimer::getInstance()->addUpdateTimer([netInterface](uint32_t timerID, float dt) {
        netInterface->pollNetEvents();
    });*/
    
}


KatipoBrowser::KatipoBrowser()
{
    
}

KatipoBrowser::~KatipoBrowser()
{
    scriptState->release();
    rootTable->release();
    delete mainView;
}
