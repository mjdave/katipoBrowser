#include "BrowserTracker.h"
//#include "Timer.h"
#include "MJVersion.h"

//#include "ClientNetInterface.h"
//#include "TuiFileUtils.h"
//#include "Database.h"
//#include "DatabaseEnvironment.h"
//#include "sodium.h"
#include "Tracker.h"

/*

 katipo.hostPort = launchArgs[i]
 katipo.clientPort = launchArgs[i]
 katipo.defaultHost = launchArgs[i]
 
 ### initialize katipo now that we have loaded the launch args
 success = katipo.init()

 if(!success)
 {
     print("Failed to initialize katipo.")
     return
 }

 tracker.init()

 ### start the servers
 success = katipo.start()
 
 */

/*

 std::string basePath = Tui::pathByRemovingLastPathComponent(argv[0]);
 katipoTable->setString("basePath", basePath);
 
 tracker = new Tracker(katipoTable);
 
 scriptState = (TuiTable*)TuiRef::runScriptFile(katipoTable->getString("basePath") + "/scripts/code.tui", katipoTable);
 
 
 */

BrowserTracker::BrowserTracker(std::string trackerScriptPath)
{
    //std::string basePath = Katipo::getSavePath("tracker/"); //will be needed
    //katipoTable->setString("basePath", basePath);
    
    rootTable = Tui::initRootTable(); //todo this should optionally use a safe root table, probably by default
    
    katipoTable = new TuiTable(rootTable);
    rootTable->set("katipo", katipoTable);
    katipoTable->release();
    
    TuiTable* trackerScriptState = (TuiTable*)TuiRef::runScriptFile(trackerScriptPath, rootTable);
    katipoTable->set("tracker", trackerScriptState);
    trackerScriptState->release();
    
    
    katipoTable->setString("version", KATIPO_VERSION);
    katipoTable->setString("hostPort", "3470");
    katipoTable->setString("clientPort", "3471");
    
    //katipoTable->setString("defaultHost", "notes"); //todo
    
    
    //todo
    /*databaseEnvironment = new DatabaseEnvironment(basePath + "database",
                                                     1,
                                                     2);
    database = new Database(databaseEnvironment, "app");*/
    
    
    tracker = new Tracker(katipoTable);
    
    TuiRef* success = katipoTable->getFunction("init")->call("BrowserTracker::BrowserTracker()->katipo.init()");
    if(!success || !success->boolValue())
    {
        TuiError("Failed to initialize tracker.");
    }
    
    trackerScriptState->getFunction("init")->call("BrowserTracker::BrowserTracker()->tracker.init()");
    katipoTable->getFunction("start")->call("BrowserTracker::BrowserTracker()->katipo.start()");

    /*
     ### initialize katipo now that we have loaded the launch args
     success = katipo.init()

     if(!success)
     {
         print("Failed to initialize katipo.")
         return
     }

     tracker.init()

     ### start the servers
     success = katipo.start()
     
     */
}


BrowserTracker::~BrowserTracker()
{
    delete tracker;
    rootTable->release();
    scriptState->release();
    //delete database;
    //delete databaseEnvironment;
}
