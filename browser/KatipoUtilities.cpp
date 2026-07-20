#include "KatipoUtilities.h"
#include "SDL.h"

namespace Katipo {

std::string baseSavePath = "";

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
    if(baseSavePath.empty())
    {
        TuiError("base save path must be set with eg: Katipo::baseSavePath = SDL_GetPrefPath(\"katipobrowser\", \"waraki\");");
    }

    if(appendPath.empty())
    {
        return baseSavePath;
    }

    return baseSavePath + appendPath;
}

};
