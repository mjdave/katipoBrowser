#ifndef BrowserTracker_h
#define BrowserTracker_h

#include "TuiScript.h"
#include <thread>

class Tracker;

class BrowserTracker {
    
public:
    
    std::thread* thread = nullptr;
    bool needsToExit = false;
    
   // DatabaseEnvironment* databaseEnvironment;
   // Database* database;
    
    TuiTable* rootTable;
    TuiTable* katipoTable;
    TuiTable* scriptState;
    
    BrowserTracker();
    ~BrowserTracker();
    
private:
    Tracker* tracker;
    //std::string basePath;
};

#endif
