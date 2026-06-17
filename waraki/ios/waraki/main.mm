
#define SDL_MAIN_USE_CALLBACKS 1  
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


#include "MainController.h"
#include "Waraki.h"
#include "MJVersion.h"

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Waraki", KATIPO_VERSION, "com.katipobrowser.waraki");
    
    Waraki::getInstance()->init();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    MainController::getInstance()->handleEvent(event);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    MainController::getInstance()->sdlLoopIterate();
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
