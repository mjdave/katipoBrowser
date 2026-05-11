#ifndef BrowserHost_h
#define BrowserHost_h

//#include "ThreadSafeQueue.h"
#include "TuiScript.h"
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
    
    BrowserHost(std::string hostID);
    ~BrowserHost();
    
private:
    std::string basePath;
    
    void createThread();
};

#endif
