//
//  Materials.h
//  Ambience
//
//  Created by David Frampton on 27/08/18.
//Copyright © 2018 Majic Jungle. All rights reserved.
//

#ifndef MaterialManager_h
#define MaterialManager_h

#include "Vulkan.h"

struct EdgeDecalInfo
{
    std::vector<dvec4> texLocations;
    vec2 size; //x -s extruded out, y is to or from tri center
	bool discardFaces;
};

struct Material {
    bool isDecal;
    std::string decalMaterialType;
    
    bool hasEdgeDecal;
    EdgeDecalInfo edgeDecalInfo;

    vec3 color;
    vec2 roughnessMetal;

	u8vec4 shaderMaterial;
	u8vec4 shaderMaterialB;
};

class MaterialManager {

public: // members

    std::map<std::string, Material> materialsByName;
    
    std::map<uint32_t, std::string> materialNamesByID;
    std::map<std::string, uint32_t> materialIDsByName;
    
protected: // members

public: // functions
    MaterialManager();
    ~MaterialManager();

	u8vec4 getShaderMaterial(const std::string& matName);
	u8vec4 getShaderMaterialB(const std::string& matName);

    const std::string& getDecalMaterial(const std::string& matName);
    bool getIsDecal(const std::string& matName);
    bool getHasEdgeDecal(const std::string& matName);
    EdgeDecalInfo getEdgeDecalInfo(const std::string& matName);

protected: // functions

};

#endif /* MaterialManager_h */
