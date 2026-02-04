
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

class KatipoBrowser {
public:
    MJView* mainView;
    TuiTable* rootTable;
    TuiTable* scriptState;
    TuiTable* katipoTable;
    
    std::map<std::string, ClientNetInterface*> netInterfaces;
    
    MJView* currentSiteMainView = nullptr;
    TuiTable* currentSiteRootTable = nullptr;
    TuiTable* currentSiteScriptState = nullptr;
    
    uint32_t updateTimerID;
    
   // std::map<int, std::vector<TuiCallback>> keyCallbackFunctionArraysByKeyCodes;

public:
    
    static KatipoBrowser* getInstance() {
        static KatipoBrowser* instance = new KatipoBrowser();
        return instance;
    }
    
    void init();
    
    KatipoBrowser();
    ~KatipoBrowser();
    
private:

private:

};

#endif // !KatipoBrowser_h
