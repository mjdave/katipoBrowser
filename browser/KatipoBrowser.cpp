
#ifdef _MSC_VER
#define _WINSOCKAPI_    // stops windows.h including winsock.h
#include <windows.h>
#include <direct.h>
#include <cstdint>
#endif

#include "KatipoBrowser.h"
#include "KatipoUtilities.h"
#include "MainController.h"
#include "EventManager.h"
#include "MJTimer.h"
#include "TuiFileUtils.h"
#include "ClientNetInterface.h"
#include "MJTimer.h"
#include "MJAudio.h"
#include "sodium.h"

#include "MJView.h"


void doGet(ClientNetInterface* netInterface, const std::string& remoteURL, const std::string& hostName, TuiTable* args)
{
    TuiFunction* mainGetCallbackFunction = nullptr;
    if(!args->arrayObjects.empty() && args->arrayObjects[args->arrayObjects.size() - 1]->type() == Tui_ref_type_FUNCTION)
    {
        mainGetCallbackFunction = ((TuiFunction*)args->arrayObjects[args->arrayObjects.size() - 1]);
        
        if(!netInterface->connected)
        {
            MJError("attempt to call get before first request has returned or while disconnected"); //todo need to cancel first request maybe?
            TuiRef* statusResult = new TuiTable("{status='error',message='not connected'}");
            mainGetCallbackFunction->call("mainGetCallbackFunction", statusResult);
            statusResult->release();
            return;
        }
        
        mainGetCallbackFunction->retain();
    }
    
    TuiTable* remoteFuncCallArgs = new TuiTable(nullptr); //todo leaks?
    
    TuiString* remoteURLString = new TuiString(remoteURL);
    remoteFuncCallArgs->arrayObjects.push_back(remoteURLString);
    
    for(int i = 1; i < args->arrayObjects.size(); i++)
    {
        if(args->arrayObjects[i]->type() != Tui_ref_type_FUNCTION)
        {
            TuiRef* arg = args->arrayObjects[i];
            arg->retain();
            remoteFuncCallArgs->arrayObjects.push_back(arg);
        }
    }
    
    MJLog("fetching from remote hostName:%s", hostName.c_str());
    
    
    TuiFunction* callHostFunctionCallbackFunction = new TuiFunction([mainGetCallbackFunction](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        
        if(args && args->arrayObjects.size() >= 2)
        {
            TuiRef* result = args->arrayObjects[0];
            TuiRef* publicKey = args->arrayObjects[1];
            mainGetCallbackFunction->call("mainGetCallbackFunction", result, publicKey);
        }
        else
        {
            mainGetCallbackFunction->call("mainGetCallbackFunction", TUI_NIL);
        }
        
        mainGetCallbackFunction->release();
        return TUI_NIL;
    });
    
    remoteFuncCallArgs->push(callHostFunctionCallbackFunction);
    callHostFunctionCallbackFunction->release();
    
    TuiFunction* getSiteKeyCallbackFunction = new TuiFunction([mainGetCallbackFunction, remoteFuncCallArgs, netInterface](TuiTable* incomingCallbackResponseData, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        
        if(incomingCallbackResponseData && !incomingCallbackResponseData->arrayObjects.empty())
        {
            TuiRef* result = incomingCallbackResponseData->arrayObjects[0];
            
            if(result->type() == Tui_ref_type_TABLE && (((TuiTable*)result)->hasKey("data") || ((TuiTable*)result)->getString("status") == "ok"))
            {
                //TODO check for host key locally tracker_hostname
                //abort on mismatch
                //save new host key
                
                TuiRef* hostPublicKeyRef = ((TuiTable*)result)->get("publicKey");
                if(hostPublicKeyRef && hostPublicKeyRef->type() == Tui_ref_type_STRING)
                {
                    netInterface->callRemoteHostFunction(((TuiString*)hostPublicKeyRef)->value, remoteFuncCallArgs);
                }
                else
                {
                    MJError("missing public key");
                    TuiRef* statusResult = new TuiTable("{status='error',message='missing public key'}");
                    mainGetCallbackFunction->call("mainGetCallbackFunction", statusResult);
                    mainGetCallbackFunction->release();
                    statusResult->release();
                }
                
            }
            else
            {
                mainGetCallbackFunction->call("mainGetCallbackFunction", result); //status not ok
                mainGetCallbackFunction->release();
            }
        }
        else
        {
            TuiRef* statusResult = new TuiTable("{status='error',message='no connection'}");
            mainGetCallbackFunction->call("mainGetCallbackFunction", statusResult);
            mainGetCallbackFunction->release();
            statusResult->release();
        }
        
        return TUI_NIL;
    });
    
    TuiTable* remoteHostKeyFuncCallArgs = new TuiTable(nullptr);
    remoteHostKeyFuncCallArgs->pushString("getSiteKey");
    remoteHostKeyFuncCallArgs->pushString(hostName);
    remoteHostKeyFuncCallArgs->push(getSiteKeyCallbackFunction);
    getSiteKeyCallbackFunction->release();
    
    netInterface->callTrackerFunction(remoteHostKeyFuncCallArgs);
    
    remoteHostKeyFuncCallArgs->release();
}

void KatipoBrowser::init()
{
    MainController::getInstance()->init("Katipo Browser");
    
     if(sodium_init() < 0) //this is safe to call multiple times
     {
         MJError("Sodium initialization failed. Exiting.");
         abort();
     }
    
    TuiTable* rootTable = Tui::getRootTable();
    
    rootTable->getTable("file")->setFunction("getSavePath", [rootTable](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return new TuiString(Katipo::getSavePath(args->arrayObjects[0]->getStringValue()));
        }
        return new TuiString(Katipo::getSavePath());
    });
    
    rootTable->getTable("file")->setFunction("getResourcePath", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            const std::string& appendPath = args->arrayObjects[0]->getStringValue();
            return new TuiString(Katipo::getResourcePath(appendPath));
        }
        return new TuiString(Katipo::getResourcePath());
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
        return TUI_NIL;
    });
    
    //rootTable->setVec2("screenSize", dvec2(MainController::getInstance()->windowInfo->screenWidth, MainController::getInstance()->windowInfo->screenHeight));
    
    EventManager::getInstance()->bindTui(rootTable);
    MJAudio::getInstance()->bindTui(rootTable);
    
    katipoTable = new TuiTable(rootTable);
    rootTable->set("katipo", katipoTable);
    katipoTable->release();
    
    //katipo.loadSite(siteSavePath, untrustedSiteCodePermissionFunction)
    katipoTable->setFunction("loadSite", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(currentSiteRootTable)
        {
            MJError("todo need to handle this currentSiteRootTable");
            return TUI_NIL;
        }
        if(args->arrayObjects.size() >= 1)
        {
            std::string siteSavePath = args->arrayObjects[0]->getStringValue();
            TuiFunction* permissionCallbackFunction = nullptr;
            if(args->arrayObjects.size() >= 2)
            {
                TuiRef* arg = args->arrayObjects[1];
                if(arg->type() == Tui_ref_type_FUNCTION)
                {
                    permissionCallbackFunction = (TuiFunction*)arg;
                }
            }
            currentSiteRootTable = Tui::initSafeRootTable(permissionCallbackFunction, siteSavePath);
            
            currentSiteRootTable->getTable("file")->setFunction("getSavePath", [siteSavePath](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
                if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
                {
                    const std::string& appendPath = args->arrayObjects[0]->getStringValue();
                    return new TuiString(Katipo::getSavePath(siteSavePath + "/" + appendPath));
                }
                return new TuiString(Katipo::getSavePath(siteSavePath + "/"));
            });
            
            currentSiteRootTable->getTable("file")->setFunction("getResourcePath", [siteSavePath](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
                if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
                {
                    const std::string& appendPath = args->arrayObjects[0]->getStringValue();
                    std::string siteResourcePath = siteSavePath + "/" + appendPath;
                    if(Tui::fileExistsAtPath(siteResourcePath))
                    {
                        return new TuiString(siteResourcePath); //todo ensure within allowed dirs
                    }
                    
                    return new TuiString(Katipo::getResourcePath(appendPath)); //todo ensure within allowed dirs
                }
                return new TuiString(Katipo::getResourcePath(Katipo::getSavePath(siteSavePath + "/"))); //todo ensure within allowed dirs
            });
            
            TuiTable* sceneTable = (TuiTable*)TuiRef::load(siteSavePath + "/scripts/scene.tui", currentSiteRootTable);
            currentSiteRootTable->setTable("scene", sceneTable);
            sceneTable->release();
            
            sceneTable->setFunction("getView", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
                if(args->arrayObjects.size() >= 1)
                {
                    TuiRef* viewNameRef = args->arrayObjects[0];
                    if(viewNameRef->type() == Tui_ref_type_STRING)
                    {
                        MJView* subView = currentSiteMainView->getSubViewWithID(((TuiString*)viewNameRef)->value);
                        return subView->stateTable->retain();
                    }
                }
                return TUI_NIL;
            });
            
            currentSiteMainView = MJView::loadUnknownViewFromTable(sceneTable->getTable("mainView"), MainController::getInstance()->mainMJView->getSubViewWithID("siteContent"), true);
            currentSiteScriptState = (TuiTable*)TuiRef::runScriptFile(siteSavePath + "/scripts/code.tui", currentSiteRootTable);
        }
        return TUI_NIL;
    });
    
    //katipo.get("127.0.0.1/example", sendData, function(result){ print("got result:", result)})
    katipoTable->setFunction("get", [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* urlRef = args->arrayObjects[0];
            if(urlRef->type() == Tui_ref_type_STRING)
            {
                std::string remoteURL = urlRef->getStringValue();
                std::vector<std::string> split = Tui::splitString(remoteURL, '/');
                
                std::string trackerURL = "127.0.0.1";
                std::string trackerPort = "3471";
                std::string hostName = split[0];
                
                if(split[0].find(".") != -1)
                {
                    std::vector<std::string> portSplit = Tui::splitString(split[0], ':');
                    trackerURL = portSplit[0];
                    if(portSplit.size() > 1)
                    {
                        trackerPort = portSplit[1];
                    }
                    remoteURL = remoteURL.substr(split[0].length() + 1, -1);
                    hostName = split[1];
                }
                
                std::string trackerKey = trackerURL + ":" + trackerPort;
                ClientNetInterface* netInterface = nullptr;
                if(netInterfaces.count(trackerKey) != 0)
                {
                    doGet(netInterfaces[trackerKey], remoteURL, hostName, args);
                }
                else
                {
                    std::string publicKey = "";
                    std::string secretKey = "";
                    
                    std::string clientKeyPath = Katipo::getSavePath("client_privateKey.tui"); //todo these should be saved in the database, not files
                    
                    if(Tui::fileExistsAtPath(clientKeyPath))
                    {
                        TuiTable* saveData = (TuiTable*)TuiRef::loadBinary(clientKeyPath);
                        if(saveData)
                        {
                            publicKey = saveData->getString("publicKey");
                            secretKey = saveData->getString("secretKey");
                        }
                    }
                    
                    if(publicKey.empty())
                    {
                        publicKey.resize(crypto_box_PUBLICKEYBYTES);
                        secretKey.resize(crypto_box_SECRETKEYBYTES);
                        crypto_box_keypair((unsigned char*)&(publicKey[0]), (unsigned char*)&(secretKey[0]));
                        
                        TuiTable* saveData = new TuiTable(nullptr);
                        
                        saveData->setString("publicKey", publicKey);
                        saveData->setString("secretKey", secretKey);
                        
                        saveData->saveBinary(clientKeyPath);
                        saveData->release();
                        MJLog("Generated and saved new private key:\n%s.\nPlease backup this file and keep it safe and secure!", Tui::getAbsolutePath(clientKeyPath).c_str());
                    }
                    else
                    {
                        MJLog("loaded private key:\n%s", Tui::getAbsolutePath(clientKeyPath).c_str());
                    }
                    
                    TuiTable* getArgs = args;
                    getArgs->retain();
                    TuiFunction* onConnect = new TuiFunction([this, trackerKey,remoteURL, hostName, getArgs](TuiTable* innerFuncArgs, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
                        //todo check for connection success
                        doGet(netInterfaces[trackerKey], remoteURL, hostName, getArgs);
                        getArgs->release();
                        return TUI_NIL;
                    });
                    
                    katipoTable->set("connected", onConnect);
                    
                    onConnect->release();
                    
                    netInterface = new ClientNetInterface(trackerURL,
                                                          trackerPort,
                                                          publicKey,
                                                          secretKey);
                    netInterfaces[trackerKey] = netInterface;
                    
                    netInterface->bindTui(katipoTable);
                }
            }
            else
            {
                MJError("get expected url string");
            }
        }
        return TUI_NIL;
    });
    
    TuiTable* sceneTable = (TuiTable*)TuiRef::load(Katipo::getResourcePath("app/katipoBrowser/scripts/scene.tui"), rootTable);
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
        return TUI_NIL;
    });
    
    mainView = MJView::loadUnknownViewFromTable(sceneTable->getTable("mainView"), MainController::getInstance()->mainMJView, true);
    
    scriptState = (TuiTable*)TuiRef::runScriptFile(Katipo::getResourcePath("app/katipoBrowser/scripts/code.tui"), rootTable);
    
    updateTimerID = MJTimer::getInstance()->addUpdateTimer([this](uint32_t timerID, float dt) {
         for(auto& idAndRequestInterface : netInterfaces)
         {
             idAndRequestInterface.second->pollNetEvents();
         }
    });
    
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
