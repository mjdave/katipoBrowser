

#ifndef __World__NOISE3DFAST__
#define __World__NOISE3DFAST__

#define SAMPLE_SIZE_FAST 2048
#define RANDOM_N 624

#include "MathUtils.h"

class Noise3DFast {
    
   /* int p[SAMPLE_SIZE_FAST + SAMPLE_SIZE_FAST + 2];
    double g3[SAMPLE_SIZE_FAST + SAMPLE_SIZE_FAST + 2][3];
    double g2[SAMPLE_SIZE_FAST + SAMPLE_SIZE_FAST + 2][2];*/

	unsigned long state[RANDOM_N];
	int left = 1;
	int initf = 0;
	unsigned long *next;

	int* p;
	double* g3;
    
    double persistance;
    
public:
    Noise3DFast(int seed,
                  double persistance);
    Noise3DFast();
    
    double get(dvec3 vec, int octaves = 1);
    
    double rangedFraction(dvec3 vec, int octaves);
    uint32_t rangedIntegerValue(dvec3 vec, int octaves, uint32_t max);
    
    bool getChance(dvec3 vec,
                   int octaves,
                   uint32_t chanceNumerator,
                   uint32_t chanceDenominator,
                   uint64_t uniqueID,
                   uint32_t seed);

private:
    
    double noise3DPrivate(const dvec3& vec);
	double noiseRand();
    
};


#endif
