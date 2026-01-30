//
//  Materials.cpp
//  Ambience
//
//  Created by David Frampton on 27/08/18.
//Copyright © 2018 Majic Jungle. All rights reserved.
//

#include "MaterialManager.h"

#include "TuiScript.h"
#include "KatipoUtilities.h"

MaterialManager::MaterialManager()
{
    std::string materialsPath = Katipo::getResourcePath("app/common/materials.tui");
    MJLog("materialsPath:%s", materialsPath.c_str());
    TuiTable* materialsTable = (TuiTable*)TuiRef::load(materialsPath);
    
    uint32_t idCounter = 0;
    for(auto& kv : materialsTable->objectsByStringKey)
    {
        uint32_t matID = ++idCounter;
        materialNamesByID[matID] = kv.first;
        materialIDsByName[kv.first] = matID;
        
        TuiTable* matTable = (TuiTable*)kv.second;
        
        Material& mat = materialsByName[kv.first];
        mat.isDecal = false;
        mat.hasEdgeDecal = false;
        mat.decalMaterialType = -1;
        
        if(matTable->getBool("decal"))
        {
            mat.isDecal = true;
        }
        
        if(matTable->hasKey("edgeDecal"))
        {
            TuiTable* edgeDecalTable = matTable->getTable("edgeDecal");
            if(edgeDecalTable && edgeDecalTable->hasKey("textureLocations") && edgeDecalTable->hasKey("size"))
            {
                
                mat.hasEdgeDecal = true;
                TuiTable* textureLocationsTable = edgeDecalTable->getTable("textureLocations");
                for(TuiRef* texLocationRef : textureLocationsTable->arrayObjects)
                {
                    mat.edgeDecalInfo.texLocations.push_back(((TuiVec4*)texLocationRef)->value);
                }
                
                mat.edgeDecalInfo.size = edgeDecalTable->getVec2("size");
                mat.edgeDecalInfo.discardFaces = edgeDecalTable->getBool("discardFaces");
            }
            else
            {
                MJError("Missing textureLocations or size in materials.tui edgeDecal");
            }
        }
        
        mat.color = vec4(matTable->getVec3("color"), 1.0);
        mat.roughnessMetal = vec2(matTable->getDouble("roughness"), matTable->getBool("metal") ? 1.0 : 0.0);
        
        mat.shaderMaterial.x = (uint8_t)clamp(mat.color.x * 255.0, 0.0, 255.0);
        mat.shaderMaterial.y = (uint8_t)clamp(mat.color.y * 255.0, 0.0, 255.0);
        mat.shaderMaterial.z = (uint8_t)clamp(mat.color.z * 255.0, 0.0, 255.0);
        
        if(mat.roughnessMetal.y > 0.5)
        {
            mat.shaderMaterial.w = (uint8_t)128 + (uint8_t)clamp(mat.roughnessMetal.x * 127.0, 0.0, 127.0);
        }
        else
        {
            mat.shaderMaterial.w = (uint8_t)clamp(mat.roughnessMetal.x * 127.0, 0.0, 127.0);
        }
        
        mat.shaderMaterialB = mat.shaderMaterial;
        
        if(matTable->hasKey("colorB"))
        {
            dvec3 colorB = matTable->getVec3("colorB");
            mat.shaderMaterialB.x = (uint8_t)clamp(colorB.x * 255.0, 0.0, 255.0);
            mat.shaderMaterialB.y = (uint8_t)clamp(colorB.y * 255.0, 0.0, 255.0);
            mat.shaderMaterialB.z = (uint8_t)clamp(colorB.z * 255.0, 0.0, 255.0);
        }
        
        if(matTable->hasKey("roughnessB") || matTable->hasKey("metalB"))
        {
            float roughnessB = mat.roughnessMetal.x;
            bool metalB = mat.roughnessMetal.y > 0.5;
            if(matTable->hasKey("roughnessB"))
            {
                roughnessB = matTable->getDouble("roughnessB");
            }
            if(matTable->hasKey("metalB"))
            {
                metalB = matTable->getBool("metalB") ? 1.0 : 0.0;
            }
            
            if(metalB > 0.5)
            {
                mat.shaderMaterialB.w = (uint8_t)128 + (uint8_t)clamp(roughnessB * 127.0, 0.0, 127.0);
            }
            else
            {
                mat.shaderMaterialB.w = (uint8_t)clamp(roughnessB* 127.0, 0.0, 127.0);
            }
            
        }
    }
    
    delete materialsTable;
}


MaterialManager::~MaterialManager()
{
}

const std::string& MaterialManager::getDecalMaterial(const std::string& matName)
{
    if(materialsByName.count(matName) == 0)
    {
        static const std::string nilString = "";
        return nilString;
    }
    return materialsByName[matName].decalMaterialType;
}

bool MaterialManager::getIsDecal(const std::string& matName)
{
    if(materialsByName.count(matName) == 0)
    {
        return false;
    }
    return materialsByName[matName].isDecal;
}

bool MaterialManager::getHasEdgeDecal(const std::string& matName)
{
    if(materialsByName.count(matName) == 0)
    {
        return false;
    }
    return materialsByName[matName].hasEdgeDecal;
}

EdgeDecalInfo MaterialManager::getEdgeDecalInfo(const std::string& matName)
{
    EdgeDecalInfo decalInfo;
    decalInfo.size = vec2(0.0);
    
    if(materialsByName.count(matName) != 0)
    {
        decalInfo = materialsByName[matName].edgeDecalInfo;
    }
    
    return decalInfo;
    
}




u8vec4 MaterialManager::getShaderMaterial(const std::string& matName)
{
	 return materialsByName[matName].shaderMaterial;
}

u8vec4 MaterialManager::getShaderMaterialB(const std::string& matName)
{
	return materialsByName[matName].shaderMaterialB;
}

