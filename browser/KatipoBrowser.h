
#ifndef KatipoBrowser_h
#define KatipoBrowser_h

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

class KatipoBrowser {
public:
    MJView* mainView;
    TuiTable* rootTable;
    TuiTable* scriptState;
    TuiTable* katipoTable;
    
    DatabaseEnvironment* appDatabaseEnvironment;
    Database* appDatabase;
    
    std::map<std::string, ClientNetInterface*> netInterfaces;
    
    std::map<std::string, SiteConnectionInfo> siteConnectionInfosByHostID;
    
    uint32_t updateTimerID;

public:
    
    static KatipoBrowser* getInstance() {
        static KatipoBrowser* instance = new KatipoBrowser();
        return instance;
    }
    
    void init();
    
    KatipoBrowser();
    ~KatipoBrowser();
    
private:
    void doGet(const std::string& trackerKey,
               const std::string& hostID,
               const std::string& remoteURL,
               const std::string& hostName,
               TuiTable* args);

private:

};

#endif // !KatipoBrowser_h
