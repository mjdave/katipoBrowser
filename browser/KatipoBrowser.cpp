
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
#include "DatabaseEnvironment.h"
#include "Database.h"
#include "Scanner.h"

#include "MJView.h"


void KatipoBrowser::doGet(const std::string& trackerKey,
                          const std::string& hostID,
                          const std::string& remoteURL,
                          const std::string& hostName,
                          TuiTable* args)
{
    
    TuiFunction* mainGetCallbackFunction = nullptr;
    if(!args->arrayObjects.empty() && args->arrayObjects[args->arrayObjects.size() - 1]->type() == Tui_ref_type_FUNCTION)
    {
        mainGetCallbackFunction = ((TuiFunction*)args->arrayObjects[args->arrayObjects.size() - 1]);
        mainGetCallbackFunction->retain();
    }
    
    std::string fullURL = trackerKey + "/" + remoteURL;
    
    if(netInterfaces.count(trackerKey) == 0 || !netInterfaces[trackerKey]->connected)
    {
        if(mainGetCallbackFunction)
        {
            TuiRef* statusResult = new TuiTable("{status='error',message='not connected'}");
            TuiString* remoteURLString = new TuiString(fullURL);
            mainGetCallbackFunction->call("mainGetCallbackFunction", statusResult, TUI_NIL, remoteURLString);
            remoteURLString->release();
            statusResult->release();
            mainGetCallbackFunction->release();
        }
        return;
    }
    
    ClientNetInterface* netInterface = netInterfaces[trackerKey];
    TuiTable* remoteFuncCallArgs = new TuiTable(nullptr);
    
    TuiString* remoteURLStringForArg = new TuiString(remoteURL);
    remoteFuncCallArgs->arrayObjects.push_back(remoteURLStringForArg); //push_back, not retained, no need to release
    
    
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
    
    
    TuiFunction* callHostFunctionCallbackFunction = new TuiFunction([mainGetCallbackFunction, fullURL](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(mainGetCallbackFunction)
        {
            if(args && args->arrayObjects.size() >= 2)
            {
                TuiRef* result = args->arrayObjects[0];
                TuiRef* publicKey = args->arrayObjects[1];
                TuiString* remoteURLString = new TuiString(fullURL);
                mainGetCallbackFunction->call("mainGetCallbackFunction", result, publicKey, remoteURLString);
                remoteURLString->release();
            }
            else
            {
                TuiRef* statusResult = new TuiTable("{status='error',message='remote error'}");
                TuiString* remoteURLString = new TuiString(fullURL);
                mainGetCallbackFunction->call("mainGetCallbackFunction", statusResult, TUI_NIL, remoteURLString);
                statusResult->release();
                remoteURLString->release();
            }
            
            mainGetCallbackFunction->release();
        }
        return TUI_NIL;
    });
    
    remoteFuncCallArgs->push(callHostFunctionCallbackFunction);
    callHostFunctionCallbackFunction->release();
    
    TuiFunction* getSiteKeyCallbackFunction = new TuiFunction([this, trackerKey, hostName, mainGetCallbackFunction, remoteFuncCallArgs, netInterface, fullURL](TuiTable* incomingCallbackResponseData, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        
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
                    std::string hostSiteKey = TuiSHA1::sha1(trackerKey + hostName).substr(0,6) + "_" + hostName;
                    
                    netInterface->callRemoteHostFunction(hostSiteKey, ((TuiString*)hostPublicKeyRef)->value, remoteFuncCallArgs);
                    
                    SiteConnectionInfo& siteConnectionInfo = siteConnectionInfosByHostID[hostSiteKey];
                    siteConnectionInfo.trackerKey = trackerKey;
                    siteConnectionInfo.hostName = hostName;
                }
                else
                {
                    MJError("missing public key");
                    TuiRef* statusResult = new TuiTable("{status='error',message='missing public key'}");
                    TuiString* remoteURLString = new TuiString(fullURL);
                    mainGetCallbackFunction->call("mainGetCallbackFunction", statusResult, TUI_NIL, remoteURLString);
                    mainGetCallbackFunction->release();
                    statusResult->release();
                    remoteURLString->release();
                }
                
            }
            else
            {
                TuiString* remoteURLString = new TuiString(fullURL);
                mainGetCallbackFunction->call("mainGetCallbackFunction", result, TUI_NIL, remoteURLString); //status not ok
                mainGetCallbackFunction->release();
                remoteURLString->release();
            }
        }
        /*else //this causes a crash as we get another callback later... usually? all the time? Not sure yet
        {
            TuiRef* statusResult = new TuiTable("{status='error',message='no connection'}");
            mainGetCallbackFunction->call("mainGetCallbackFunction", statusResult);
            mainGetCallbackFunction->release();
            mainGetCallbackFunction = nullptr;
            statusResult->release();
        }*/
        
        remoteFuncCallArgs->release();
        
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

TuiTable* addKatipoTable(TuiTable* rootTableToAddTo)
{
    TuiTable* katipoTableResult = new TuiTable(rootTableToAddTo);
    rootTableToAddTo->set("katipo", katipoTableResult);
    katipoTableResult->release();
    
    return katipoTableResult;
}

void KatipoBrowser::init()
{
    if(sodium_init() < 0) //this is safe to call multiple times
    {
        MJError("Sodium initialization failed. Exiting.");
        abort();
    }
    
    TuiTable* rootTable = Tui::getRootTable();
    appDatabaseEnvironment = new DatabaseEnvironment(Katipo::getSavePath("database"),
                                                     1,
                                                     2);
    appDatabase = new Database(appDatabaseEnvironment, "app");
    appDatabase->bindTui("", rootTable);
    
    
    rootTable->getTable("file")->setFunction("getSavePath", [rootTable](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return new TuiString(Katipo::getSavePath(args->arrayObjects[0]->getStringValue()));
        }
        return new TuiString(Katipo::getSavePath());
    });
    
    rootTable->getTable("file")->setFunction("getResourcePath", [](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            const std::string& appendPath = args->arrayObjects[0]->getStringValue();
            return new TuiString(Katipo::getResourcePath(appendPath));
        }
        return new TuiString(Katipo::getResourcePath());
    });
    
    //rootTable->setVec2("screenSize", dvec2(MainController::getInstance()->windowInfo->screenWidth, MainController::getInstance()->windowInfo->screenHeight));
    
    
    MainController::getInstance()->init(rootTable, appDatabase, "Katipo Browser");
    
    
    EventManager::getInstance()->bindTui(rootTable);
    MJAudio::getInstance()->bindTui(rootTable);
    katipoTable = addKatipoTable(rootTable);
    
    //katipo.gotSiteUnchanged(hostID)
    /*katipoTable->setFunction("gotSiteUnchanged", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            std::string hostID = args->arrayObjects[0]->getStringValue();
            SiteConnectionInfo& siteConnectionInfo = siteConnectionInfosByHostID[hostID];
            TuiFunction* onSiteLoadFunc = siteConnectionInfo.katipoTable->getFunction("onSiteLoad");
            if(onSiteLoadFunc)
            {
                onSiteLoadFunc->call("site loaded onSiteLoad", TUI_FALSE, siteConnectionInfo.publicData);
            }
        }
        
        return TUI_NIL;
    });*/
    
    //katipo.loadSite(siteSavePath, untrustedSiteCodePermissionFunction, publicDataOrNil)
    katipoTable->setFunction("loadSite", [this](TuiTable* args,
                                                TuiRef* existingResult,
                                                TuiFunctionCallData* incomingCallData,
                                                TuiDebugInfo* callingDebugInfo) -> TuiRef* { //note added outside function above so for now sites can't load other sites
        if(args->arrayObjects.size() >= 3)
        {
            std::string hostID = args->arrayObjects[0]->getStringValue();
            std::string siteSavePath = args->arrayObjects[1]->getStringValue();
            TuiRef* publicData = args->arrayObjects[2];
            
            if(currentSiteView)
            {
                currentSiteView->stateTable->setBool("hidden", TUI_TRUE);
                currentSiteView = nullptr;
            }
            
            //katipo.loadSite(bookmarkInfo.hostID, siteSavePath, untrustedSiteCodePermissionFunction, isDustyOldCacheLoad)
            bool isDustyOldCacheLoad = false;
            if(args->arrayObjects.size() >= 5)
            {
                isDustyOldCacheLoad = args->arrayObjects[4]->boolValue();
            }
            SiteConnectionInfo& siteConnectionInfo = siteConnectionInfosByHostID[hostID];
            
            if(siteConnectionInfo.publicData)
            {
                siteConnectionInfo.publicData->release();
                siteConnectionInfo.publicData = nullptr;
            }
            if(publicData)
            {
                siteConnectionInfo.publicData = publicData->retain();
            }
            
            if(!siteConnectionInfo.rootTable)
            {
                TuiFunction* permissionCallbackFunction = nullptr;
                if(args->arrayObjects.size() >= 4)
                {
                    TuiRef* arg = args->arrayObjects[3];
                    if(arg->type() == Tui_ref_type_FUNCTION)
                    {
                        permissionCallbackFunction = (TuiFunction*)arg;
                    }
                }
                
                siteConnectionInfo.rootTable = Tui::initSafeRootTable(permissionCallbackFunction, siteSavePath);
                MJAudio::getInstance()->bindTui(siteConnectionInfo.rootTable);
                EventManager::getInstance()->bindTui(siteConnectionInfo.rootTable);
                TuiTable* siteKatipoTable = addKatipoTable(siteConnectionInfo.rootTable);
                siteConnectionInfo.katipoTable = siteKatipoTable;
                
                appDatabase->bindTui(hostID, siteConnectionInfo.rootTable);

                
                //from within site code: katipo.get("example", sendData, function(result){ print("got result:", result)})
                //NOTE get requests from within site code can only currently request from that same server
                //todo for now we assume that, but we need to add another option to get in the same way as the base method
                siteConnectionInfo.rootTable->getTable("katipo")->setFunction("get", [this, hostID](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
                    if(args->arrayObjects.size() >= 1)
                    {
                        TuiRef* urlRef = args->arrayObjects[0];
                        if(urlRef->type() == Tui_ref_type_STRING)
                        {
                            if(siteConnectionInfosByHostID.count(hostID) != 0)
                            {
                                SiteConnectionInfo& siteConnectionInfo = siteConnectionInfosByHostID[hostID];
                                //todo use stuff in siteConnectionInfosByHostID
                                std::string remoteURL = urlRef->getStringValue();
                                doGet(siteConnectionInfo.trackerKey, hostID, remoteURL, siteConnectionInfo.hostName, args);
                            }
                            
                        }
                    }
                    return TUI_NIL;
                });
                
                //this forwards the site's call to it's katipo.linkClicked(url) function on to the main katipoBrowser/code.tui linkClicked function
                siteConnectionInfo.rootTable->getTable("katipo")->setFunction("linkClicked", [this, hostID](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
                    if(args->arrayObjects.size() >= 1)
                    {
                        TuiRef* urlRef = args->arrayObjects[0];
                        if(urlRef->type() == Tui_ref_type_STRING)
                        {
                            if(currentSiteView)
                            {
                                currentSiteView->fadeOut();
                                currentSiteView = nullptr;
                            }
                            TuiFunction* mainLinkClickedFunction = katipoTable->getFunction("onLinkClick");
                            mainLinkClickedFunction->call("onLinkClick", urlRef);
                            
                        }
                    }
                    return TUI_NIL;
                });
                
                siteConnectionInfo.rootTable->getTable("file")->setFunction("getSavePath", [siteSavePath](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
                    if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
                    {
                        const std::string& appendPath = args->arrayObjects[0]->getStringValue();
                        return new TuiString(siteSavePath + "/" + appendPath);
                    }
                    return new TuiString(siteSavePath + "/");
                });
                
                siteConnectionInfo.rootTable->getTable("file")->setFunction("getResourcePath", [this, siteSavePath, hostID](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
                    if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
                    {
                        const std::string& appendPath = args->arrayObjects[0]->getStringValue();
                        std::string siteResourcePath = siteSavePath + "/" + appendPath;
                        
                        SiteConnectionInfo& siteConnectionInfoReloaded = siteConnectionInfosByHostID[hostID];
                        
                        if(siteConnectionInfoReloaded.checkedResourceFiles.count(appendPath) == 0)
                        {
                            if(Tui::fileExistsAtPath(siteResourcePath))
                            {
                                siteConnectionInfoReloaded.checkedResourceFiles[appendPath] = true;
                                return new TuiString(siteResourcePath); //todo ensure within allowed dirs
                            }
                            siteConnectionInfoReloaded.checkedResourceFiles[appendPath] = false;
                        }
                        else if(siteConnectionInfoReloaded.checkedResourceFiles[appendPath])
                        {
                            return new TuiString(siteResourcePath);
                        }
                        
                        return new TuiString(Katipo::getResourcePath(appendPath)); //todo ensure within allowed dirs
                    }
                    return new TuiString(Katipo::getResourcePath(siteSavePath + "/")); //todo ensure within allowed dirs
                });
            }
            
            if(!EventManager::getInstance()->currentHostID.empty() && hostID != EventManager::getInstance()->currentHostID)
            {
                SiteConnectionInfo& prevSiteInfo = siteConnectionInfosByHostID[EventManager::getInstance()->currentHostID];
                TuiFunction* onBackgroundChange = prevSiteInfo.katipoTable->getFunction("onBackgroundChange");
                if(onBackgroundChange)
                {
                    onBackgroundChange->call("onBackgroundChange", TUI_TRUE);
                }
            }
            
            EventManager::getInstance()->currentHostID = hostID;
            EventManager::getInstance()->currentKatipoTable = siteConnectionInfo.katipoTable;
            
            std::string siteScenePath = siteSavePath + "/scripts/scene.tui";
            if(!Tui::fileExistsAtPath(siteScenePath))
            {
                siteScenePath = siteSavePath + "/scene.tui";
                if(!Tui::fileExistsAtPath(siteScenePath))
                {
                    siteScenePath = Katipo::getResourcePath("app/katipoBrowser/scripts/defaultSiteScene.tui");
                }
            }
            
            TuiTable* sceneTable = (TuiTable*)TuiRef::runScriptFile(siteScenePath, siteConnectionInfo.rootTable);
            siteConnectionInfo.rootTable->setTable("scene", sceneTable);
            sceneTable->release();
            
            sceneTable->setFunction("getView", [this, hostID](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
                if(args->arrayObjects.size() >= 1)
                {
                    TuiRef* viewNameRef = args->arrayObjects[0];
                    if(viewNameRef->type() == Tui_ref_type_STRING && siteConnectionInfosByHostID.count(hostID) != 0)
                    {
                        SiteConnectionInfo& siteConnectionInfo = siteConnectionInfosByHostID[hostID];
                        if(((TuiString*)viewNameRef)->value == siteConnectionInfo.mainView->idString)
                        {
                            return siteConnectionInfo.mainView->stateTable->retain();
                        }
                        MJView* subView = siteConnectionInfo.mainView->getSubViewWithID(((TuiString*)viewNameRef)->value);
                        if(subView)
                        {
                            return subView->stateTable->retain();
                        }
                    }
                }
                return TUI_NIL;
            });
            
            if(siteConnectionInfo.mainView)
            {
                MainController::getInstance()->mainMJView->getSubViewWithID("siteContent")->removeSubview(siteConnectionInfo.mainView);
                siteConnectionInfo.mainView = nullptr;
                if(siteConnectionInfo.scriptState)
                {
                    siteConnectionInfo.scriptState->release();
                    siteConnectionInfo.scriptState = nullptr;
                }
            }
            
            siteConnectionInfo.mainView = MJView::loadUnknownViewFromTable(sceneTable->getTable("mainView"), MainController::getInstance()->mainMJView->getSubViewWithID("siteContent"), true, siteConnectionInfo.rootTable);
            currentSiteView = siteConnectionInfo.mainView;
            currentSiteView->alpha = 0.0;
            currentSiteView->fadeIn();
            
            std::string siteMarkupPath = siteSavePath + "/index.tml";
            if(Tui::fileExistsAtPath(siteMarkupPath))
            {
                MJView* pageBodyView = siteConnectionInfo.mainView->getSubViewWithID("pageBody");
                if(pageBodyView)
                {
                    TuiFunction* setTextFunction = pageBodyView->stateTable->getFunction("setText");
                    if(setTextFunction)
                    {
                        TuiString* fileContentsString = new TuiString("");
                        Tui::getFileContents(siteMarkupPath, &fileContentsString->value);
                        setTextFunction->call("setting index.tml", fileContentsString);
                        fileContentsString->release();
                    }
                    else
                    {
                        MJError("pageBody view type must implement setText:%s", siteScenePath.c_str());
                    }
                }
                else
                {
                    MJError("No pageBody found in scene file:%s", siteScenePath.c_str());
                }
            }
            
            std::string siteCodePath = siteSavePath + "/scripts/code.tui";
            if(Tui::fileExistsAtPath(siteCodePath))
            {
                siteConnectionInfo.scriptState = (TuiTable*)TuiRef::runScriptFile(siteCodePath, siteConnectionInfo.rootTable);
            }
            
            
            //if(!isDustyOldCacheLoad)
            //{
                TuiFunction* onSiteLoadFunc = siteConnectionInfo.katipoTable->getFunction("onSiteLoad");
                if(onSiteLoadFunc)
                {
                    MJLog("calling site loaded onSiteLoad");
                    onSiteLoadFunc->call("site loaded onSiteLoad", TUI_BOOL(!isDustyOldCacheLoad), siteConnectionInfo.publicData);
                }
            //}
            /*else
            {
                TuiFunction* onConnectionFailedFunc = siteConnectionInfo.katipoTable->getFunction("onConnectionFailed");
                if(onConnectionFailedFunc)
                {
                    onConnectionFailedFunc->call("site loaded onConnectionFailed");
                }
            }*/
        }
        return TUI_NIL;
    });
    
    //katipo.get("127.0.0.1/example", sendData, function(result){ print("got result:", result)})
    katipoTable->setFunction("get", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
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
                    
                    if(split.size() > 1)
                    {
                        remoteURL = remoteURL.substr(split[0].length() + 1, -1);
                        hostName = split[1];
                    }
                    else
                    {
                        hostName = "";
                    }
                }
                
                std::string trackerKey = trackerURL + ":" + trackerPort;
                ClientNetInterface* netInterface = nullptr;
                if(netInterfaces.count(trackerKey) != 0)
                {
                    if(netInterfaces[trackerKey]->connected)
                    {
                        doGet(trackerKey, "", remoteURL, hostName, args);
                    }
                    else if(!netInterfaces[trackerKey]->connectedOrConnecting())
                    {
                        TuiTable* getArgs = args;
                        getArgs->retain();

                        TuiFunction* onConnect = new TuiFunction([this, trackerKey, remoteURL, hostName, getArgs](TuiTable* innerFuncArgs, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
                            //todo check for connection success
                            doGet(trackerKey, "", remoteURL, hostName, getArgs);
                            getArgs->release();
                            return TUI_NIL;
                        });

                        katipoTable->set("onConnected", onConnect);
                        katipoTable->set("onConnectionFailed", onConnect); //onConnect will call the main callback with an error if not connected
                        onConnect->release();

                        netInterfaces[trackerKey]->connect();
                    }
                    else
                    {
                        MJError("attempt to call get while connecting"); //todo we could queue it up instead
                    }
                }
                else
                {
                    std::string publicKey = "";
                    std::string secretKey = "";
                    
                    std::string clientKeyPath = Katipo::getSavePath("client_privateKey.tuib");
                    
                    if(Tui::fileExistsAtPath(clientKeyPath))
                    {
                        TuiTable* saveData = (TuiTable*)TuiRef::loadBinary(clientKeyPath);
                        if(saveData)
                        {
                            publicKey = saveData->getString("publicKey");
                            secretKey = saveData->getString("secretKey");
                            saveData->release();
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

                    TuiFunction* onConnect = new TuiFunction([this, trackerKey,remoteURL, hostName, getArgs](TuiTable* innerFuncArgs, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
                        //todo check for connection success
                        doGet(trackerKey, "", remoteURL, hostName, getArgs);
                        getArgs->release();
                        return TUI_NIL;
                    });
                    
                    katipoTable->set("onConnected", onConnect);
                    katipoTable->set("onConnectionFailed", onConnect); //onConnect will call the main callback with an error if not connected
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
    
    //katipo.setPauseSiteContentForModalOverlay(pauseSite)
    katipoTable->setFunction("setPauseSiteContentForModalOverlay", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            bool paused = args->arrayObjects[0]->boolValue();
            for(auto& kv : siteConnectionInfosByHostID)
            {
                if(kv.second.katipoTable)
                {
                    if(kv.second.katipoTable->getBool("paused") != paused)
                    {
                        kv.second.katipoTable->set("paused", TUI_BOOL(paused));
                        TuiFunction* setPausedFunction = kv.second.katipoTable->getFunction("setPaused");
                        if(setPausedFunction)
                        {
                            setPausedFunction->call(incomingCallData,callingDebugInfo,TUI_BOOL(paused));
                        }
                    }
                }
            }
            
        }
        return TUI_NIL;
    });
    
    TuiTable* sceneTable = (TuiTable*)TuiRef::runScriptFile(Katipo::getResourcePath("app/katipoBrowser/scripts/scene.tui"), rootTable);
    rootTable->setTable("scene", sceneTable);
    sceneTable->release();
    
    sceneTable->setFunction("getView", [this](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 1)
        {
            TuiRef* viewNameRef = args->arrayObjects[0];
            if(viewNameRef->type() == Tui_ref_type_STRING)
            {
                if(((TuiString*)viewNameRef)->value == mainView->idString)
                {
                    return mainView->stateTable->retain();
                }
                MJView* subView = mainView->getSubViewWithID(((TuiString*)viewNameRef)->value);
                if(subView)
                {
                    return subView->stateTable->retain();
                }
            }
        }
        return TUI_NIL;
    });
    
    mainView = MJView::loadUnknownViewFromTable(sceneTable->getTable("mainView"), MainController::getInstance()->mainMJView, true, rootTable);
    
    scriptState = (TuiTable*)TuiRef::runScriptFile(Katipo::getResourcePath("app/katipoBrowser/scripts/code.tui"), rootTable);
    
    //scanner = new Scanner();
    
    updateTimerID = MJTimer::getInstance()->addUpdateTimer([this](uint32_t timerID, float dt) {
        for(auto& idAndRequestInterface : netInterfaces)
        {
            idAndRequestInterface.second->pollNetEvents();
        }
        if(katipoTable)
        {
            TuiRef* updateFunc = katipoTable->get("update");
            if(updateFunc && updateFunc->type() == Tui_ref_type_FUNCTION)
            {
                TuiRef* dtRef = new TuiNumber(dt);
                ((TuiFunction*)updateFunc)->call("main update callback", dtRef);
                dtRef->release();
            }
        }
        
        for(auto& kv : siteConnectionInfosByHostID)
        {
            if(kv.second.katipoTable)
            {
                TuiRef* updateFunc = kv.second.katipoTable->get("update");
                if(updateFunc && updateFunc->type() == Tui_ref_type_FUNCTION)
                {
                    TuiRef* dtRef = new TuiNumber(dt);
                    ((TuiFunction*)updateFunc)->call("main update callback", dtRef);
                    dtRef->release();
                }
            }
        }
        
        //scanner->update();
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
