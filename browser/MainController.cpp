//
//  MainController.cpp
//  World
//
//  Created by David Frampton on 1/12/14.
//  Copyright (c) 2014 Majic Jungle. All rights reserved.
//

#include "MainController.h"
//#include "MJAtmosphere.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#include "MJColorView.h"
#include "MJTextView.h"

#include "EventManager.h"
//#include "MacroFace.h"
//#include "ClientNetInterface.h"
#include "TuiFileUtils.h"
#include "KatipoUtilities.h"
#include "MJImageTexture.h"
#include "MJFont.h"
//#include "MJTextView.h"
#include "MJCache.h"
#include "TuiScript.h"
#include "MJImageView.h"
//#include "ServerWorld.h"
#include "Timer.h"
//#include "MJAudio.h"
//#include "BugReporting.h"
#include "MJDrawable.h"
#include "MJTimer.h"


#include "DatabaseEnvironment.h"
#include "Database.h"

#include "Model.h" //debug

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


void MainController::addResolution(ivec2 resolution)
{
	bool added = false;

	for(int i = 0; i < supportedResolutions.size(); i++)
	{
		ivec2 currentRes = supportedResolutions[i];
		if(currentRes.x > resolution.x)
		{
			supportedResolutions.insert(supportedResolutions.begin() + i, resolution);
			added = true;
			break;
		}
		else if(currentRes.x == resolution.x)
		{
			if(currentRes.y > resolution.y)
			{
				supportedResolutions.insert(supportedResolutions.begin() + i, resolution);
				added = true;
				break;
			}
			else if(currentRes.y == resolution.y)
			{
				return;
			}
		}
	}

	if(!added)
	{
		supportedResolutions.push_back(resolution);
	}
}

void MainController::updateScreenResolution(ivec2 newResolutionToUse, int newWindowModeToUse)
{
	if(newResolutionToUse.x != currentResolution.x || newResolutionToUse.y != currentResolution.y)
	{

		windowInfo->screenWidth = newResolutionToUse.x;
		windowInfo->screenHeight = newResolutionToUse.y;
		windowInfo->halfScreenWidth = windowInfo->screenWidth / 2;
		windowInfo->halfScreenHeight = windowInfo->screenHeight / 2;

		vulkan->screenResolutionChanged(newResolutionToUse);
		currentResolution = newResolutionToUse;
	}

	if(windowMode == MJ_WINDOW_MODE_FULLSCREEN || windowMode == MJ_WINDOW_MODE_BORDERLESS)
	{
		SDL_SetWindowFullscreen(displayWindow, 0);
		if(resolutionType == MJ_WINDOW_RESOLUTION_MULTI_DISPLAY)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(4000));
		}
	}

	windowMode = MJ_WINDOW_MODE_BORDERLESS;

	if(resolutionType == MJ_WINDOW_RESOLUTION_NATIVE_DISPLAY)
	{
		windowMode = newWindowMode;

		windowInfo->windowWidth = windowInfo->screenWidth;
		windowInfo->windowHeight = windowInfo->screenHeight;
	}
	else if(resolutionType == MJ_WINDOW_RESOLUTION_MULTI_DISPLAY)
	{
		windowMode = MJ_WINDOW_MODE_FULLSCREEN;

		windowInfo->windowWidth = windowInfo->screenWidth;
		windowInfo->windowHeight = windowInfo->screenHeight;
	}
	else
	{
		windowMode = newWindowMode;

		windowInfo->windowWidth = windowInfo->screenWidth;
		windowInfo->windowHeight = windowInfo->screenHeight;

		if(windowMode == MJ_WINDOW_MODE_BORDERLESS)
		{	
			windowInfo->windowWidth = singleDisplayNativeResolution.x;
			windowInfo->windowHeight = singleDisplayNativeResolution.y;
		}
		else if(windowMode == MJ_WINDOW_MODE_FULLSCREEN)
		{
			bool found = false;
			std::vector<ivec2> primaryDisplaySupportedResolutions;

            SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
            int numDisplayModes = 0;
            SDL_DisplayMode ** modes = SDL_GetFullscreenDisplayModes(displayID, &numDisplayModes);
			for(int modeIndex = 0; modeIndex < numDisplayModes; modeIndex++)
			{
				SDL_DisplayMode* mode = modes[modeIndex];
                primaryDisplaySupportedResolutions.push_back(ivec2(mode->w, mode->h));
			}

			for(ivec2& supported : primaryDisplaySupportedResolutions)
			{
				if(supported.x == windowInfo->screenWidth && supported.y == windowInfo->screenHeight)
				{
					found = true;
					break;
				}
			}

			if(found)
			{
				windowInfo->windowWidth = windowInfo->screenWidth;
				windowInfo->windowHeight = windowInfo->screenHeight;
			}
			else
			{
				windowInfo->windowWidth = singleDisplayNativeResolution.x;
				windowInfo->windowHeight = singleDisplayNativeResolution.y;
			}
		}

		if(windowMode != MJ_WINDOW_MODE_FULLSCREEN)
		{
			if(multiDisplayNativeResolution.x != 0)
			{
				windowInfo->windowWidth = min(windowInfo->windowWidth, (float)multiDisplayNativeResolution.x);
				windowInfo->windowHeight = min(windowInfo->windowHeight, (float)multiDisplayNativeResolution.y);
			}
			else
			{
				windowInfo->windowWidth = min(windowInfo->windowWidth, (float)singleDisplayNativeResolution.x);
				windowInfo->windowHeight = min(windowInfo->windowHeight, (float)singleDisplayNativeResolution.y);
			}
		}

		windowInfo->windowWidth = max(windowInfo->windowWidth, 100.0f);
		windowInfo->windowHeight = max(windowInfo->windowHeight, 100.0f);
	}


	windowInfo->halfWindowWidth = windowInfo->windowWidth / 2;
	windowInfo->halfWindowHeight = windowInfo->windowHeight / 2;

	//windowMode = newWindowModeToUse;

	SDL_SetWindowSize(displayWindow, windowInfo->windowWidth, windowInfo->windowHeight);

	if(windowMode == MJ_WINDOW_MODE_BORDERLESS)
	{
		SDL_SetWindowBordered(displayWindow, false);
		SDL_SetWindowPosition(displayWindow, 0, 0);
		SDL_SetWindowFullscreen(displayWindow, true);
	}
	else if(windowMode == MJ_WINDOW_MODE_FULLSCREEN)
	{
		SDL_SetWindowBordered(displayWindow, false);
		if(resolutionType == MJ_WINDOW_RESOLUTION_MULTI_DISPLAY)
		{
			SDL_SetWindowPosition(displayWindow, multiDisplayOrigin.x, multiDisplayOrigin.y);
		}
		else
		{
			SDL_SetWindowPosition(displayWindow, 0, 0);
		}
		SDL_SetWindowFullscreen(displayWindow, SDL_WINDOW_FULLSCREEN);
	}
	else
	{
		SDL_SetWindowBordered(displayWindow, true);
		SDL_SetWindowPosition(displayWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}
}


MainController::MainController()
{
    
}


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


    int screenX = SDL_WINDOWPOS_CENTERED;
    
    int flags = SDL_WINDOW_RESIZABLE;
    
/*#ifdef WIN32
    int screenY = SDL_WINDOWPOS_CENTERED;
    
    flags = flags | SDL_WINDOW_ALLOW_HIGHDPI;
#else
    int screenY = 0;
#endif*/

	int screenY = 0;




    /*
    int numDisplayModes = 0;
    SDL_DisplayMode ** modes = SDL_GetFullscreenDisplayModes(displayID, &numDisplayModes);
    for(int modeIndex = 0; modeIndex < numDisplayModes; modeIndex++)
    {
        SDL_DisplayMode* mode = modes[modeIndex];
        singleDisplayNativeResolution = ivec2(mode->w, mode->h);
    }*/
    

    /*int displayCount = 0; //todo multi display, finish update to sdl3
    SDL_DisplayID* displays = SDL_GetDisplays(&displayCount);
    
	int displayCount = SDL_GetNumVideoDisplays();
	if(displayCount > 1)
	{
		ivec2 multiDisplayMin = ivec2(999999, 999999);
		ivec2 multiDisplayMax = ivec2(-999999, -999999);
		bool invalidDimensions = false;
		for(int displayIndex = 0; displayIndex < displayCount; displayIndex++)
		{
			SDL_Rect displayBounds;
			if(SDL_GetDisplayBounds(displayIndex, &displayBounds) != 0) 
			{
				SDL_Log("SDL_GetDisplayBounds failed: %s", SDL_GetError());
			}
			else
			{
				if(displayBounds.w != singleDisplayNativeResolution.x || displayBounds.h != singleDisplayNativeResolution.y || displayBounds.y != 0)
				{
					invalidDimensions = true;
					break;
				}
				multiDisplayMin.x = min(multiDisplayMin.x, displayBounds.x);
				multiDisplayMin.y = min(multiDisplayMin.y, displayBounds.y);

				multiDisplayMax.x = max(multiDisplayMax.x, displayBounds.x + displayBounds.w);
				multiDisplayMax.y = max(multiDisplayMax.y, displayBounds.y + displayBounds.h);

				//MJLog("detected display at index:%d with bounds: (x:%d, y:%d, w:%d, h:%d)", displayIndex, displayBounds.x, displayBounds.y, displayBounds.w, displayBounds.h);
			}
		}
		if(!invalidDimensions && multiDisplayMax.x > multiDisplayMin.x)
		{
			multiDisplayNativeResolution = multiDisplayMax - multiDisplayMin;
			multiDisplayOrigin = multiDisplayMin;
		}
	}*/

	addResolution(ivec2(2560,1440));
	addResolution(ivec2(1920,1080));
	addResolution(ivec2(1280,720));
	addResolution(ivec2(720,480));
	//addResolution(ivec2(1920,720));

	//std::vector<ivec2> primaryDisplaySupportedResolutions;

    /*
    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
    singleDisplayNativeResolution = ivec2(mode->w, mode->h);
    
    int numDisplayModes = 0;
    SDL_DisplayMode ** modes = SDL_GetFullscreenDisplayModes(displayID, &numDisplayModes);
    for(int modeIndex = 0; modeIndex < numDisplayModes; modeIndex++)
    {
        SDL_DisplayMode* mode = modes[modeIndex];
        addResolution(ivec2(mode->w, mode->h));
        primaryDisplaySupportedResolutions.push_back(ivec2(mode->w, mode->h));
    }*/
    
	/*int numDisplayModes = SDL_GetNumDisplayModes(0);
	for(int modeIndex = 0; modeIndex < numDisplayModes; modeIndex++)
	{
		SDL_DisplayMode mode;
		if (SDL_GetDisplayMode(0, modeIndex, &mode) != 0) 
		{
			MJLog("SDL_GetDisplayMode failed: %s", SDL_GetError());  
		}
		else
		{
			addResolution(ivec2(mode.w, mode.h));
			primaryDisplaySupportedResolutions.push_back(ivec2(mode.w, mode.h));
		}
	}*/

	/*for(ivec2 res : supportedResolutions)
	{
		MJLog("Supports:%dx%d", res.x, res.y);
	}*/
    
    windowInfo = new WindowInfo();


	int screenWidth = 1920;
    int screenHeight = 1080;
	int windowModeToUse = MJ_WINDOW_MODE_FULLSCREEN;
	fovY = DEFAULT_FOVY;
    
#if TARGET_OS_IPHONE
    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
    singleDisplayNativeResolution = ivec2(mode->w, mode->h);
    windowInfo->pixelDensity = mode->pixel_density;
    screenWidth = singleDisplayNativeResolution.x;
    screenHeight = singleDisplayNativeResolution.y;
#endif

	vsync = true;

	/*GameClientConfigurationData configData;
	if(unserializeObjectAtPath(&configData, getSavePath("gameConfig.xml"), MJ_SERIALIZATION_XML))
	{
		locale = configData.locale;

		resolutionType = configData.resolutionType;
		vsync = configData.vsync;
		fovY = clamp(configData.fovY, 30.0 * 0.0174533, 120.0 * 0.0174533);

		if(resolutionType == MJ_WINDOW_RESOLUTION_NATIVE_DISPLAY)
		{
			screenWidth = singleDisplayNativeResolution.x;
			screenHeight = singleDisplayNativeResolution.y;

			windowModeToUse = configData.windowMode;

			windowInfo->windowWidth = screenWidth;
			windowInfo->windowHeight = screenHeight;
		}
		else if(resolutionType == MJ_WINDOW_RESOLUTION_MULTI_DISPLAY)
		{
			if(multiDisplayNativeResolution.x != 0)
			{
				screenWidth = multiDisplayNativeResolution.x;
				screenHeight = multiDisplayNativeResolution.y;
				windowModeToUse = MJ_WINDOW_MODE_FULLSCREEN;
			}
			else
			{
				resolutionType = MJ_WINDOW_RESOLUTION_NATIVE_DISPLAY;
				screenWidth = singleDisplayNativeResolution.x;
				screenHeight = singleDisplayNativeResolution.y;
				windowModeToUse = MJ_WINDOW_MODE_BORDERLESS;
			}

			windowInfo->windowWidth = screenWidth;
			windowInfo->windowHeight = screenHeight;
		}
		else
		{
			screenWidth = configData.resolution.x;
			screenHeight = configData.resolution.y;

			screenWidth = clamp(screenWidth, 100, 100000);
			screenHeight = clamp(screenHeight, 100, 100000);

			addResolution(ivec2(screenWidth, screenHeight));

			windowModeToUse = configData.windowMode;

			windowInfo->windowWidth = configData.windowSize.x;
			windowInfo->windowHeight = configData.windowSize.y;

			if(windowModeToUse == MJ_WINDOW_MODE_BORDERLESS)
			{	
				windowInfo->windowWidth = singleDisplayNativeResolution.x;
				windowInfo->windowHeight = singleDisplayNativeResolution.y;
			}
			else if(windowModeToUse == MJ_WINDOW_MODE_FULLSCREEN)
			{
				bool found = false;
				for(ivec2& supported : primaryDisplaySupportedResolutions)
				{
					if(supported.x == screenWidth && supported.y == screenHeight)
					{
						found = true;
						break;
					}
				}

				if(found)
				{
					windowInfo->windowWidth = screenWidth;
					windowInfo->windowHeight = screenHeight;
				}
				else
				{
					windowInfo->windowWidth = singleDisplayNativeResolution.x;
					windowInfo->windowHeight = singleDisplayNativeResolution.y;
				}
			}
			else if(windowModeToUse == MJ_WINDOW_MODE_WINDOWED)
			{
				screenX = configData.windowPos.x;
				screenY = configData.windowPos.y;
			}

			if(windowModeToUse != MJ_WINDOW_MODE_FULLSCREEN)
			{
				if(multiDisplayNativeResolution.x != 0)
				{
					windowInfo->windowWidth = min(windowInfo->windowWidth, (float)multiDisplayNativeResolution.x);
					windowInfo->windowHeight = min(windowInfo->windowHeight, (float)multiDisplayNativeResolution.y);
				}
				else
				{
					windowInfo->windowWidth = min(windowInfo->windowWidth, (float)singleDisplayNativeResolution.x);
					windowInfo->windowHeight = min(windowInfo->windowHeight, (float)singleDisplayNativeResolution.y);
				}
			}

			windowInfo->windowWidth = max(windowInfo->windowWidth, 100.0f);
			windowInfo->windowHeight = max(windowInfo->windowHeight, 100.0f);
		}
	}
	else
	{*/

		//resolutionType = MJ_WINDOW_RESOLUTION_NATIVE_DISPLAY;
		//windowModeToUse = MJ_WINDOW_MODE_BORDERLESS;
    
        resolutionType = MJ_WINDOW_RESOLUTION_STANDARD;   
		windowModeToUse = MJ_WINDOW_MODE_WINDOWED;
    
		/*if(screenWidth > 1920)
		{
			screenWidth = 1920;
			screenHeight = 1080;

			resolutionType = MJ_WINDOW_RESOLUTION_STANDARD;
		}*/

		/*GameClientConfigurationData configData;
		configData.resolution = ivec2(screenWidth,screenHeight);
		configData.windowMode = windowModeToUse;
		configData.resolutionType = resolutionType;
		configData.windowSize = ivec2(screenWidth,screenHeight);
		configData.vsync = vsync;
		configData.fovY = fovY;
		configData.windowPos = ivec2(screenX,screenY);
		configData.locale = locale;


		serializeObjectToPath(configData, getSavePath("gameConfig.xml"), MJ_SERIALIZATION_XML);*/

		windowInfo->windowWidth = screenWidth;
		windowInfo->windowHeight = screenHeight;
	//}

	//MJLuaModule* localeModule = new MJLuaModule(luaEnvironment, "common/locale");
	//localeModule->callFunction("setLocale", locale);

	currentResolution = ivec2(screenWidth, screenHeight);

    windowInfo->screenWidth = screenWidth;
    windowInfo->screenHeight = screenHeight;
	windowInfo->halfScreenWidth = windowInfo->screenWidth / 2;
	windowInfo->halfScreenHeight = windowInfo->screenHeight / 2;



	windowMode = windowModeToUse;
	if(windowMode == MJ_WINDOW_MODE_BORDERLESS)
	{
		flags = flags | SDL_WINDOW_BORDERLESS;
	}
	else if(windowMode == MJ_WINDOW_MODE_FULLSCREEN)
	{
		flags = flags | SDL_WINDOW_BORDERLESS;
	}
	else
	{
		/*SDL_Rect usableRect;
		SDL_GetDisplayUsableBounds(0, &usableRect);

		if(usableRect.y != 0 && usableRect.w <= (int)windowInfo->windowWidth && usableRect.h <= windowInfo->windowHeight + 40)
		{
			screenX = usableRect.x;
			screenY = singleDisplayNativeResolution.y - windowInfo->windowHeight;
			MJLog("screenY:%d currentResolution.y:%d windowInfo->windowHeight:%.1f", screenY, singleDisplayNativeResolution.y, windowInfo->windowHeight)
		}*/
	}

	/*int windowWidth = 1920;
	int windowHeight = 1080;
	windowWidth = windowWidth * 1.5;
	windowHeight = windowHeight * 1.5;


	//screenWidth = 3200;
	//screenHeight = 900;

	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	int actualScreenWidth = DM.w;
	int actualScreenHeight = DM.h;

	if(actualScreenWidth > 0 && windowWidth > actualScreenWidth)
	{
		windowWidth = actualScreenWidth;
	}
	if(actualScreenHeight > 0 && windowHeight > actualScreenHeight - 70)
	{
		windowHeight = actualScreenHeight - 70;
		screenY = 30;
	}*/
    
    flags |= SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    
	displayWindow = SDL_CreateWindow(windowTitle.c_str(), windowInfo->windowWidth, windowInfo->windowHeight, flags);
    
    if(!displayWindow)
    {
		std::string sdlMessage = SDL_GetError();
        MJLog("Failed to create SDL window:%s", sdlMessage.c_str());
		std::string userMessage = "Please try updating both your GPU and CPU drivers.\n" + sdlMessage;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Failed to create game window",userMessage.c_str(),NULL);
		exit(0);
    }

	if(windowMode == MJ_WINDOW_MODE_FULLSCREEN)
	{
		SDL_SetWindowFullscreen(displayWindow, SDL_WINDOW_FULLSCREEN);
	}


	if(windowMode == MJ_WINDOW_MODE_BORDERLESS || windowMode == MJ_WINDOW_MODE_FULLSCREEN)
	{
		int w,h;
		SDL_GetWindowSize(displayWindow, &w, &h);
		windowInfo->windowWidth = w;
		windowInfo->windowHeight = h;
        windowInfo->screenWidth = w;
        windowInfo->screenHeight = h;
	}

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
    EventManager::getInstance()->setMouseHidden(true);

    renderTimer = new Timer();
#if LOG_DEBUG_TIMINGS
	debugTimer = new Timer();
#endif


	int noiseTextureSize = 2048;
	uint8_t* noiseData = (uint8_t*)malloc(noiseTextureSize * noiseTextureSize * 4);
	for(int i = 0; i < noiseTextureSize * noiseTextureSize; i++)
	{
		uint64_t index = i * 4;
		double randomA = randomValueForUniqueIDAndSeed(index, 48916);
		double randomB = randomValueForUniqueIDAndSeed(index + 1, 3454633);
		double randomC = randomValueForUniqueIDAndSeed(index + 2, 6753657);
		double randomD = randomValueForUniqueIDAndSeed(index + 3, 545437653);
		dvec3 randomVec = dvec3(randomA - 0.5, randomB - 0.5, randomC - 0.5);
		randomVec = normalize(randomVec) * pow(randomD, 1.5);

		noiseData[index + 0] = (randomVec.x * 0.5 + 0.5) * 255;
		noiseData[index + 1] = (randomVec.y * 0.5 + 0.5) * 255;
		noiseData[index + 2] = (randomVec.z * 0.5 + 0.5) * 255;
		noiseData[index + 3] = 255;
	}

	noiseTexture = new MJDataTexture(vulkan, ivec2(noiseTextureSize, noiseTextureSize), noiseData, true, false, VK_FILTER_NEAREST, VK_FILTER_NEAREST);

	free(noiseData);
    
    /*
#if !STEAM_DISABLED
    steam->checkCommandLineArgsAndJoinLobbyIfNeeded();
#endif*/
    
    load();

}

void MainController::save()
{
    /*AppControllerSaveData saveData;
    saveData.playerID = playerID;
    appDatabase->setSerialized(saveData, "controller");*/
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
    /*
#if TARGET_OS_IPHONE
    double scaleToUse = windowInfo->pixelDensity;
#else
    double scaleToUse = windowInfo->screenHeight / 1080;
    if(scaleToUse > 1.0001)
    {
        if(scaleToUse >= 3.0)
        {
            scaleToUse = 3.0;
        }
        else if(scaleToUse >= 2.0)
        {
            scaleToUse = 2.0;
        }
        else if(scaleToUse >= 1.5)
        {
            scaleToUse = 1.5;
        }
        else if(scaleToUse >= 1.25)
        {
            scaleToUse = 1.25;
        }
        else
        {
            scaleToUse = 1.0;
        }
    }
    else if(scaleToUse < 0.9999)
    {
        if(scaleToUse >= 0.75)
        {
            scaleToUse = 0.75;
        }
        else if(scaleToUse >= 0.66)
        {
            scaleToUse = 2.0/3.0;
        }
        else
        {
            scaleToUse = 0.5;
        }
    }
    else
    {
        scaleToUse = 1.0;
    }
#endif*/
    
    
#if TARGET_OS_IPHONE
    double scaleToUse = windowInfo->pixelDensity;
#else
    double scaleToUse = 1.0;//windowInfo->screenHeight / 1080;
#endif

    if(mainMJView)
    {
        double windowZOffset = -(windowInfo->screenHeight * scaleToUse * 0.5 / tan(fovY * 0.5));
        mainMJView->setWindowZOffset(windowZOffset);
        
        //dvec2 oldSize = mainMJView->getSize();
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

    /*finalOutputDrawQuad = new MJFinalOutputQuad(cache,
        vulkan->finalOutputRenderPass,
        vulkan->worldRenderTarget,
        bloom->getOutputRenderTarget());*/

//    saveCurrentClientConfiguration();
}

void MainController::mainWindowChangedSize()
{
    vulkan->windowSizeChanged();
}

void MainController::mainWindowChangedPosition()
{
//    saveCurrentClientConfiguration();
}

void MainController::update(float dt)
{
    //luaEnvironment->update(dt);
	//controllerLuaModule->callFunction("update", dt);
    
    MJTimer::getInstance()->update(dt);
    
    //audio->update();
    if(mainMJView)
    {
        mainMJView->update(dt);
    }

	//steam->update(dt);
	//bugReporting->update(dt);
    
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
		//MJLog("dropped %d frames this second", 75 - FPSCount);
      //  MJLog("FPS:%d", FPSCount);
        currentFPS = FPSCount;
        FPSCount = 0;
        
    }
    FPSCount++;

	if(needsToUpdateResolution)
	{
		needsToUpdateResolution = false;
		updateScreenResolution(newResolution, newWindowMode);
	}
    

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
    
    mainMJView->updateUBOs(1.0, dvec3(0.0), dvec3(0.0));
        


	cache->recordEnded();
    vulkan->endRecord();


	//debugTimer->getDt();

	//debugTimer->getDt();
#if LOG_DEBUG_TIMINGS
	MJLog("draw pre submit:%.2f", (debugTimer->getDt() * 1000.0));
#endif
	//std::this_thread::sleep_for(std::chrono::milliseconds(30)); //simulates CPU lag, to run at 30FPS


	vulkan->submit();
	
    EventManager::getInstance()->doCPUWork(); //smooth but laggy input

#if LOG_DEBUG_TIMINGS
	MJLog("vulkan->submit():%.2f", (debugTimer->getDt() * 1000.0));
#endif

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
   /* if(steam)
    {
        delete steam;
        steam = nullptr;
    }*/
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
    exit(0);
}

void MainController::unload()
{
    vkDeviceWaitIdle(vulkan->device);
    if(materialManager)
    {
        delete materialManager;
        materialManager = nullptr;
    }
    
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
    
   /* TuiTable* table = new TuiTable();
    MJLog("Empty table:%s", table->getDebugString().c_str());
    TuiNumber* fractionalValue = new TuiNumber(0.12345);
    TuiNumber* wholeValue = new TuiNumber(2);
    TuiNumber* zeroValue = new TuiNumber(0);
    MJLog("fractionalValue:%s", fractionalValue->getDebugString().c_str());
    MJLog("wholeValue:%s", wholeValue->getDebugString().c_str());
    MJLog("zeroValue:%s", zeroValue->getDebugString().c_str());
    
    table->set("fractional value string key", wholeValue);
    table->set(0, fractionalValue);
    
    MJLog("added table:%s", table->getDebugString().c_str());
    
    std::string testStringString = "{\
    testKey1 = stringValue,\
    testKey2 = \"This is a double 'quoted' string\",\
    testKey3 = 37.0, \
    testKey4 = 'should not be an \"error\"', \
    testKey5 = 'Duplicate testKey5 entry 1', \
    testKey5 = 'Duplicate testKey5 entry 2', \
    testKey6 = '', \
    testKey7 = "", \
    0 = \"zero point seven three five\", \
    3 = \"three\", \
} ";
    
    TuiTable* testTable = TuiTable::initWithHumanReadableString(testStringString);
    
    table->set("testKey", testTable);
    
    std::string testStringStringB = "{\
    firstValue,\
    0.356,\
    secondValue,\
    \"third value\",\
    4\
}";
    
    TuiTable* testArrrayTable = TuiTable::initWithHumanReadableString(testStringStringB);
    
    TuiTable* parentTable = new TuiTable();
    parentTable->set("childTable", table);
    parentTable->set("testArrrayTable", testArrrayTable);
    
    MJLog("nested table:%s", parentTable->getDebugString().c_str());*/
    
   // exit(0);

    
	materialManager = new MaterialManager();
    
    
    cache = new MJCache(vulkan, materialManager, appDatabase, camera, noiseTexture);
    
    //eventManager->setUpLuaEnvironment();
//    audio->setUpLuaEnvironment();



	mainMJView = new MJView(windowInfo, cache);
	mainMJView->setSize(vec2(windowInfo->screenWidth, windowInfo->screenHeight));
	mainMJView->setRelativePosition(MJViewPosition(MJPositionCenter, MJPositionCenter));
    
    /*MJColorView* testView = new MJColorView(mainMJView);
    testView->setSize(dvec2(windowInfo->screenWidth, windowInfo->screenHeight) / 4.0);
    testView->setColor(dvec4(0.2,0.5,0.9,1.0));*/
    
    /*MJTextView* testTextView = new MJTextView(mainMJView);
    FontNameAndSize foo;
    foo.name = "sapiensLight";
    foo.size = 20;
    testTextView->setFontNameAndSize(foo);
    testTextView->setText("Hello World!\nanother line because I am very ambitious");*/

	windowInfoChanged();
    
    vulkan->logMainDeviceDetails();
    EventManager::getInstance()->setMouseHidden(false);

}

dvec3 MainController::getPointerRayStartUISpace()
{
    if(!EventManager::getInstance()->getMouseHidden())
    {
        return dvec3(0.0,0.0,0.0);
    }
    return dvec3(0.0,0.0,100.0);
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
	if(!EventManager::getInstance()->getMouseHidden())
	{
		dvec2 mouseLoc = EventManager::getInstance()->mouseLoc;
		mouseLoc.x /= windowInfo->halfScreenWidth;
		mouseLoc.y /= -windowInfo->halfScreenHeight;
		dmat4 projInverse = inverse(windowInfo->projectionMatrix);
		dvec4 rayProj = projInverse * dvec4(mouseLoc.x, mouseLoc.y, -100.0, 1.0);

		return normalize(dvec3(rayProj));
	}
	return dvec3(0.0,0.0,-1.0);
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
	//fovY = fovY_;
//	saveCurrentClientConfiguration();
	//windowInfoChanged();
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


/*
LuaRef MainController::getSupportedScreenResolutionList()
{
	LuaRef result = luabridge::newTable(luaEnvironment->state);

	int resultCount = 0;

	LuaRef entry = luabridge::newTable(luaEnvironment->state);
	entry["type"] = "native";
	entry["x"] = singleDisplayNativeResolution.x;
	entry["y"] = singleDisplayNativeResolution.y;

	result[++resultCount] = entry;

	if(multiDisplayNativeResolution.x != 0)
	{
		LuaRef entry = luabridge::newTable(luaEnvironment->state);
		entry["type"] = "multi";
		entry["x"] = multiDisplayNativeResolution.x;
		entry["y"] = multiDisplayNativeResolution.y;

		result[++resultCount] = entry;
	}

	for(int i = 0; i < supportedResolutions.size(); i++)
	{
		ivec2 resolution = supportedResolutions[supportedResolutions.size() - i - 1];
		LuaRef entry = luabridge::newTable(luaEnvironment->state);
		entry["type"] = "standard";
		entry["x"] = resolution.x;
		entry["y"] = resolution.y;

		result[++resultCount] = entry;
	}
	return result;
}*/


void MainController::selectScreenResolutionAndWindowMode(int screenResolutionIndex, int windowModeIndex)
{
	MJLog("selectScreenResolutionAndWindowMode: screenResolutionIndex:%d windowModeIndex:%d", screenResolutionIndex, windowModeIndex);
	int selectionOffset = 1;
	newResolution = ivec2(0,0);
	newWindowMode = windowMode;

	resolutionType = MJ_WINDOW_RESOLUTION_STANDARD;


	if(screenResolutionIndex == 1)
	{
		newResolution = singleDisplayNativeResolution;
		resolutionType = MJ_WINDOW_RESOLUTION_NATIVE_DISPLAY;
		newWindowMode = windowModeIndex;
	}

	else
	{
		if(multiDisplayNativeResolution.x != 0)
		{
			if(screenResolutionIndex == 1 + selectionOffset)
			{
				newResolution = multiDisplayNativeResolution;
				resolutionType = MJ_WINDOW_RESOLUTION_MULTI_DISPLAY;
				newWindowMode = MJ_WINDOW_MODE_FULLSCREEN;
			}
			selectionOffset++;
		}

		if(newResolution.x == 0)
		{
			int arrayIndex = (int)supportedResolutions.size() - screenResolutionIndex + selectionOffset;
			if(arrayIndex >= 0 && arrayIndex < supportedResolutions.size())
			{
				newResolution = supportedResolutions[arrayIndex];
				newWindowMode = windowModeIndex;
			}
		}
	}

	if(newResolution.x != 0)
	{
		MJLog("selectScreenResolutionAndWindowMode: setting:%d,%d resolutionType:%d", newResolution.x, newResolution.y, resolutionType);
		
		needsToUpdateResolution = true;

		//updateScreenResolution(newResolution);
	}
}

void MainController::setVsync(bool newValue)
{
	if(newValue != vsync)
	{
		vsync = newValue;
		vulkan->vsyncSettingChanged(vsync);
//		saveCurrentClientConfiguration();
	}
}

dvec2 MainController::getWindowSize()
{
	return dvec2(windowInfo->windowWidth, windowInfo->windowHeight);
}
