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
    
    DatabaseEnvironment* databaseEnvironment;
    Database* database;
    
    ClientNetInterface* trackerNetInterface;
    
    TuiTable* rootTable;
    TuiTable* katipoTable;
    TuiTable* scriptState;
    
    BrowserHost(std::string hostID, std::string basePath_ = "", std::string hostScriptPath = Katipo::getResourcePath("host.tui"));
    ~BrowserHost();
    
private:
    std::string basePath;
    
    void createThread();
};

#endif
