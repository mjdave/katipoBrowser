
#ifdef _MSC_VER
#define _WINSOCKAPI_    // stops windows.h including winsock.h
#include <windows.h>
#include <direct.h>
#include <cstdint>
#endif

#include "Waraki.h"
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
#include "BrowserHost.h"
#include "BrowserTracker.h"

#include "MJView.h"

#define APP_NAME "Waraki"

void Waraki::doGet(const std::string& trackerKey,
                          const std::string& hostID,
                          const std::string& remoteURL,
                          const std::string& hostName,
                          const tui::table& args)
{
    
    tui::function mainGetCallbackFunction = nullptr;
    if(args && !args->arrayObjects.empty() && args->arrayObjects[args->arrayObjects.size() - 1]->type() == Tui_ref_type_FUNCTION)
    {
        mainGetCallbackFunction = tui::toFunction(args->arrayObjects[args->arrayObjects.size() - 1]);
    }
    
    std::string fullURL = trackerKey + "/" + remoteURL;
    
    if(netInterfaces.count(trackerKey) == 0 || !netInterfaces[trackerKey]->connected)
    {
        if(mainGetCallbackFunction)
        {
            tui::table statusResult = tui::newTable("{status='error',message='not connected'}");
            tui::string remoteURLString = tui::newString(fullURL);
            mainGetCallbackFunction->call("mainGetCallbackFunction", statusResult, TUI_NIL, remoteURLString);
        }
        return;
    }
    
    ClientNetInterface* netInterface = netInterfaces[trackerKey];
    tui::table remoteFuncCallArgs = tui::newTable();
    
    tui::string remoteURLStringForArg = tui::newString(remoteURL);
    remoteFuncCallArgs->arrayObjects.push_back(remoteURLStringForArg); //push_back, not retained, no need to release
    
    tui::function progressCallbackFunction = nullptr;
    
    for(int i = 1; i < args->arrayObjects.size(); i++)
    {
        if(args->arrayObjects[i]->type() != Tui_ref_type_FUNCTION)
        {
            remoteFuncCallArgs->arrayObjects.push_back(args->arrayObjects[i]);
        }
        
        if(args->arrayObjects.size() > 1 && args->arrayObjects[args->arrayObjects.size() - 2]->type() == Tui_ref_type_FUNCTION)
        {
            progressCallbackFunction = tui::toFunction(args->arrayObjects[args->arrayObjects.size() - 2]);
        }
    }
    
    MJLog("fetching from remote hostName:%s fullURL:%s", hostName.c_str(), fullURL.c_str());
    
    
    tui::function callHostFunctionCallbackFunction = tui::newFunction([mainGetCallbackFunction, fullURL](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(mainGetCallbackFunction)
        {
            if(args && args->arrayObjects.size() >= 2)
            {
                mainGetCallbackFunction->call("mainGetCallbackFunction", args->arrayObjects[0], args->arrayObjects[1], tui::newString(fullURL));
            }
            else
            {
                tui::ref statusResult = tui::newTable("{status='error',message='remote error'}");
                tui::string remoteURLString = tui::newString(fullURL);
                mainGetCallbackFunction->call("mainGetCallbackFunction", statusResult, TUI_NIL, remoteURLString);
            }
        }
        return TUI_NIL;
    });
    
    if(progressCallbackFunction)
    {
        remoteFuncCallArgs->push(progressCallbackFunction);
    }
    
    remoteFuncCallArgs->push(callHostFunctionCallbackFunction);
    
    tui::function getSiteKeyCallbackFunction = tui::newFunction([this, trackerKey, hostName, mainGetCallbackFunction, remoteFuncCallArgs, netInterface, fullURL](const tui::table& incomingCallbackResponseData, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
        
        if(incomingCallbackResponseData && !incomingCallbackResponseData->arrayObjects.empty())
        {
            tui::ref result = incomingCallbackResponseData->arrayObjects[0];
            
            if(result->type() == Tui_ref_type_TABLE && (tui::toTable(result)->hasKey("data") || tui::toTable(result)->getString("status") == "ok"))
            {
                //TODO check for host key locally tracker_hostname
                //abort on mismatch
                //save new host key
                
                tui::ref hostPublicKeyRef = tui::toTable(result)->get("publicKey");
                if(hostPublicKeyRef && hostPublicKeyRef->type() == Tui_ref_type_STRING)
                {
                    std::string hostSiteKey = TuiSHA1::sha1(trackerKey + hostName).substr(0,6) + "_" + hostName;
                    
                    netInterface->callRemoteHostFunction(hostSiteKey, tui::toString(hostPublicKeyRef)->value, remoteFuncCallArgs);
                    
                    SiteConnectionInfo& siteConnectionInfo = siteConnectionInfosByHostID[hostSiteKey];
                    siteConnectionInfo.trackerKey = trackerKey;
                    siteConnectionInfo.hostName = hostName;
                }
                else
                {
                    MJError("missing public key");
                    tui::table statusResult = tui::newTable("{status='error',message='missing public key'}");
                    tui::string remoteURLString = tui::newString(fullURL);
                    mainGetCallbackFunction->call("mainGetCallbackFunction", statusResult, TUI_NIL, remoteURLString);
                }
                
            }
            else
            {
                tui::string remoteURLString = tui::newString(fullURL);
                mainGetCallbackFunction->call("mainGetCallbackFunction", result, TUI_NIL, remoteURLString); //status not ok
            }
        }
        /*else //this causes a crash as we get another callback later... usually? all the time? Not sure yet
        {
            tui::ref statusResult = new TuiTable("{status='error',message='no connection'}");
            mainGetCallbackFunction->call("mainGetCallbackFunction", statusResult);
            mainGetCallbackFunction->release();
            mainGetCallbackFunction = nullptr;
            statusResult->release();
        }*/
        
        return TUI_NIL;
    });
    
    tui::table remoteHostKeyFuncCallArgs = tui::newTable();
    remoteHostKeyFuncCallArgs->pushString("getSiteKey");
    remoteHostKeyFuncCallArgs->pushString(hostName);
    remoteHostKeyFuncCallArgs->push(getSiteKeyCallbackFunction);
    
    netInterface->callTrackerFunction(remoteHostKeyFuncCallArgs);
}

tui::table addKatipoTable(const tui::table& rootTableToAddTo)
{
    tui::table katipoTableResult = tui::newTable(rootTableToAddTo);
    rootTableToAddTo->set("katipo", katipoTableResult);
    katipoTableResult->setString("savePath", Katipo::getSavePath());
    
    return katipoTableResult;
}

void getLocalKeys(std::string* publicKey, std::string* secretKey)
{
    
    std::string clientKeyPath = Katipo::getSavePath("client_privateKey.tuib");
    
    if(tui::fileExistsAtPath(clientKeyPath))
    {
        tui::table saveData = tui::toTable(TuiRef::loadBinary(clientKeyPath));
        if(saveData)
        {
            *publicKey = saveData->getString("publicKey");
            *secretKey = saveData->getString("secretKey");
            return;
        }
    }
    
    publicKey->resize(crypto_box_PUBLICKEYBYTES);
    secretKey->resize(crypto_box_SECRETKEYBYTES);
    crypto_box_keypair((unsigned char*)&((*publicKey)[0]), (unsigned char*)&((*secretKey)[0]));
    
    tui::table saveData = tui::newTable();
    
    saveData->setString("publicKey", *publicKey);
    saveData->setString("secretKey", *secretKey);
    
    saveData->saveBinary(clientKeyPath);
    MJLog("Generated and saved new private key:\n%s.\nPlease backup this file and keep it safe and secure!", tui::getAbsolutePath(clientKeyPath).c_str());
}

void sdlFileOpenCallback(void *userdata, const char * const *filelist, int filter)
{
    tui::table fileListTable = nullptr;
    
    if(!filelist)
    {
         MJWarn("SDL_ShowOpenFolderDialog: An error occured:%s", SDL_GetError());
    }
    else if(*filelist)
    {
        fileListTable = tui::newTable();
        for(int fileListIndex = 0; filelist[fileListIndex]; fileListIndex++)
        {
            fileListTable->insert(fileListIndex, tui::newString(filelist[fileListIndex]));
        }
    }
    
    ((TuiFunction*)userdata)->call("sdlFileOpenCallback", fileListTable);
}

void Waraki::init()
{
    Katipo::baseSavePath = SDL_GetPrefPath("katipobrowser", "waraki");
    
    if(sodium_init() < 0) //this is safe to call multiple times
    {
        MJError("Sodium initialization failed. Exiting.");
        abort();
    }
    
    tui::table rootTable = tui::getRootTable();
    appDatabaseEnvironment = new DatabaseEnvironment(Katipo::getSavePath("database"),
                                                     1,
                                                     2);
    appDatabase = new Database(appDatabaseEnvironment, "app");
    appDatabase->bindTui("", rootTable);
    
    
    rootTable->getTable("file")->setFunction("getSavePath", [rootTable](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return tui::newString(Katipo::getSavePath(args->arrayObjects[0]->getStringValue()));
        }
        return tui::newString(Katipo::getSavePath());
    });
    
    rootTable->getTable("file")->setFunction("getResourcePath", [](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            const std::string& appendPath = args->arrayObjects[0]->getStringValue();
            return tui::newString(Katipo::getResourcePath(appendPath));
        }
        return tui::newString(Katipo::getResourcePath());
    });
    
    //rootTable->setVec2("screenSize", dvec2(MainController::getInstance()->windowInfo->screenWidth, MainController::getInstance()->windowInfo->screenHeight));
    
    
    MainController::getInstance()->init(rootTable, appDatabase, APP_NAME);
    
    
    EventManager::getInstance()->bindTui(rootTable);
    MJAudio::getInstance()->bindTui(rootTable);
    katipoTable = addKatipoTable(rootTable);
    
    
    katipoTable->setFunction("openURL", [this](const tui::table& args,
                                                const tui::ref& existingResult,
                                                TuiFunctionCallData* incomingCallData,
                                                TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 1)
        {
            std::string url = args->arrayObjects[0]->getStringValue();
            SDL_OpenURL(url.c_str());
        }
        return TUI_NIL;
    });
    
    katipoTable->setFunction("setBundledHostRunning", [this, rootTable](const tui::table& args,
                                                const tui::ref& existingResult,
                                                TuiFunctionCallData* incomingCallData,
                                                TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 2)
        {
            std::string hostDirName = args->arrayObjects[0]->getStringValue();
            bool shouldRun = args->arrayObjects[1]->boolValue();
            
            if(!hostDirName.empty())
            {
                if(browserHostsByHostID.count(hostDirName) == 0)
                {
                    if(shouldRun)
                    {
                        if(!browserTracker)
                        {
                            browserTracker = new BrowserTracker(Katipo::getResourcePath("tracker.tui"));
                        }
                        tui::table userConfiguration = nullptr;
                        if(args->arrayObjects.size() >= 3 && args->arrayObjects[2]->type() == Tui_ref_type_TABLE)
                        {
                            userConfiguration = tui::toTable(args->arrayObjects[2]);
                        }
                        
                        BrowserHost* host = new BrowserHost(hostDirName,
                                                            Katipo::getResourcePath("waraki-site"),
                                                            Katipo::getResourcePath("host.tui"),
                                                            userConfiguration);
                        
                        if(host->loadSuccess)
                        {
                            browserHostsByHostID[hostDirName] = host;
                            return TUI_TRUE;
                        }
                        else
                        {
                            delete host;
                            return TUI_FALSE;
                        }
                    }
                }
                else
                {
                    if(!shouldRun)
                    {
                        delete browserHostsByHostID[hostDirName];
                        browserHostsByHostID.erase(hostDirName);
                        return TUI_FALSE;
                    }
                    return TUI_TRUE;
                }
                
            }
            
        }
        return TUI_FALSE;
    });
    
    katipoTable->setFunction("hideCurrentSite", [this](const tui::table& args,
                                                const tui::ref& existingResult,
                                                TuiFunctionCallData* incomingCallData,
                                                TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(currentSiteView)
        {
            currentSiteView->stateTable->setBool("hidden", true);
            currentSiteView = nullptr;
        }
        return TUI_NIL;
    });
    
    // START WARAKI, NEED TO PORT PROBABLY
    
    
    katipoTable->setFunction("openFileDialog", [this](const tui::table& args,
                                                const tui::ref& existingResult,
                                                TuiFunctionCallData* incomingCallData,
                                                TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 2)
        {
            std::string initialPath = args->arrayObjects[0]->getStringValue();
            
            SDL_ShowOpenFileDialog(sdlFileOpenCallback,
                                   args->arrayObjects[1].get(),
                  MainController::getInstance()->displayWindow,
                                   nullptr, 0,
                                     initialPath.c_str(),
                  false); //allow many
        }
        return TUI_NIL;
    });
    
    katipoTable->setFunction("openFolderDialog", [this](const tui::table& args,
                                                const tui::ref& existingResult,
                                                TuiFunctionCallData* incomingCallData,
                                                TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 2)
        {
            std::string initialPath = args->arrayObjects[0]->getStringValue();
            
            SDL_ShowOpenFolderDialog(sdlFileOpenCallback,
                                     args->arrayObjects[1].get(),
                  MainController::getInstance()->displayWindow,
                                     initialPath.c_str(),
                  false); //allow many
        }
        return TUI_NIL;
    });
    
    
    katipoTable->setFunction("startScan", [this](const tui::table& args,
                                                const tui::ref& existingResult,
                                                TuiFunctionCallData* incomingCallData,
                                                TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 2 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_FUNCTION)
        {
            if(!scanner)
            {
                std::string publicKey = "";
                std::string secretKey = "";
                getLocalKeys(&publicKey, &secretKey);
                scanner = new Scanner(publicKey, secretKey, katipoTable);
            }
            
            scanner->startScan(args->arrayObjects[0]->getStringValue(), tui::toFunction(args->arrayObjects[1]));
            
        }
        return TUI_NIL;
    });
    
    
    katipoTable->setFunction("connectToScanResult", [this](const tui::table& args,
                                                const tui::ref& existingResult,
                                                TuiFunctionCallData* incomingCallData,
                                                TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            //scanner
            
            std::string trackerKey = args->arrayObjects[0]->getStringValue();
            
            ScannerConnection connection = scanner->getConnection(trackerKey); //we are now responsible for closing the connection
            if(!connection.netInterface)
            {
                MJError("invalid connection");
                return TUI_NIL;
            }
            netInterfaces[trackerKey] = connection.netInterface;
            
            if(scanner)
            {
                delete scanner;
                scanner = nullptr;
            }
            
        }
        return TUI_NIL;
    });
    
    katipoTable->setFunction("registerAppFunction", [this](const tui::table& args,
                                                const tui::ref& existingResult,
                                                TuiFunctionCallData* incomingCallData,
                                                TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 2 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            tui::table registeredAppFunctionsTable = katipoTable->getTable("registeredAppFunctions");
            if(!registeredAppFunctionsTable)
            {
                registeredAppFunctionsTable = tui::newTable();
                katipoTable->setTable("registeredAppFunctions", registeredAppFunctionsTable);
            }
            registeredAppFunctionsTable->set(args->arrayObjects[0]->getStringValue(), args->arrayObjects[1]);
        }
        
        return TUI_NIL;
    });
    
    // END WARAKI
    
    //katipo.loadSite(siteSavePath, untrustedSiteCodePermissionFunction, publicDataOrNil)
    katipoTable->setFunction("loadSite", [this](const tui::table& args,
                                                const tui::ref& existingResult,
                                                TuiFunctionCallData* incomingCallData,
                                                TuiDebugInfo* callingDebugInfo) -> tui::ref { //note added outside function above so for now sites can't load other sites
        if(args->arrayObjects.size() >= 3)
        {
            std::string hostID = args->arrayObjects[0]->getStringValue();
            std::string siteSavePath = args->arrayObjects[1]->getStringValue();
            tui::ref publicData = args->arrayObjects[2];
            
            
            if(!tui::fileExistsAtPath(siteSavePath))
            {
                TuiWarn("No cached site files found at:%s", siteSavePath.c_str());
                return TUI_FALSE;
            }
            
            if(currentSiteView)
            {
                currentSiteView->stateTable->setBool("hidden", true);
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
                siteConnectionInfo.publicData = nullptr;
            }
            if(publicData)
            {
                siteConnectionInfo.publicData = publicData;
            }
            
            if(!siteConnectionInfo.rootTable)
            {
                tui::function permissionCallbackFunction = nullptr;
                if(args->arrayObjects.size() >= 4)
                {
                    tui::ref arg = args->arrayObjects[3];
                    if(arg->type() == Tui_ref_type_FUNCTION)
                    {
                        permissionCallbackFunction = tui::toFunction(arg);
                    }
                }
                
                siteConnectionInfo.rootTable = tui::initSafeRootTable(permissionCallbackFunction, siteSavePath);
                MJAudio::getInstance()->bindTui(siteConnectionInfo.rootTable);
                EventManager::getInstance()->bindTui(siteConnectionInfo.rootTable);
                tui::table siteKatipoTable = addKatipoTable(siteConnectionInfo.rootTable);
                siteConnectionInfo.katipoTable = siteKatipoTable;
                
                
                // WARAKI SPECIFIC HACKERY to allow direct local playback without having to create a cache of music files
                if(siteConnectionInfo.trackerKey.find("127.0.0.1") == 0 && !browserHostsByHostID.empty())
                {
                    BrowserHost* host = browserHostsByHostID.begin()->second; //we can only host one for now
                    std::string musicDirectoryPath = host->scriptState->getString("musicDirectoryPath");
                    siteKatipoTable->setString("warakiBundledHostLocalSongPath", musicDirectoryPath + "/");
                    //warakiBundledHostLocalSongPath
                }
                // END WARAKI SPECIFIC
                
                appDatabase->bindTui(hostID, siteConnectionInfo.rootTable);

                
                //from within site code: katipo.get("example", sendData, function(result){ print("got result:", result)})
                //NOTE get requests from within site code can only currently request from that same server
                //todo for now we assume that, but we need to add another option to get in the same way as the base method
                siteConnectionInfo.rootTable->getTable("katipo")->setFunction("get", [this, hostID](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
                    if(args->arrayObjects.size() >= 1)
                    {
                        tui::ref urlRef = args->arrayObjects[0];
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
                siteConnectionInfo.rootTable->getTable("katipo")->setFunction("linkClicked", [this, hostID](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
                    if(args->arrayObjects.size() >= 1)
                    {
                        tui::ref urlRef = args->arrayObjects[0];
                        if(urlRef->type() == Tui_ref_type_STRING)
                        {
                            if(currentSiteView)
                            {
                                currentSiteView->fadeOut();
                                currentSiteView = nullptr;
                            }
                            katipoTable->getFunction("onLinkClick")->call("onLinkClick", urlRef);
                            
                        }
                    }
                    return TUI_NIL;
                });
                
                
                //this allows the site/clientSite code to call functions in the main katipoBrowser/code.tui enviornment.
                //todo linkClicked could just use this instead
                siteConnectionInfo.rootTable->getTable("katipo")->setFunction("callAppFunction", [this](const tui::table& args,
                                                            const tui::ref& existingResult,
                                                            TuiFunctionCallData* incomingCallData,
                                                            TuiDebugInfo* callingDebugInfo) -> tui::ref {
                    if(args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
                    {
                        tui::table registeredAppFunctionsTable = katipoTable->getTable("registeredAppFunctions");
                        if(registeredAppFunctionsTable)
                        {
                            tui::function func = registeredAppFunctionsTable->getFunction(args->arrayObjects[0]->getStringValue());
                            if(func)
                            {
                                tui::table functionArgs = tui::newTable();
                                for(int i = 1; i < args->arrayObjects.size(); i++)
                                {
                                    functionArgs->insert(functionArgs->arrayObjects.size() - 1, args->arrayObjects[i]);
                                }
                                tui::ref result = func->call(functionArgs, existingResult, incomingCallData, callingDebugInfo);
                                return result;
                            }
                            else
                            {
                                MJWarn("attempt to call unregistered app function:%s", args->arrayObjects[0]->getStringValue().c_str());
                            }
                        }
                    }
                    
                    return TUI_NIL;
                });
                
                siteConnectionInfo.rootTable->getTable("file")->setFunction("getSavePath", [siteSavePath](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
                    if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
                    {
                        const std::string& appendPath = args->arrayObjects[0]->getStringValue();
                        return tui::newString(siteSavePath + "/" + appendPath);
                    }
                    return tui::newString(siteSavePath + "/");
                });
                
                siteConnectionInfo.rootTable->getTable("file")->setFunction("getResourcePath", [this, siteSavePath, hostID](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
                    if(args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
                    {
                        const std::string& appendPath = args->arrayObjects[0]->getStringValue();
                        std::string siteResourcePath = siteSavePath + "/" + appendPath;
                        
                        SiteConnectionInfo& siteConnectionInfoReloaded = siteConnectionInfosByHostID[hostID];
                        
                        if(siteConnectionInfoReloaded.checkedResourceFiles.count(appendPath) == 0)
                        {
                            if(tui::fileExistsAtPath(siteResourcePath))
                            {
                                siteConnectionInfoReloaded.checkedResourceFiles[appendPath] = true;
                                return tui::newString(siteResourcePath); //todo ensure within allowed dirs
                            }
                            siteConnectionInfoReloaded.checkedResourceFiles[appendPath] = false;
                        }
                        else if(siteConnectionInfoReloaded.checkedResourceFiles[appendPath])
                        {
                            return tui::newString(siteResourcePath);
                        }
                        
                        return tui::newString(Katipo::getResourcePath(appendPath)); //todo ensure within allowed dirs
                    }
                    return tui::newString(Katipo::getResourcePath(siteSavePath + "/")); //todo ensure within allowed dirs
                });
            }
            
            if(!EventManager::getInstance()->currentHostID.empty() && hostID != EventManager::getInstance()->currentHostID)
            {
                SiteConnectionInfo& prevSiteInfo = siteConnectionInfosByHostID[EventManager::getInstance()->currentHostID];
                tui::function onBackgroundChange = prevSiteInfo.katipoTable->getFunction("onBackgroundChange");
                if(onBackgroundChange)
                {
                    onBackgroundChange->call("onBackgroundChange", TUI_TRUE);
                }
            }
            
            EventManager::getInstance()->currentHostID = hostID;
            EventManager::getInstance()->currentSiteKatipoTable = siteConnectionInfo.katipoTable;
            
            std::string siteScenePath = siteSavePath + "/scripts/scene.tui";
            if(!tui::fileExistsAtPath(siteScenePath))
            {
                siteScenePath = siteSavePath + "/scene.tui";
                if(!tui::fileExistsAtPath(siteScenePath))
                {
                    siteScenePath = Katipo::getResourcePath("app/katipoBrowser/scripts/defaultSiteScene.tui");
                }
            }
            
            tui::table sceneTable = tui::toTable(TuiRef::runScriptFile(siteScenePath, siteConnectionInfo.rootTable));
            siteConnectionInfo.rootTable->setTable("scene", sceneTable);
            
            sceneTable->setFunction("getView", [this, hostID](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
                if(args->arrayObjects.size() >= 1)
                {
                    tui::ref viewNameRef = args->arrayObjects[0];
                    if(viewNameRef->type() == Tui_ref_type_STRING && siteConnectionInfosByHostID.count(hostID) != 0)
                    {
                        SiteConnectionInfo& siteConnectionInfo = siteConnectionInfosByHostID[hostID];
                        if(viewNameRef->getStringValue() == siteConnectionInfo.mainView->idString)
                        {
                            return siteConnectionInfo.mainView->stateTable;
                        }
                        MJView* subView = siteConnectionInfo.mainView->getSubViewWithID(viewNameRef->getStringValue());
                        if(subView)
                        {
                            return subView->stateTable;
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
                    siteConnectionInfo.scriptState = nullptr;
                }
            }
            
            siteConnectionInfo.mainView = MJView::loadUnknownViewFromTable(sceneTable->getTable("mainView"), MainController::getInstance()->mainMJView->getSubViewWithID("siteContent"), true, siteConnectionInfo.rootTable);
            currentSiteView = siteConnectionInfo.mainView;
            currentSiteView->alpha = 0.0;
            currentSiteView->fadeIn();
            
            katipoTable->setTable("siteView", siteConnectionInfo.mainView->stateTable); //todo make this available in Koru too
            
            std::string siteMarkupPath = siteSavePath + "/index.tml";
            if(tui::fileExistsAtPath(siteMarkupPath))
            {
                MJView* pageBodyView = siteConnectionInfo.mainView->getSubViewWithID("pageBody");
                if(pageBodyView)
                {
                    tui::function setTextFunction = pageBodyView->stateTable->getFunction("setText");
                    if(setTextFunction)
                    {
                        tui::string fileContentsString = tui::newString("");
                        tui::getFileContents(siteMarkupPath, &fileContentsString->value);
                        setTextFunction->call("setting index.tml", fileContentsString);
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
            if(tui::fileExistsAtPath(siteCodePath))
            {
                siteConnectionInfo.scriptState = tui::toTable(TuiRef::runScriptFile(siteCodePath, siteConnectionInfo.rootTable));
            }
            
            
            //if(!isDustyOldCacheLoad)
            //{
                tui::function onSiteLoadFunc = siteConnectionInfo.katipoTable->getFunction("onSiteLoad");
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
        return TUI_TRUE;
    });
    
    //katipo.get("127.0.0.1/example", sendData, function(result){ print("got result:", result)})
    katipoTable->setFunction("get", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 1)
        {
            tui::ref urlRef = args->arrayObjects[0];
            if(urlRef->type() == Tui_ref_type_STRING)
            {
                std::string remoteURL = urlRef->getStringValue();
                std::vector<std::string> split = tui::splitString(remoteURL, '/');
                
                std::string trackerURL = "127.0.0.1";
                std::string trackerPort = "3471";
                std::string hostName = split[0];
                
                if(split[0].find(".") != -1)
                {
                    std::vector<std::string> portSplit = tui::splitString(split[0], ':');
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
                        remoteURL = "";
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
                        tui::table getArgs = args;

                        tui::function onConnect = tui::newFunction([this, trackerKey, remoteURL, hostName, getArgs](const tui::table& innerFuncArgs, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
                            //todo check for connection success
                            doGet(trackerKey, "", remoteURL, hostName, getArgs);
                            return TUI_NIL;
                        });

                        katipoTable->set("onConnected", onConnect);
                        katipoTable->set("onConnectionFailed", onConnect); //onConnect will call the main callback with an error if not connected

                        netInterfaces[trackerKey]->connect();
                    }
                    else
                    {
                        MJError("attempt to call get while connecting"); //todo we could queue it up instead
                    }
                }
                else
                {
                    for(auto& idAndRequestInterface : netInterfaces) //we may want to keep these connections open in some cases, but for now it is cleanest to kill them
                    {
                        delete idAndRequestInterface.second;
                    }
                    netInterfaces.clear();
                    
                    std::string publicKey = "";
                    std::string secretKey = "";
                    
                    getLocalKeys(&publicKey, &secretKey);
                    
                    tui::table getArgs = args;

                    tui::function onConnect = tui::newFunction([this, trackerKey,remoteURL, hostName, getArgs](const tui::table& innerFuncArgs, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
                        //todo check for connection success
                        doGet(trackerKey, "", remoteURL, hostName, getArgs);
                        return TUI_NIL;
                    });
                    
                    katipoTable->set("onConnected", onConnect);
                    katipoTable->set("onConnectionFailed", onConnect); //onConnect will call the main callback with an error if not connected
                    
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
    katipoTable->setFunction("setPauseSiteContentForModalOverlay", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
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
                        tui::function setPausedFunction = kv.second.katipoTable->getFunction("setPaused");
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
    
    tui::table sceneTable = tui::toTable(TuiRef::runScriptFile(Katipo::getResourcePath("app/katipoBrowser/scripts/scene.tui"), rootTable));
    rootTable->setTable("scene", sceneTable);
    
    sceneTable->setFunction("getView", [this](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(args->arrayObjects.size() >= 1)
        {
            tui::ref viewNameRef = args->arrayObjects[0];
            if(viewNameRef->type() == Tui_ref_type_STRING)
            {
                if(viewNameRef->getStringValue() == mainView->idString)
                {
                    return mainView->stateTable;
                }
                MJView* subView = mainView->getSubViewWithID(viewNameRef->getStringValue());
                if(subView)
                {
                    return subView->stateTable;
                }
            }
        }
        return TUI_NIL;
    });
    
    mainView = MJView::loadUnknownViewFromTable(sceneTable->getTable("mainView"), MainController::getInstance()->mainMJView, true, rootTable);
    
    scriptState = tui::toTable(TuiRef::runScriptFile(Katipo::getResourcePath("waraki-app/waraki.tui"), rootTable));
    
    updateTimerID = MJTimer::getInstance()->addUpdateTimer([this](uint32_t timerID, float dt) {
        for(auto& idAndRequestInterface : netInterfaces)
        {
            idAndRequestInterface.second->pollNetEvents();
        }
        if(katipoTable)
        {
            tui::function updateFunc = katipoTable->getFunction("update");
            if(updateFunc)
            {
                updateFunc->call("main update callback", tui::newNumber(dt));
            }
        }
        
        for(auto& kv : siteConnectionInfosByHostID)
        {
            if(kv.second.katipoTable)
            {
                tui::function updateFunc = kv.second.katipoTable->getFunction("update");
                if(updateFunc)
                {
                    updateFunc->call("main update callback", tui::newNumber(dt));
                }
            }
        }
        
        if(scanner)
        {
            scanner->update();
        }
    });
    
}


Waraki::Waraki()
{
    
}

Waraki::~Waraki()
{
    if(browserTracker)
    {
        delete browserTracker;
    }
    delete mainView;
}
