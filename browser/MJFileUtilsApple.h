
#ifndef MJFileUtilsApple_h
#define MJFileUtilsApple_h

#include <string>

std::string createSecurityBookmarkForDirectory(std::string path); //returns bookmarkData
std::string resolveSecurityBookmark(std::string bookmarkData);

#endif /* MJFileUtilsApple_h */
