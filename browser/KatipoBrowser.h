
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
class Scanner;
class BrowserHost;
class BrowserTracker;

struct SiteConnectionInfo {
    MJView* mainView = nullptr;
    tui::table rootTable = nullptr;
    tui::table katipoTable = nullptr;
    tui::table scriptState = nullptr;
    tui::ref publicData = nullptr;
    std::string trackerKey;
    std::string hostName;
    std::map<std::string, bool> checkedResourceFiles;
};

class KatipoBrowser {
public:
    MJView* mainView = nullptr;
    tui::table rootTable = nullptr;
    tui::table scriptState = nullptr;
    tui::table katipoTable = nullptr;
    
    DatabaseEnvironment* appDatabaseEnvironment = nullptr;
    Database* appDatabase = nullptr;
    
    Scanner* scanner = nullptr;
    
    std::map<std::string, ClientNetInterface*> netInterfaces;
    
    std::map<std::string, SiteConnectionInfo> siteConnectionInfosByHostID;
    
    BrowserTracker* browserTracker = nullptr;
    std::map<std::string, BrowserHost*> browserHostsByHostID;
    
    MJView* currentSiteView = nullptr;
    
    uint32_t updateTimerID = 0;

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
               const tui::table& args);

private:

};

#endif // !KatipoBrowser_h
