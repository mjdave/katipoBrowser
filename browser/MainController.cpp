

#include "MainController.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#include "MJColorView.h"
#include "MJTextView.h"

#include "EventManager.h"
#include "TuiFileUtils.h"
#include "KatipoUtilities.h"
#include "MJImageTexture.h"
#include "MJFont.h"
#include "MJCache.h"
#include "TuiScript.h"
#include "MJImageView.h"
#include "Timer.h"
#include "MJDrawable.h"
#include "MJTimer.h"


#include "DatabaseEnvironment.h"
#include "Database.h"

#include "MJRenderTarget.h"
#include "Vulkan.h"
#include "GCommandBuffer.h"
#include "Camera.h"
#include "MJDrawQuad.h"
#include "MJDataTexture.h"

#include "Noise3DFast.h" //debug

#include "TuiScript.h"


#define DEFAULT_FOVY (70.0 * 0.0174533)
#define DEPTH_BUFFER_NEAR 0.01
#define DEPTH_BUFFER_FAR 10000000.0


MainController::MainController()
{
    
}


//todo there is a bunch of unused stuff in here related to full screen switching etc and isn't supported. Fullscreen support will be needed, but not worth sorting out yet
void MainController::init(std::string windowTitle, std::string organizationName, std::string appTitle)
{
	char *basePathCString = SDL_GetPrefPath(organizationName.c_str(), appTitle.c_str());
	if(!basePathCString)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Failed to find save files location", "You might be able to work around this by running the application as an administrator, or by fixing issues with file permissions.",NULL);
		exit(0);
	}

    multiSamplingEnabled = false;
    
    currentFPS = 0;
    FPSCount = 0;
    FPSCounterTimer = 0;
    //SDL_SetMainReady();
    int result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "1");
	if(result < 0)
    {
        MJError("Failed to initialize SDL:%d", result);
        MJError("%s\n", SDL_GetError());
    }

	MJLog("SDL version:%d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
    
    
	initializeDatabase();
    
    int flags = SDL_WINDOW_RESIZABLE;
	
    windowInfo = new WindowInfo();

    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);

	int screenWidth = mode->w * 0.8;
    int screenHeight = mode->h * 0.8;
	fovY = DEFAULT_FOVY;
    
#if TARGET_OS_IPHONE
    //SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    //const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
    //singleDisplayNativeResolution = ivec2(mode->w, mode->h);
    //screenWidth = singleDisplayNativeResolution.x;
    //screenHeight = singleDisplayNativeResolution.y;
#endif
    
    windowInfo->pixelDensity = mode->pixel_density;

	vsync = true;
    windowInfo->screenWidth = screenWidth;
    windowInfo->screenHeight = screenHeight;
	windowInfo->halfScreenWidth = windowInfo->screenWidth / 2;
	windowInfo->halfScreenHeight = windowInfo->screenHeight / 2;

    
    flags |= SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    
	displayWindow = SDL_CreateWindow(windowTitle.c_str(), screenWidth, screenHeight, flags);
    
    if(!displayWindow)
    {
		std::string sdlMessage = SDL_GetError();
        MJLog("Failed to create SDL window:%s", sdlMessage.c_str());
		std::string userMessage = "Please try updating both your GPU and CPU drivers.\n" + sdlMessage;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Failed to create game window",userMessage.c_str(),NULL);
		exit(0);
    }


    int w,h;
    SDL_GetWindowSize(displayWindow, &w, &h);
    windowInfo->windowWidth = w;
    windowInfo->windowHeight = h;
    windowInfo->screenWidth = w;
    windowInfo->screenHeight = h;

	windowInfo->halfWindowWidth = windowInfo->windowWidth / 2;
	windowInfo->halfWindowHeight = windowInfo->windowHeight / 2;
    windowInfo->halfScreenWidth = windowInfo->screenWidth / 2;
    windowInfo->halfScreenHeight = windowInfo->screenHeight / 2;
    
    
    int finalWindowsPosX,finalWindowsPosY;
    SDL_GetWindowPosition(displayWindow, &finalWindowsPosX, &finalWindowsPosY);
    windowInfo->posX = finalWindowsPosX;
    windowInfo->posY = finalWindowsPosY;


    vulkan = new Vulkan(this, displayWindow, ivec2(screenWidth * windowInfo->pixelDensity, screenHeight * windowInfo->pixelDensity), vsync);

	camera = new Camera(vulkan);
    
    //audio = new MJAudio(luaEnvironment, modManager);
    

    EventManager::getInstance()->init(this, displayWindow, windowInfo);

    renderTimer = new Timer();
    
    load();

}

void MainController::save()
{
}


// sapiens used to store a player id by itself in a player database. Now uses Steam ID.
void MainController::initializeDatabase()
{
    appDatabaseEnvironment = new DatabaseEnvironment(Katipo::getSavePath("appdb"),
                                                     1,
                                                     2);
    appDatabase = new Database(appDatabaseEnvironment, "app");
}


void MainController::windowInfoChanged()
{

	double ratio = windowInfo->screenWidth / windowInfo->screenHeight;
	projectionMatrix = makeInfReversedZProjRH(fovY, ratio, DEPTH_BUFFER_NEAR);
	projectionMatrix = mat4(1,0,0,0,
		0,-1,0,0,
		0,0,1,0,
		0,0,0,1) * projectionMatrix;

	windowInfo->projectionMatrix = projectionMatrix;
    
    
#if TARGET_OS_IPHONE //maybe can always do this?
    double scaleToUse = windowInfo->pixelDensity;
#else
    double scaleToUse = 1.0;//windowInfo->screenHeight / 1080;
#endif

    if(mainMJView)
    {
        double windowZOffset = -(windowInfo->screenHeight * scaleToUse * 0.5 / tan(fovY * 0.5));
        mainMJView->setWindowZOffset(windowZOffset);
        
        mainMJView->setScale(scaleToUse);
        mainMJView->setSize(dvec2(windowInfo->screenWidth, windowInfo->screenHeight));
    }


}

void MainController::recreateDrawablesAndSaveSize()
{
    int w,h;
    SDL_GetWindowSize(displayWindow, &w, &h);
    windowInfo->windowWidth = w;
    windowInfo->windowHeight = h;
    windowInfo->screenWidth = w;
    windowInfo->screenHeight = h;
    windowInfo->halfWindowWidth = windowInfo->windowWidth / 2;
    windowInfo->halfWindowHeight = windowInfo->windowHeight / 2;
    windowInfo->halfScreenWidth = windowInfo->screenWidth / 2;
    windowInfo->halfScreenHeight = windowInfo->screenHeight / 2;
    windowInfoChanged();
}

void MainController::mainWindowChangedSize()
{
    vulkan->windowSizeChanged();
}

void MainController::mainWindowChangedPosition()
{
    
}

void MainController::update(float dt)
{
    MJTimer::getInstance()->update(dt);
    
    if(mainMJView)
    {
        mainMJView->update(dt);
    }
}


void MainController::draw(double frameLerp)
{
#if LOG_DEBUG_TIMINGS
	debugTimer->getDt();
#endif
    double fpsDt = clamp(renderTimer->getDt(), 0.0, 0.1);
	animationTimer += fpsDt;
    FPSCounterTimer -= fpsDt;
    if(FPSCounterTimer <= 0.0)
    {
        FPSCounterTimer += 1.0;
        currentFPS = FPSCount;
        FPSCount = 0;
        
    }
    FPSCount++;

    bool recordStarted = vulkan->startRecord();

	if(!recordStarted)
	{
        MJLog("returning early");
        EventManager::getInstance()->doCPUWork();
		return;
	}

    GCommandBuffer* commandBuffer = vulkan->primaryCommandBuffer;

	int commandBufferIndex = commandBuffer->currentIndex;

	cache->recordStarted(*(commandBuffer->getBuffer()));

	//eventManager->doCPUWork(); more responsive but drops loads of frames
    
    //MJLog("render");
    mainMJView->preRender(commandBuffer, vulkan->finalOutputRenderPass, 0, fpsDt, frameLerp, animationTimer);

    commandBuffer->startRenderPass();

    //commandBuffer->startRenderPass();
    mat4 viewMatrix = mat4(1.0);
     
    mainMJView->render(commandBuffer);

    
    commandBuffer->endRenderPass();

    camera->update(commandBufferIndex,
        projectionMatrix,
       mat4(1.0),
        viewMatrix,
                   fpsDt, animationTimer);
    
    //MJLog("update ubos");
    mainMJView->updateUBOs(1.0, dvec3(0.0), dvec3(0.0));
	cache->recordEnded();
    vulkan->endRecord();
	vulkan->submit();
	
    EventManager::getInstance()->doCPUWork();
    //MJLog("done");

}

void MainController::runEventLoop()
{
    EventManager::getInstance()->runEventLoop();
}

void MainController::sdlLoopIterate()
{
    EventManager::getInstance()->idle();
}

void MainController::handleEvent(SDL_Event* event)
{
    EventManager::getInstance()->handleEvent(event);
}

MainController::~MainController()
{
    unload();
}



void MainController::applicationWillTerminate()
{
	//std::this_thread::sleep_for(std::chrono::milliseconds(500)); //if we need to wait for some stupid dependency to clean up itself
    //exit(0);
}

void MainController::unload()
{
    vkDeviceWaitIdle(vulkan->device);
    
    if(cache)
    {
        delete cache;
        cache = nullptr;
    }
    
    if(mainMJView)
    {
        delete mainMJView;
        mainMJView = nullptr;
    }
    
    
    EventManager::getInstance()->stopTextEntry();
    
}

void MainController::load()
{
    cache = new MJCache(vulkan, appDatabase, camera);
    
    cache->pixelDensity = windowInfo->pixelDensity;
    
	mainMJView = new MJView(windowInfo, cache);
	mainMJView->setSize(vec2(windowInfo->screenWidth, windowInfo->screenHeight));
	mainMJView->setRelativePosition(MJViewPosition(MJPositionCenter, MJPositionCenter));

	windowInfoChanged();
    
    vulkan->logMainDeviceDetails();

}

dvec3 MainController::getPointerRayStartUISpace()
{
    return dvec3(0.0,0.0,0.0);
}

uint64_t MainController::getVulkanDeviceVendorID() const
{
	return vulkan->deviceVendorID;
}
std::string MainController::getVulkanDeviceName() const
{
	return vulkan->deviceName;
}
uint32_t MainController::getVulkanDriverVersion() const
{
	return vulkan->driverVersion;
}

dvec3 MainController::getPointerRayDirectionUISpace()
{
    dvec2 mouseLoc = EventManager::getInstance()->mouseLoc;
    mouseLoc.x /= windowInfo->halfScreenWidth;
    mouseLoc.y /= -windowInfo->halfScreenHeight;
    dmat4 projInverse = inverse(windowInfo->projectionMatrix);
    dvec4 rayProj = projInverse * dvec4(mouseLoc.x, mouseLoc.y, -100.0, 1.0);

    return normalize(dvec3(rayProj));
}

bool MainController::mouseDown(dvec2 mousePos, int buttonIndex, int modKey)
{
	return mainMJView->mouseDown3D(getPointerRayStartUISpace(), getPointerRayDirectionUISpace(), false, buttonIndex);
}

bool MainController::mouseMoved()
{
	return mainMJView->mouseMoved3D(getPointerRayStartUISpace(), getPointerRayDirectionUISpace(), false, false);
}

bool MainController::mouseUp(dvec2 mousePos, int buttonIndex, int modKey)
{
	return mainMJView->mouseUp3D(getPointerRayStartUISpace(), getPointerRayDirectionUISpace(), buttonIndex);
}

bool MainController::mouseWheel(dvec2 mousePos, dvec2 scrollChange)
{
	return mainMJView->mouseWheel3D(getPointerRayStartUISpace(), getPointerRayDirectionUISpace(), scrollChange);
}

bool MainController::getMultiSamplingEnabled() const
{
    return multiSamplingEnabled;
}

void MainController::setMultiSamplingEnabled(bool multiSamplingEnabled_)
{
    if(multiSamplingEnabled_ != multiSamplingEnabled)
    {
        multiSamplingEnabled = multiSamplingEnabled_;
    }
}


double MainController::getFOVY() const
{
	return fovY;
}


double MainController::getFOVYIncludingUnappliedChange() const
{
	if(unconfirmedFOVChangeValue > 0.0)
	{
		return unconfirmedFOVChangeValue;
	}
	return fovY;
}

void MainController::setFOVY(double fovY_)
{
	unconfirmedFOVChangeValue = fovY_;
}



void MainController::exitToDesktop()
{
    exit(0);
}

void MainController::appLostFocus()
{
   // audio->appLostFocus();
}

void MainController::appGainedFocus()
{
    //audio->appGainedFocus();
}

void MainController::setVsync(bool newValue)
{
	if(newValue != vsync)
	{
		vsync = newValue;
		vulkan->vsyncSettingChanged(vsync);
	}
}

dvec2 MainController::getWindowSize()
{
	return dvec2(windowInfo->windowWidth, windowInfo->windowHeight);
}
