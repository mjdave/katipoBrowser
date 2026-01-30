#include "Noise3DFast.h"

#include <iostream>
#include <math.h>


#define normalize3(v) \
double d;\
d = (double)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);\
d = 1.0/d;\
v[0] = v[0] * d;\
v[1] = v[1] * d;\
v[2] = v[2] * d;


#define PERLIN_B_FAST SAMPLE_SIZE_FAST
#define PERLIN_BM_FAST (SAMPLE_SIZE_FAST-1)

#define PERLIN_N_FAST 0x1000
#define PERLIN_NP_FAST 12   /* 2^N */
#define PERLIN_NM_FAST 0xfff

#define s_curve(t) ( t * t * (3.0 - 2.0 * t) )
#define lerp(t, a, b) ( a + t * (b - a) )

#define setup(i,b0,b1,r0,r1)\
t = vec[i] + PERLIN_N_FAST;\
b0 = (((int)t) & PERLIN_BM_FAST);\
b1 = ((b0+1) & PERLIN_BM_FAST);\
r0 = t - (int)t;\
r1 = r0 - 1.0;

#define RANDOM_M 397
#define MATRIX_A 0x9908b0dfUL  
#define UMASK 0x80000000UL
#define LMASK 0x7fffffffUL 
#define MIXBITS(u,v) ( ((u) & UMASK) | ((v) & LMASK) )
#define TWIST(u,v) ((MIXBITS(u,v) >> 1) ^ ((v)&1UL ? MATRIX_A : 0UL))

double Noise3DFast::noiseRand()
{
	unsigned long y;

	if(--left == 0)
	{
		unsigned long *p=state;
		int j;

		left = RANDOM_N;
		next = state;

		for (j=RANDOM_N-RANDOM_M+1; --j; p++) 
			*p = p[RANDOM_M] ^ TWIST(p[0], p[1]);

		for (j=RANDOM_M; --j; p++) 
			*p = p[RANDOM_M-RANDOM_N] ^ TWIST(p[0], p[1]);

		*p = p[RANDOM_M-RANDOM_N] ^ TWIST(p[0], state[0]);
	}
	y = *next++;

	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return (double)y * (1.0/4294967295.0); 
}

Noise3DFast::Noise3DFast(int seed,
              double persistance_)
{
    persistance = persistance_;

	//RNG seed
	state[0]= seed & 0xffffffffUL;
	for (int j=1; j<RANDOM_N; j++) {
		state[j] = (1812433253UL * (state[j-1] ^ (state[j-1] >> 30)) + j); 
		state[j] &= 0xffffffffUL; 
	}
	left = 1; 
	initf = 1;
	//end RNG

	p = (int*)malloc(sizeof(int) * (SAMPLE_SIZE_FAST + SAMPLE_SIZE_FAST + 2));

	g3 = (double*)malloc(sizeof(double) * (SAMPLE_SIZE_FAST + SAMPLE_SIZE_FAST + 2) * 3);

    
    int i, j, k;

    for (i = 0 ; i < PERLIN_B_FAST ; i++)
    {
        p[i] = i;
        for (j = 0 ; j < 3 ; j++)
            g3[i * 3 + j] = (double)((noiseRand() * (PERLIN_B_FAST + PERLIN_B_FAST)) - PERLIN_B_FAST) / PERLIN_B_FAST;
		double* vec = &(g3[i * 3]);
        normalize3(vec);
    }

    while (--i)
    {
        k = p[i];
        p[i] = p[j = noiseRand() * PERLIN_B_FAST];
        p[j] = k;
    }

    for (i = 0 ; i < PERLIN_B_FAST + 2 ; i++)
    {
        p[PERLIN_B_FAST + i] = p[i];
        for (j = 0 ; j < 3 ; j++)
            g3[(PERLIN_B_FAST + i) *3 + j] = g3[i * 3 + j];
    }
}

Noise3DFast::Noise3DFast()
{
	free(p);
	free(g3);
}

double Noise3DFast::noise3DPrivate(const dvec3& vec)
{
    int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
    double rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
    int i, j;
    
    
    setup(0,bx0,bx1,rx0,rx1);
    setup(1,by0,by1,ry0,ry1);
    setup(2,bz0,bz1,rz0,rz1);
    
    i = p[ bx0 ];
    j = p[ bx1 ];
    
    b00 = p[ i + by0 ];
    b10 = p[ j + by0 ];
    b01 = p[ i + by1 ];
    b11 = p[ j + by1 ];
    
    t  = s_curve(rx0);
    sy = s_curve(ry0);
    sz = s_curve(rz0);
    
#define at3(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )
    
    q = &(g3[ (b00 + bz0) * 3 ]) ; u = at3(rx0,ry0,rz0);
    q = &(g3[ (b10 + bz0) * 3 ]) ; v = at3(rx1,ry0,rz0);
    a = lerp(t, u, v);
    
    q = &(g3[ (b01 + bz0) * 3 ]); u = at3(rx0,ry1,rz0);
    q = &(g3[ (b11 + bz0) * 3 ]); v = at3(rx1,ry1,rz0);
    b = lerp(t, u, v);
    
    c = lerp(sy, a, b);
    
    q = &(g3[ (b00 + bz1) * 3 ]); u = at3(rx0,ry0,rz1);
    q = &(g3[ (b10 + bz1) * 3 ]); v = at3(rx1,ry0,rz1);
    a = lerp(t, u, v);
    
    q = &(g3[ (b01 + bz1) * 3 ]); u = at3(rx0,ry1,rz1);
    q = &(g3[ (b11 + bz1) * 3 ]); v = at3(rx1,ry1,rz1);
    b = lerp(t, u, v);
    
    d = lerp(sy, a, b);
    
    return lerp(sz, c, d);
}

double Noise3DFast::get(dvec3 vec, int endOctave)
{
    if(endOctave <= 1)
    {
        return noise3DPrivate(vec);
    }
    double result = 0.0;
    double amp = 1.0;
    
    for(int i = 0; i<endOctave; i++ )
    {
        result += noise3DPrivate(vec) * amp;
        vec *= 2.0;
        amp*=persistance;
    }
    
    return result;
}

uint32_t Noise3DFast::rangedIntegerValue(dvec3 vec, int octaves, uint32_t max)
{
    double rawValue = get(vec, octaves);
    rawValue = (rawValue + 1.0) * 0.5;
    rawValue = rawValue * max;
    uint32_t intValue = rawValue;
    intValue = MAX(MIN(intValue, max - 1), 0);
    return intValue;
}



double Noise3DFast::rangedFraction(dvec3 vec, int octaves)
{
    double rawValue = get(vec, octaves);
    //rawValue = fabs(rawValue);
    rawValue = rawValue * 2.0;
    rawValue = powf(rawValue, 2.0);
    return clamp(rawValue * 2.0, 0.0, 1.0);
}

bool Noise3DFast::getChance(dvec3 vec,
               int octaves,
               uint32_t chanceNumerator,
               uint32_t chanceDenominator,
               uint64_t uniqueID,
               uint32_t seed)
{
    /*if(chanceDenominator == 100)
    {
        MJLog("hi");
    }*/
    double rangedFractionValue = rangedFraction(vec, octaves);
    uint32_t randomValue = randomIntegerValueForUniqueIDAndSeed(uniqueID, seed, chanceDenominator * 1000);
    
    bool win = (randomValue < rangedFractionValue * chanceNumerator * 1000);
    
    
    //MJLog("%d %d %lld - %.2f %d %d", chanceNumerator, chanceDenominator, uniqueID, rangedFractionValue, randomValue, win);
    
    /*if(chanceDenominator == 100)
    {
        MJLog("%d %d %lld - %.2f %d %d", chanceNumerator, chanceDenominator, uniqueID, rangedFractionValue, randomValue, win);
        
        if(randomValue == 0)
        {
            uint32_t randomValueB = randomIntegerValueForUniqueIDAndSeed(uniqueID, seed, chanceDenominator);
            MJLog("randomValueB:%d", randomValueB);
        }
    }*/
    
    return win;
    
}
