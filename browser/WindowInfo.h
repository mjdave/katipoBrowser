
#ifndef WindowInfo_h
#define WindowInfo_h

#include "MathUtils.h"

struct WindowInfo {
	float windowWidth;
	float windowHeight;
	float halfWindowWidth;
	float halfWindowHeight;

    float screenWidth;
    float screenHeight;
    float halfScreenWidth;
    float halfScreenHeight;
    
    float pixelDensity = 1.0;

	dmat4 projectionMatrix;
    
    int32_t posX;
    int32_t posY;
};

typedef struct WindowInfo WindowInfo;

#endif /* WindowInfo_h */
