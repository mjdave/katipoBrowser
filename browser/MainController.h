

#ifndef __World__MainController__
#define __World__MainController__

#include "WindowInfo.h"

#include <vector>
#include "Serialization.h"
//#include "MJLua.h"

class Shader;
//class ClientNetInterface;
class MJImageTexture;
class MJFont;
class MJView;
class MJTextView;
class DatabaseEnvironment;
class Database;
class Timer;
class MJCache;
class MJAudio;
class Vulkan;
class GCommandBuffer;
class MaterialManager;
class MJRenderTarget;
class Camera;
class MJDrawQuad;
class RandomNumberGenerator;
class MJDrawable;
class MJDataTexture;

class UserSettings;


#define MJ_WINDOW_MODE_WINDOWED 1
#define MJ_WINDOW_MODE_BORDERLESS 2
#define MJ_WINDOW_MODE_FULLSCREEN 3

#define MJ_WINDOW_RESOLUTION_STANDARD 1
#define MJ_WINDOW_RESOLUTION_NATIVE_DISPLAY 2
#define MJ_WINDOW_RESOLUTION_MULTI_DISPLAY 3


class MainController
{
public:
    
    static MainController* getInstance() {
        static MainController* instance = new MainController();
        return instance;
    }

    MJView* mainMJView = nullptr;
    MJCache* cache;
    MaterialManager* materialManager;
    UserSettings* userSettings;
    WindowInfo* windowInfo;

    Vulkan* vulkan;
	Camera* camera;
    
public:
    
    MainController();
    ~MainController();
    
    void init(std::string windowTitle = "Ambience", std::string organizationName = "majicjungle", std::string appTitle = "ambience");
    
    void applicationWillTerminate();

    void update(float dt);
    
    void runEventLoop(void);
    void sdlLoopIterate();
    void handleEvent(SDL_Event* event);

	void draw(double frameLerp);
    
    void exitToDesktop();
    
    bool getMultiSamplingEnabled() const;
    void setMultiSamplingEnabled(bool multiSamplingEnabled_);

	double getFOVY() const;
	double getFOVYIncludingUnappliedChange() const;
	void setFOVY(double fovY_);

	std::string getLocale() const;
	void setLocale(std::string locale_);
    
    void appLostFocus();
    void appGainedFocus();

	dvec3 getPointerRayStartUISpace();
	dvec3 getPointerRayDirectionUISpace();

	bool mouseMoved();
	bool mouseDown(dvec2 mousePos, int buttonIndex, int modKey);
	bool mouseUp(dvec2 mousePos, int buttonIndex, int modKey);
	bool mouseWheel(dvec2 mousePos, dvec2 scrollChange);

	void selectScreenResolutionAndWindowMode(int screenResolutionIndex, int windowModeIndex);

	std::string getVersionString();
	bool getIsDevelopmentBuild();

	void recreateDrawablesAndSaveSize();
    void mainWindowChangedSize();
    void mainWindowChangedPosition();
	void setVsync(bool newValue);
	dvec2 getWindowSize();


	uint64_t getVulkanDeviceVendorID() const;
	std::string getVulkanDeviceName() const;
	uint32_t getVulkanDriverVersion() const;

	void generateVRAMProfile();
    
    
protected:
	SDL_Window * displayWindow;
    //MJAudio* audio;

	MJDataTexture* noiseTexture;
    
    DatabaseEnvironment* appDatabaseEnvironment;
    Database* appDatabase;

	std::vector<ivec2> supportedResolutions;
	ivec2 multiDisplayNativeResolution = ivec2(0,0);
	ivec2 multiDisplayOrigin = ivec2(0,0);
	ivec2 singleDisplayNativeResolution = ivec2(0,0);
	ivec2 currentResolution;
	ivec2 newResolution;
	bool vsync;
	int newWindowMode;
	bool needsToUpdateResolution = false;

	int windowMode = MJ_WINDOW_MODE_BORDERLESS;
	int resolutionType = MJ_WINDOW_RESOLUTION_STANDARD;
    
    bool multiSamplingEnabled;
	double fovY;
	double unconfirmedFOVChangeValue = -1.0;
    
    double FPSCounterTimer;
	double animationTimer = 0.0;
    int FPSCount;
    int currentFPS;

	mat4 projectionMatrix;
    
    bool animating;

    Timer* renderTimer;
	Timer* debugTimer;

	RandomNumberGenerator* randomNumberGenerator;
    
private:

	void updateScreenResolution(ivec2 newResolution, int newWindowModeToUse);
	void addResolution(ivec2 resolution);
    void save();
    void initializeDatabase();
	void windowInfoChanged();
    
    void load();
    void unload();
};


#endif /* defined(__World__MainController__) */
