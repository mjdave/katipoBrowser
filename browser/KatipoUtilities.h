
#ifndef __KatipoUtilities__
#define __KatipoUtilities__

#include "TuiFileUtils.h"



namespace Katipo {

extern std::string baseSavePath;

std::string getResourcePath(const std::string &appendPath = "");
std::string getSavePath(const std::string &appendPath = "");

};



#endif
