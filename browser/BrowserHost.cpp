
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

BrowserHost::BrowserHost(std::string hostDirName, std::string basePath_, std::string hostScriptPath, TuiTable* userConfiguration)
{
    basePath = basePath_;
    if(basePath.empty())
    {
        basePath = Katipo::getSavePath("hostedSites/" + hostDirName + "/public/");
    }
    std::string privateSavePath = Katipo::getSavePath("hostedSites/" + hostDirName + "/private/");
    
    rootTable = Tui::initRootTable(); //todo this should optionally use a safe root table, probably by default
    
    katipoTable = new TuiTable(rootTable);
    rootTable->set("katipo", katipoTable);
    katipoTable->release();
    
    TuiRef* hostScriptState = (TuiTable*)TuiRef::runScriptFile(hostScriptPath, rootTable);
    katipoTable->set("host", hostScriptState);
    hostScriptState->release();
    
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

    katipoTable->setFunction("init", [this, privateSavePath](TuiTable* args, TuiRef* existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(!trackerNetInterface)
        {
            //TuiTable* clientInfo = katipoTable->getTable("clientInfo");
            TuiTable* siteInfo = katipoTable->getTable("siteInfo");
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
            
            if(Tui::fileExistsAtPath(hostKeyPath)) //todo these should be saved in the database, not files
            {
                TuiTable* saveData = (TuiTable*)TuiRef::loadBinary(hostKeyPath);
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
                
                saveData->saveBinary(hostKeyPath);
                saveData->release();
            
                MJLog("Generated and saved new private key:\n%s.\nPlease backup this file and keep it safe and secure!", Tui::getAbsolutePath(hostKeyPath).c_str());
            }
            else
            {
                MJLog("Loaded private key:\n%s", Tui::getAbsolutePath(hostKeyPath).c_str());
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
    
    
    scriptState = (TuiTable*)TuiRef::runScriptFile(Tui::pathByAppendingPathComponent(basePath,"code.tui"), rootTable);
    
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
    
    rootTable->release();
    scriptState->release();
    delete database;
    delete databaseEnvironment;
    delete trackerNetInterface;
}

#define SERVER_FIXED_TIME_STEP 0.1

void BrowserHost::createThread()
{
    
    TuiFunction* startFunction = ((TuiTable*)katipoTable->get("host"))->getFunction("start");
    TuiFunction* updateFunction = ((TuiTable*)katipoTable->get("host"))->getFunction("update");
    startFunction->call("BrowserHost::BrowserHost()->start");
    
    Timer* timer = new Timer();
    Timer* deltaTimer = new Timer();
    
    TuiNumber* dtNumRef = new TuiNumber(0.0);
    
    while(1)
    {
        if(needsToExit)
        {
            delete timer;
            delete deltaTimer;
            dtNumRef->release();
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
            dtNumRef->release();
            return;
        }
        
        double timeElapsed = timer->getDt();
        if(timeElapsed < SERVER_FIXED_TIME_STEP)
        {
            std::this_thread::sleep_for(std::chrono::duration<double>(SERVER_FIXED_TIME_STEP - timeElapsed));
        }
    }
}
