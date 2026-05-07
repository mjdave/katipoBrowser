

#ifndef __World__MainController__
#define __World__MainController__


#include "WindowInfo.h"

#include <vector>
#include "SDL.h"
//#include "MJLua.h"

class Shader;
//class ClientNetInterface;
class MJImageTexture;
class MJFont;
class MJView;
class MJTextView;
class Timer;
class MJCache;
class MJAudio;
class Vulkan;
class GCommandBuffer;
class MJRenderTarget;
class Camera;
class MJDrawQuad;
class MJDrawable;
class MJDataTexture;
class Database;
class TuiTable;


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
    WindowInfo* windowInfo;

    Vulkan* vulkan;
	Camera* camera;
    Database* appDatabase = nullptr;
    
public:
    
    MainController();
    ~MainController();
    
    void init(TuiTable* rootTable, Database* appDatabase_, std::string windowTitle = "Katipo", std::string organizationName = "katipo", std::string appTitle = "katipo");
    
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
    
    void appLostFocus();
    void appGainedFocus();

	dvec3 getPointerRayStartUISpace();
	dvec3 getPointerRayDirectionUISpace();

	bool mouseMoved();
	bool mouseDown(dvec2 mousePos, int buttonIndex, int modKey);
	bool mouseUp(dvec2 mousePos, int buttonIndex, int modKey);
	bool mouseWheel(dvec2 mousePos, dvec2 scrollChange);

	void updateWindowInfoSize();
    void mainWindowChangedSize();
    void mainWindowChangedPosition();
	void setVsync(bool newValue);
	dvec2 getWindowSize();


	uint64_t getVulkanDeviceVendorID() const;
	std::string getVulkanDeviceName() const;
	uint32_t getVulkanDriverVersion() const;
    
    
protected:
	SDL_Window * displayWindow;
    //MJAudio* audio;

	bool vsync;
    
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
    
private:

	void windowInfoChanged();
    void saveWindowInfoToDatabase();
    
    void load();
    void unload();
};


#endif /* defined(__World__MainController__) */
