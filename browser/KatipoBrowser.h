
#ifndef KatipoBrowser_h
#define KatipoBrowser_h

#include <map>
#include <vector>

#include "TuiScript.h"

class MainController;
class MainMenu;
class MJCache;
class MJView;
class TuiRef;
class TuiFunction;
class TuiTable;
class ClientNetInterface;

class KatipoBrowser {
public:
    MainMenu* mainMenu;
    MJView* mainView;
    TuiTable* rootTable;
    TuiTable* katipoTable;
    TuiTable* scriptState;
    
    std::map<std::string, ClientNetInterface*> netInterfaces;
    
    MJCache* cache;
    
    
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
