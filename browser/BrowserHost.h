#ifndef BrowserHost_h
#define BrowserHost_h

//#include "ThreadSafeQueue.h"
#include "TuiScript.h"
#include "KatipoUtilities.h"
#include <thread>

class ClientNetInterface;
class DatabaseEnvironment;
class Database;

class BrowserHost {
    
public:
    
    std::thread* thread = nullptr;
    bool needsToExit = false;
    bool loadSuccess = false;
    
    DatabaseEnvironment* databaseEnvironment;
    Database* database;
    
    ClientNetInterface* trackerNetInterface;
    
    tui::table rootTable;
    tui::table katipoTable;
    tui::table scriptState;
    
    BrowserHost(std::string hostDirName,
                std::string basePath_ = "",
                std::string hostScriptPath = Katipo::getResourcePath("host.tui"),
                const tui::table& userConfiguration = nullptr);
    ~BrowserHost();
    
private:
    std::string basePath;
    
    void createThread();
};

#endif
