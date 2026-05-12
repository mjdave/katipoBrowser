
#ifndef Waraki_h
#define Waraki_h

#include <map>
#include <vector>

#include "TuiScript.h"

class MainController;
class MJView;
class TuiRef;
class TuiFunction;
class TuiTable;
class ClientNetInterface;
class DatabaseEnvironment;
class Database;
class Scanner;
class BrowserHost;
class BrowserTracker;

struct SiteConnectionInfo {
    MJView* mainView = nullptr;
    TuiTable* rootTable = nullptr;
    TuiTable* katipoTable = nullptr;
    TuiTable* scriptState = nullptr;
    TuiRef* publicData = nullptr;
    std::string trackerKey;
    std::string hostName;
    std::map<std::string, bool> checkedResourceFiles;
};

class Waraki {
public:
    MJView* mainView;
    TuiTable* rootTable;
    TuiTable* scriptState;
    TuiTable* katipoTable;
    
    DatabaseEnvironment* appDatabaseEnvironment;
    Database* appDatabase;
    
    Scanner* scanner = nullptr;
    
    std::map<std::string, ClientNetInterface*> netInterfaces;
    
    std::map<std::string, SiteConnectionInfo> siteConnectionInfosByHostID;
    
    BrowserTracker* browserTracker = nullptr;
    std::map<std::string, BrowserHost*> browserHostsByHostID;
    
    MJView* currentSiteView = nullptr;
    
    uint32_t updateTimerID;

public:
    
    static Waraki* getInstance() {
        static Waraki* instance = new Waraki();
        return instance;
    }
    
    void init();
    
    Waraki();
    ~Waraki();
    
private:
    void doGet(const std::string& trackerKey,
               const std::string& hostID,
               const std::string& remoteURL,
               const std::string& hostName,
               TuiTable* args);

private:

};

#endif
