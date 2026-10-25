
#ifdef _MSC_VER
#define _WINSOCKAPI_    // stops windows.h including winsock.h
#include <windows.h>
#include <direct.h>
#include <cstdint>
#endif

#include "BrowserHost.h"
#include "Timer.h"
#include "MJVersion.h"

#include "ClientNetInterface.h"
#include "TuiFileUtils.h"
#include "Database.h"
#include "DatabaseEnvironment.h"
#include "sodium.h"
#include "KatipoUtilities.h"

#ifdef __APPLE__
#include "MJFileUtilsApple.h"
#endif

BrowserHost::BrowserHost(std::string hostDirName, std::string basePath_, std::string hostScriptPath, const tui::table& userConfiguration)
{
    basePath = basePath_;
    if(basePath.empty())
    {
        basePath = Katipo::getSavePath("hostedSites/" + hostDirName + "/public/");
    }
    std::string privateSavePath = Katipo::getSavePath("hostedSites/" + hostDirName + "/private/");
    
    rootTable = tui::initRootTable(); //todo this should optionally use a safe root table, probably by default
    
    katipoTable = tui::newTable(rootTable);
    rootTable->set("katipo", katipoTable);
    
    tui::table hostScriptState = tui::toTable(TuiRef::runScriptFile(hostScriptPath, rootTable));
    katipoTable->set("host", hostScriptState);
    
    katipoTable->setString("version", KATIPO_VERSION);
    
    databaseEnvironment = new DatabaseEnvironment(privateSavePath + "database",
                                                     1,
                                                     2);
    database = new Database(databaseEnvironment, "app");
    
    
    katipoTable->setString("basePath", basePath);
    katipoTable->setString("sitePath", basePath);
    katipoTable->setString("privateSavePath", privateSavePath);
    
    if(userConfiguration)
    {
        katipoTable->setTable("userConfiguration", userConfiguration);
    }
    
    
    katipoTable->setString("trackerIP", "127.0.0.1"); //todo
    katipoTable->setString("trackerPort", "3470"); //todo
    katipoTable->setBool("retry", true); //todo

    katipoTable->setFunction("init", [this, privateSavePath](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
        if(!trackerNetInterface)
        {
            //tui::table clientInfo = katipoTable->getTable("clientInfo");
            tui::table siteInfo = katipoTable->getTable("siteInfo");
            if(!siteInfo)
            {
                MJError("Invalid or missing siteInfo table");
                abort();
            }
            if(!siteInfo->hasKey("nameKey"))
            {
                MJError("siteInfo is missing a 'nameKey' entry ");
                abort();
            }
            
            std::string publicKey = "";
            std::string secretKey = "";
            
            MJLog("Loading site %s", siteInfo->getString("nameKey").c_str());
            
            std::string hostKeyPath = privateSavePath + "/" + siteInfo->getString("nameKey") + "_privateKey.tuib";
            
            if(tui::fileExistsAtPath(hostKeyPath)) //todo these should be saved in the database, not files
            {
                tui::table saveData = tui::toTable(TuiRef::loadBinary(hostKeyPath));
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
                
                tui::table saveData = tui::newTable();
                
                saveData->setString("publicKey", publicKey);
                saveData->setString("secretKey", secretKey);
                
                saveData->saveBinary(hostKeyPath);
            
                MJLog("Generated and saved new private key:\n%s.\nPlease backup this file and keep it safe and secure!", tui::getAbsolutePath(hostKeyPath).c_str());
            }
            else
            {
                MJLog("Loaded private key:\n%s", tui::getAbsolutePath(hostKeyPath).c_str());
            }
            
            siteInfo->setString("publicKey", publicKey);
            
            trackerNetInterface = new ClientNetInterface(katipoTable->get("trackerIP")->getStringValue(),
                                                    katipoTable->get("trackerPort")->getStringValue(),
                                                         publicKey, secretKey, siteInfo);
            trackerNetInterface->bindTui(katipoTable);

            
            
            std::string currentHostNameKey = siteInfo->getString("nameKey");
            
            database->bindTui(currentHostNameKey, rootTable);
    

            return TUI_TRUE;
        }
        return TUI_NIL;
    });
    
    katipoTable->setFunction("createSecurityBookmarkForDirectory", [this, privateSavePath](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
#ifdef __APPLE__
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            tui::string directoryPath = tui::toString(args->arrayObjects[0]);
            std::string bookmark = createSecurityBookmarkForDirectory(directoryPath->value);
            return tui::newString(bookmark);
        }
#endif
        return TUI_NIL;
    });
    
    
    katipoTable->setFunction("resolveSecurityBookmark", [this, privateSavePath](const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> tui::ref {
#ifdef __APPLE__
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            tui::string bookmarkData = tui::toString(args->arrayObjects[0]);
            std::string pathResult = resolveSecurityBookmark(bookmarkData->value);
            return tui::newString(pathResult);
        }
#endif
        return TUI_NIL;
    });
    
    scriptState = tui::toTable(TuiRef::runScriptFile(tui::pathByAppendingPathComponent(basePath,"code.tui"), rootTable));
    
    if(scriptState->getBool("loadSuccess"))
    {
        loadSuccess = true;
        thread = new std::thread(&BrowserHost::createThread, this);
    }
    else
    {
        needsToExit = true;
        return;
    }
    
}


BrowserHost::~BrowserHost()
{
    needsToExit = true;
    if(thread)
    {
        thread->join();
        delete thread;
    }
    
    delete database;
    delete databaseEnvironment;
    delete trackerNetInterface;
}

#define SERVER_FIXED_TIME_STEP 0.1

void BrowserHost::createThread()
{
    
    tui::function startFunction = tui::toTable(katipoTable->get("host"))->getFunction("start");
    tui::function updateFunction = tui::toTable(katipoTable->get("host"))->getFunction("update");
    startFunction->call("BrowserHost::BrowserHost()->start");
    
    Timer* timer = new Timer();
    Timer* deltaTimer = new Timer();
    
    tui::number dtNumRef = tui::newNumber(0.0);
    
    while(1)
    {
        if(needsToExit)
        {
            delete timer;
            delete deltaTimer;
            return;
        }
        //checkInput();
        
        double dt = std::clamp(deltaTimer->getDt(), 0.0, 4.0);
        
        dtNumRef->value = dt;
        
        updateFunction->call("update", dtNumRef);
        
        if(trackerNetInterface)
        {
            trackerNetInterface->pollNetEvents();
        }
        
        if(needsToExit)
        {
            delete timer;
            delete deltaTimer;
            return;
        }
        
        double timeElapsed = timer->getDt();
        if(timeElapsed < SERVER_FIXED_TIME_STEP)
        {
            std::this_thread::sleep_for(std::chrono::duration<double>(SERVER_FIXED_TIME_STEP - timeElapsed));
        }
    }
}
