#include "KatipoUtilities.h"
#include "SDL.h"

namespace Katipo {

std::string getResourcePath(const std::string &appendPath)
{
    static std::string basePath = SDL_GetBasePath();
    
    if(appendPath.empty())
    {
        return basePath;
    }
    
    return basePath + appendPath;
}


std::string getSavePath(const std::string& appendPath)
{
    static std::string basePath = SDL_GetPrefPath("majicjungle", "katipo");

    if(appendPath.empty())
    {
        return basePath;
    }

    return basePath + appendPath;
}

};
