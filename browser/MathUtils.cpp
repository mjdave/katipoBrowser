//
//  MathUtils.cpp
//  World
//
//  Created by David Frampton on 16/12/14.
//  Copyright (c) 2014 Majic Jungle. All rights reserved.
//

#include "MathUtils.h"

#include "MurmurHash3.h"


double randomValueForPositionAndSeed(dvec3 pos, uint32_t seed)
{
    int64_t posInt[3];
    posInt[0] = pos.x * 10000000;
    posInt[1] = pos.y * 10000000;
    posInt[2] = pos.z * 10000000;
    
    uint32_t hash;
    MurmurHash3_x86_32(posInt, sizeof(int64_t) * 3, seed, &hash);
    double value = hash;
    value = value / UINT32_MAX;
    return value;
}

double randomValueForUniqueIDAndSeed(uint64_t uniqueID, uint32_t seed)
{
    uint32_t hash;
    MurmurHash3_x86_32(&uniqueID, sizeof(uint64_t), seed, &hash);
    double value = hash;
    value = value / UINT32_MAX;
    return value;
}

uint32_t randomIntegerValueForUniqueIDAndSeed(uint64_t uniqueID, uint32_t seed, uint32_t max)
{
    if(max < 1)
    {
        return max;
    }

    uint32_t hash;
    MurmurHash3_x86_32(&uniqueID, sizeof(uint64_t), seed, &hash);
    hash = hash % max;
    return MIN(hash, max - 1);
}

uint32_t randomIntegerValueForUniqueIDAndSeedCompatibilityMode(uint64_t uniqueID_, uint32_t seed, uint32_t max, bool macCompatibilityMode)
{
    uint64_t uniqueIDSanitized = uniqueID_;
    uint8_t* data = (uint8_t*)(&uniqueIDSanitized);
    
    if(macCompatibilityMode)
    {
        data[4] = 0;
        data[5] = 0;
        data[6] = 0;
        data[7] = 0;
    }
    else
    {
        data[4] = 255;
        data[5] = 255;
        data[6] = 255;
        data[7] = 255;
    }
    
    uint32_t hash;
    MurmurHash3_x86_32(&uniqueIDSanitized, sizeof(uint64_t), seed, &hash);
    hash = hash % max;
    return MIN(hash, max - 1);
}

uint32_t randomIntegerValueForUniqueIDAndSeedWithLogging(uint64_t uniqueID, uint32_t seed, uint32_t max)
{
    if(max < 1)
    {
        return max;
    }
    const uint8_t * data = (const uint8_t*)(&uniqueID);
    for(int i = 0; i < sizeof(uint64_t); i++)
    {
        MJLog("%d", data[i]);
    }
    uint32_t hash;
    MurmurHash3_x86_32(&uniqueID, sizeof(uint64_t), seed, &hash);
    //MJLog("randomIntegerValueForUniqueIDAndSeed:%llx, %d, %d", uniqueID, seed, hash)
    hash = hash % max;
    //MJLog("hash:%d", hash)
    data = (const uint8_t*)(&uniqueID);
    for(int i = 0; i < sizeof(uint64_t); i++)
    {
        MJLog("%d", data[i]);
    }

    return MIN(hash, max - 1);
}

bool getLocalOSPadsWithZeroCasted32(uint64_t uniqueID)
{
    const uint8_t * data = (const uint8_t*)(&uniqueID);
    return (data[5] == 0);
}

bool randomBoolForUniqueIDAndSeed(uint64_t uniqueID, uint32_t seed)
{
    uint32_t hash;
    MurmurHash3_x86_32(&uniqueID, sizeof(uint64_t), seed, &hash);
    hash = hash % 2;
    return (hash == 1);
}

uint32_t seedValueFromSeedString(std::string seedString)
{
    uint32_t hash;
    MurmurHash3_x86_32(seedString.data(), seedString.size(), 123, &hash);
    return hash;
}
