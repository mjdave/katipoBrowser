//
//  Model.cpp
//  World
//
//  Created by David Frampton on 27/06/16.
//  Copyright © 2016 Majic Jungle. All rights reserved.
//

#include "Model.h"
#include "FileUtils.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "MaterialManager.h"
#include "TuiScript.h"

#include "gtx/spline.hpp"


#define DUMP_JSON 0

const mat4 gltfExportUnmangleMatrix = mat4(vec4(1.0, 0.0, 0.0, 0.0),
                                     vec4(0.0, 0.0, 1.0, 0.0),
                                     vec4(0.0, -1.0, 0.0, 0.0),
                                     vec4(0.0, 0.0, 0.0, 1.0));

struct GLTFBufferView {
    int buffer;
    size_t byteLength;
    size_t byteOffset;
};

struct GLTFAccessor {
    int bufferView;
    size_t byteOffset;
    int componentType;
    int componentSize;
    size_t count;
    int numberOfComponents;
    vec3 max;
    vec3 min;
};

struct GLTFMorphTarget {
    int positionAccessor;
    int normalAccessor;
};

struct GLTFPrimitive {
    int positionAccessor;
    int normalAccessor;
    int texCoord0Accessor;
	int weight0Accessor;
    int jointAccessor;
    int indexAccessor;
    int material;
    std::vector<GLTFMorphTarget> morphTargets;
    
};

struct GLTFJoint {
    std::vector<GLTFJoint> children;
};

struct GLTFMesh {
    int meshIndex;
    std::vector<GLTFPrimitive> primitives;
};

static inline int componenetSizeFromType(int componentType)
{
    switch(componentType)
    {
        case 5120:
        case 5121:
            return 1;
            break;
        case 5122:
        case 5123:
            return 2;
            break;
        case 5125:
        case 5126:
            return 4;
            break;
            default:
            break;
    }
    
    return 1;
}

static inline uint32_t getIntegerFromBuffer(int componentType, const char* buffer, size_t loc)
{
    uint32_t index = 0;
    switch(componentType)
    {
        case 5120:
            index = *((int8_t*)&(buffer[loc]));
            break;
        case 5121:
            index = *((uint8_t*)&(buffer[loc]));
            break;
        case 5122:
            index = *((int16_t*)&(buffer[loc]));
            break;
        case 5123:
            index = *((uint16_t*)&(buffer[loc]));
            break;
        case 5125:
            index = *((uint32_t*)&(buffer[loc]));
            break;
        default:
            break;
    }
    return index;
}

void recursivelySetupBoneIndexMapForSkeleton(ModelSkeleton* skeleton, ModelBone* bone)
{
	skeleton->bonesByIndex[bone->boneIndex] = bone;
    
    for(ModelBone& childBone : bone->children)
    {
		recursivelySetupBoneIndexMapForSkeleton(skeleton, &childBone);
    }
}

void setupBoneIndexMapForSkeleton(ModelSkeleton* skeleton)
{
    for(ModelBone& childBone : skeleton->rootBones)
    {
		recursivelySetupBoneIndexMapForSkeleton(skeleton, &childBone);
    }
}

ModelPlaceHolder loadPlaceHolderFromNode(const rapidjson::Value& node, bool demangle)
{
    ModelPlaceHolder placeHolder;
    
    placeHolder.rotation = quat(1,0,0,0);
    if(node.HasMember("rotation"))
    {
        const rapidjson::Value& rotationIn = node["rotation"];
        for (rapidjson::SizeType i = 0; i < rotationIn.Size() && i < 4; i++)
        {
            placeHolder.rotation[i] = rotationIn[i].GetFloat();
        }

		/*if(demangle)
		{
			//placeHolder.rotation = inverse(placeHolder.rotation);
			//placeHolder.rotation = mat3(parentBone->rotation) * mat3(placeHolder.rotation);
			//placeHolder.rotation = toQuat(inverse(mat3((*inverseBindMatrices)[parentBone->boneIndex])) * mat3(placeHolder.rotation));
			//placeHolder.rotation = inverse(*parentRotation) * mat3(placeHolder.rotation);
			placeHolder.rotation = toQuat(gltfExportUnmangleMatrix * mat4(placeHolder.rotation));

			//inverseBindMatrices[]
		} */
        
        /*if(demangle)
        {
            placeHolder.rotation = toQuat(gltfExportUnmangleMatrix * mat4(placeHolder.rotation));
        }*/
    }
    placeHolder.translation = vec3();
    if(node.HasMember("translation"))
    {
        const rapidjson::Value& translationIn = node["translation"];
        for (rapidjson::SizeType i = 0; i < translationIn.Size() && i < 3; i++)
        {
            placeHolder.translation[i] = translationIn[i].GetFloat();
        }
        
        /*if(demangle)
        {
            placeHolder.translation = vec3(gltfExportUnmangleMatrix * vec4(placeHolder.translation, 1.0));
        }*/
    }
    placeHolder.scale = vec3(1.0);
    if(node.HasMember("scale"))
    {
        const rapidjson::Value& scaleIn = node["scale"];
        for (rapidjson::SizeType i = 0; i < scaleIn.Size() && i < 3; i++)
        {
            placeHolder.scale[i] = fabsf(scaleIn[i].GetFloat());
        }
        
        /*if(demangle)
        {
            placeHolder.scale = vec3(gltfExportUnmangleMatrix * vec4(placeHolder.scale, 1.0));
        }*/
    }
    
    return placeHolder;
}

void recursivelyLoadBone(ModelBone* bone,
                         const rapidjson::Value& boneNode,
                         const rapidjson::Document& document,
                         std::map<int, int>& nodeIndicesToBoneIndicesMap,
                         std::map<std::string, ModelPlaceHolder>& modelPlaceHolders,
                         std::map<std::string, int>& bonesIndicesByName)
{
    bone->rotation = quat(1,0,0,0);
    if(boneNode.HasMember("rotation"))
    {
        const rapidjson::Value& rotationIn = boneNode["rotation"];
        for (rapidjson::SizeType i = 0; i < rotationIn.Size() && i < 4; i++)
        {
            bone->rotation[i] = rotationIn[i].GetFloat();
        }
    }
    bone->translation = vec3();
    if(boneNode.HasMember("translation"))
    {
        const rapidjson::Value& translationIn = boneNode["translation"];
        for (rapidjson::SizeType i = 0; i < translationIn.Size() && i < 3; i++)
        {
            bone->translation[i] = translationIn[i].GetFloat();
        }
    }
    
    if(boneNode.HasMember("name"))
    {
        bonesIndicesByName[boneNode["name"].GetString()] = bone->boneIndex;
    }
    
    if(boneNode.HasMember("children"))
    {
        const rapidjson::Value& childrenIn = boneNode["children"];
        for (rapidjson::SizeType i = 0; i < childrenIn.Size(); i++)
        {
            int childNodeIndex = childrenIn[i].GetInt();
            
            if(nodeIndicesToBoneIndicesMap.count(childNodeIndex) != 0)
            {
                ModelBone childBone;
                childBone.boneIndex = nodeIndicesToBoneIndicesMap[childNodeIndex];
                const rapidjson::Value& childNode = document["nodes"][childNodeIndex];
                recursivelyLoadBone(&childBone, childNode, document, nodeIndicesToBoneIndicesMap, modelPlaceHolders, bonesIndicesByName);
                bone->children.push_back(childBone);
            }
            else
            {
                const rapidjson::Value& childNode = document["nodes"][childNodeIndex];
                
                if(childNode.HasMember("name"))
                {
                    ModelPlaceHolder placeHolder = loadPlaceHolderFromNode(childNode, true);
                    
                    placeHolder.boneIndex = bone->boneIndex;
                    placeHolder.boneLengthOffset = 0.0f;

					//MJLog("loaded placeholder:%s", childNode["name"].GetString());
					//printMat3(mat3(placeHolder.rotation));
					


                    /*if(boneNode.HasMember("extras"))
                    {
						const rapidjson::Value& extrasNode = boneNode["extras"];
						if(extrasNode.HasMember("length"))
						{
							const rapidjson::Value& lengthIn = extrasNode["length"];
							placeHolder.boneLengthOffset = lengthIn.GetFloat();
						}
                    }*/
                    
                    modelPlaceHolders[childNode["name"].GetString()] = placeHolder;
                }
            }
        }
    }
}

Model::Model(const std::string& modelPath_,
	MaterialManager* materialManager)
{
    
    modelPath = modelPath_;
    
    max = vec3(-DBL_MAX,-DBL_MAX,-DBL_MAX);
    min = vec3(DBL_MAX,DBL_MAX,DBL_MAX);
    center = vec3(0.0,0.0,0.0);
    hasVolume = false;
    hasUVs = false;
    morphTargetCount = 0;
    hasBones = false;
    
    std::vector<std::string> pathnames;
    std::vector<std::string> boneNames;
    std::map<std::string,std::string> materialRemaps;
    
    MJLog("DAVE:%s", fileExtensionFromPath(modelPath).c_str());
    
    if(fileExtensionFromPath(modelPath) == ".glb")
    {
        pathnames.push_back(modelPath);
    }
    else
    {
        TuiTable* modelTable = (TuiTable*)TuiRef::load(modelPath, nullptr);
        
        if(!modelTable)
        {
            MJLog("no model found:%s", modelPath.c_str());
            return;
        }
        
        
        
        if(modelTable->hasKey("compositeInfo")) //todo
        {
            /*int index = 1;
             LuaRef luaTable = compositeInfoOrNil[index];
             
             while(!luaTable.isNil())
             {
             std::string pathname = luaTable["path"];
             
             pathname = getResourcePath("models/" + pathname);
             
             pathnames.push_back(pathname);
             
             LuaRef luaBoneName = luaTable["boneName"];
             if(!luaBoneName.isNil())
             {
             boneNames.push_back(luaBoneName);
             }
             else
             {
             boneNames.push_back("");
             }
             
             index++;
             luaTable = compositeInfoOrNil[index];
             }*/
        }
        else
        {
            if(modelTable->hasKey("filePath"))
            {
                std::string directory = pathByRemovingLastPathComponent(modelPath);
                pathnames.push_back(directory + modelTable->getString("filePath"));
            }
            else
            {
                pathnames.push_back(changeExtensionForPath(modelPath, ".glb"));
            }
            /*std::string pathname = modelTable["path"];
             pathname = getResourcePath("models/" + pathname);
             
             pathnames.push_back(pathname);*/
        }
        
        /* //todo
        LuaRef boolOrNil = modelTable["hasVolume"];
        if(boolOrNil)
        {
            hasVolume = true;
        }

        windStrengthMain = 1.0;
        windStrengthDecals = 1.0;

        LuaRef windStrengthOrNil = modelTable["windStrength"];
        if(!windStrengthOrNil.isNil())
        {
            dvec2 windStrength = windStrengthOrNil;
            windStrengthMain = windStrength.x;
            windStrengthDecals = windStrength.y;
        }
        
        LuaRef remapTable = modelTable["materialRemap"];*/
        
        delete modelTable;
    }
    

	debugName = "";

	std::set<int> badEdges;

    int pathIndex = 0;
    for(auto pathname : pathnames)
    {
        if(fileSizeAtPath(pathname) <= 0)
        {
            MJLog("Can't find model file at path:%s", pathname.c_str());
            return;
        }

		//MJLog("loading pathname:%s", pathname.c_str());
        
        if(fileExtensionFromPath(pathname) == ".glb")
        {
			if(debugName.empty())
			{
				debugName = fileNameFromPath(pathname);
			}
			else
			{
				debugName = debugName + " + " + fileNameFromPath(pathname);
			}

            int compositePathBoneJoint = -1;
            if(pathIndex != 0)
            {
                std::string boneName = boneNames[pathIndex];
                if(!boneName.empty() && bonesIndicesByName.count(boneName) != 0)
                {
                    compositePathBoneJoint = bonesIndicesByName[boneName];
                }
            }
            
            std::ifstream f((pathname).c_str(), std::ios::in | std::ios::binary);
            
            f.seekg(0, f.end);
            size_t fileSize = static_cast<size_t>(f.tellg());
            std::vector<char> fileBuffer(fileSize);
            
            f.seekg(0, f.beg);
            f.read(&fileBuffer.at(0), static_cast<std::streamsize>(fileSize));
            f.close();
            
            const char* fileBufferBytes = reinterpret_cast<char *>(&fileBuffer.at(0));
            
            unsigned int jsonLength;
            memcpy(&jsonLength, fileBufferBytes + 12, 4);
            
            std::string jsonString(&fileBufferBytes[20],
                                   jsonLength);
            
#if DUMP_JSON
            std::string jsonPath = getSavePath("debug/" + changeExtensionForPath(fileNameFromPath(pathname), "gltf"));
            createDirectoriesIfNeededForFilePath(jsonPath);
            std::ofstream ofs((jsonPath), std::ios::out | std::ios::trunc);
            ofs.write((const char*)(&fileBufferBytes[20]), jsonLength);
            MJLog("model json written to:%s", jsonPath.c_str());
#endif
            
            rapidjson::Document document;
            if(document.Parse(jsonString.c_str()).HasParseError())
            {
                MJLog("error parsing json within glb file %s", pathname.c_str());
            }
            
            if(document.IsObject())
            {
				//MJLog("loading model:%s", debugName.c_str())
                try
                {
                    std::vector<const char*> buffers;
                    std::vector<std::string> materials;
                    
                    int discardMaterial = -1;
                    
                    std::vector<GLTFBufferView> bufferViews;
                    std::vector<GLTFAccessor> accessors;
                    std::vector<GLTFMesh> meshes;
                    
                    std::string basePath = pathByRemovingLastPathComponent(pathname);
                    
                    if(document.HasMember("buffers"))
                    {
                        const rapidjson::Value& bufferNamesIn = document["buffers"];
                        for (rapidjson::SizeType i = 0; i < bufferNamesIn.Size(); i++)
                        {
                            if(bufferNamesIn[i].HasMember("uri"))
                            {
                                
                                MJLog("external buffers in GLTF files are not supported. Found while loading %s", pathname.c_str());
                                return;
                            }
                            else
                            {
                                const char* buffer = fileBufferBytes + 20 + jsonLength + 8;
                                buffers.push_back(buffer);
                            }
                        }
                        
                        const rapidjson::Value& bufferViewsIn = document["bufferViews"];
                        for (rapidjson::SizeType i = 0; i < bufferViewsIn.Size(); i++)
                        {
                            GLTFBufferView bufferView;
                            bufferView.buffer = bufferViewsIn[i]["buffer"].GetInt();
                            bufferView.byteLength = bufferViewsIn[i]["byteLength"].GetInt();
                            bufferView.byteOffset = bufferViewsIn[i]["byteOffset"].GetInt();
                            bufferViews.push_back(bufferView);
                        }
                        
                        
                        const rapidjson::Value& accessorsIn = document["accessors"];
                        for (rapidjson::SizeType i = 0; i < accessorsIn.Size(); i++)
                        {
                            GLTFAccessor accessor;
                            accessor.bufferView = accessorsIn[i]["bufferView"].GetInt();
                            accessor.byteOffset = (accessorsIn[i].HasMember("byteOffset") ? accessorsIn[i]["byteOffset"].GetInt() : 0);
                            accessor.componentType = accessorsIn[i]["componentType"].GetInt();
                            accessor.componentSize = componenetSizeFromType(accessor.componentType);
                            accessor.count = accessorsIn[i]["count"].GetInt();
                            
                            if(accessorsIn[i].HasMember("max"))
                            {
                                const rapidjson::Value& maxComponentsIn = accessorsIn[i]["max"];
                                for (rapidjson::SizeType j = 0; j < maxComponentsIn.Size() && j < 3; j++)
                                {
                                    accessor.max[j] = maxComponentsIn[j].GetFloat();
                                }
                            }
                            if(accessorsIn[i].HasMember("min"))
                            {
                                const rapidjson::Value& minComponentsIn = accessorsIn[i]["min"];
                                for (rapidjson::SizeType j = 0; j < minComponentsIn.Size() && j < 3; j++)
                                {
                                    accessor.min[j] = minComponentsIn[j].GetFloat();
                                }
                            }
                            
                            std::string type = accessorsIn[i]["type"].GetString();
                            int components = 1;
                            
                            if(type == "VEC2")
                            {
                                components = 2;
                            }
                            else if(type == "VEC3")
                            {
                                components = 3;
                            }
                            else if(type == "VEC4")
                            {
                                components = 4;
                            }
                            else if(type == "MAT2")
                            {
                                components = 4;
                            }
                            else if(type == "MAT3")
                            {
                                components = 9;
                            }
                            else if(type == "MAT4")
                            {
                                components = 16;
                            }
                            
                            accessor.numberOfComponents = components;
                            
                            accessors.push_back(accessor);
                        }
                        
                        
                        const rapidjson::Value& materialsIn = document["materials"];
                        for (rapidjson::SizeType i = 0; i < materialsIn.Size(); i++)
                        {
                            std::string name = materialsIn[i]["name"].GetString();
                            
                            if(name == "discard")
                            {
                                discardMaterial = (int)materials.size();
                                materials.push_back(name);
                            }
                            else
                            {
                                if(materialRemaps.count(name) != 0)
                                {
                                    name = materialRemaps[name];
                                }
                                
                                
                                if(materialManager->materialsByName.count(name) != 0)
                                {
                                    materials.push_back(name);
                                }
                                else
                                {
                                    MJLog("WARNING: material not found:%s for model:%s", name.c_str(), pathname.c_str());
                                    materials.push_back("");
                                }
                            }
                        }
                        
                        const rapidjson::Value& meshesIn = document["meshes"];
                        for (rapidjson::SizeType i = 0; i < meshesIn.Size(); i++)
                        {
                            GLTFMesh mesh;
                            const rapidjson::Value& primitivesIn = meshesIn[i]["primitives"];
                            for (rapidjson::SizeType i = 0; i < primitivesIn.Size(); i++)
                            {
                                if(primitivesIn[i].HasMember("mode") && primitivesIn[i]["mode"].GetInt() != 4)
                                {
                                    MJLog("WARNING: only triangles supported, primitives of type:%d will be skipped in GLTF file:%s", primitivesIn[i]["mode"].GetInt(), pathname.c_str());
                                }
                                else
                                {
                                    GLTFPrimitive primitive;
                                    primitive.indexAccessor = primitivesIn[i]["indices"].GetInt();
                                    primitive.material = primitivesIn[i]["material"].GetInt();
                                    primitive.positionAccessor = primitivesIn[i]["attributes"]["POSITION"].GetInt();
                                    primitive.normalAccessor = primitivesIn[i]["attributes"]["NORMAL"].GetInt();
                                    primitive.jointAccessor = -1;
                                    primitive.texCoord0Accessor = -1;
									primitive.weight0Accessor = -1;
                                    
                                    if(primitivesIn[i]["attributes"].HasMember("TEXCOORD_0"))
                                    {
                                        primitive.texCoord0Accessor = primitivesIn[i]["attributes"]["TEXCOORD_0"].GetInt();
                                        hasUVs = true;
                                    }

									if(primitivesIn[i]["attributes"].HasMember("WEIGHTS_0"))
									{
										primitive.weight0Accessor = primitivesIn[i]["attributes"]["WEIGHTS_0"].GetInt();
									}
                                    
                                    if(primitivesIn[i]["attributes"].HasMember("JOINTS_0"))
                                    {
                                        primitive.jointAccessor = primitivesIn[i]["attributes"]["JOINTS_0"].GetInt();

                                    }
                                    if(primitivesIn[i].HasMember("targets"))
                                    {
                                        const rapidjson::Value& targetsIn = primitivesIn[i]["targets"];
                                        for (rapidjson::SizeType i = 0; i < targetsIn.Size(); i++)
                                        {
                                            GLTFMorphTarget target;
                                            target.positionAccessor = targetsIn[i]["POSITION"].GetInt();
                                            target.normalAccessor = targetsIn[i]["NORMAL"].GetInt();
                                            primitive.morphTargets.push_back(target);
                                        }
                                        
                                        morphTargetCount = MAX(morphTargetCount, (int)targetsIn.Size());
                                    }
                                    mesh.primitives.push_back(primitive);
                                }
                            }
                            meshes.push_back(mesh);
                        }
                    }

					const rapidjson::Value& nodes = document["nodes"];
					for (rapidjson::SizeType i = 0; i < nodes.Size(); i++)
					{
						const rapidjson::Value& node = nodes[i];
						if(node.HasMember("name"))
						{
							std::string name = node["name"].GetString();
							if(name == "icon_camera")
							{
								ModelPlaceHolder placeHolder = loadPlaceHolderFromNode(node, false);
								placeHolder.rotation = rotate(placeHolder.rotation, -0.5f * (float)M_PI, vec3(1.0, 0.0, 0.0));
								modelPlaceHolders[name] = placeHolder;
							}
							else if(name == "icon_camera2")
							{
								ModelPlaceHolder placeHolder = loadPlaceHolderFromNode(node, false);
								//placeHolder.rotation = rotate(placeHolder.rotation, -0.5f * (float)M_PI, vec3(1.0, 0.0, 0.0));
								modelPlaceHolders["icon_camera"] = placeHolder;
							}
						}
					}

                    
                    const rapidjson::Value& scenesIn = document["scenes"];
                    for (rapidjson::SizeType i = 0; i < scenesIn.Size(); i++)
                    {
                        const rapidjson::Value& nodesIn = scenesIn[i]["nodes"];
                        for (rapidjson::SizeType j = 0; j < nodesIn.Size(); j++)
                        {
                            int nodeIndex = nodesIn[j].GetInt();
                            const rapidjson::Value& node = nodes[nodeIndex];

                            if(!node.HasMember("children") &&
                               !node.HasMember("mesh") &&
                               node.HasMember("name"))
                            {
                                std::string name = node["name"].GetString();
                                if(name != "Lamp" && name != "Camera" && name != "Light")
                                {
                                    ModelPlaceHolder placeHolder;
									if(modelPlaceHolders.count(name) == 0) //may have been loaded earlier if it had a bone parent, though this code doesn't seem to be actually working :/
									{
										placeHolder = loadPlaceHolderFromNode(node, false);
										modelPlaceHolders[name] = placeHolder;
									}
									else
									{
										placeHolder = modelPlaceHolders[name];
									}
                                    
                                    dvec3 rotatedScale = abs(placeHolder.rotation * placeHolder.scale);
                                    
                                    
                                    if(name == "bounding_sphere" || name == "bounding_radius" || name == "bouding_sphere") //there is a typo in some model files
                                    {
										hasBoundingSphere = true;
										boundingSphere.pos = placeHolder.translation;
										boundingSphere.radius = placeHolder.scale.x;
									}
									else if(name == "dynamic_sphere")
									{
										hasDynamicSphere = true;
										dynamicSphere.pos = placeHolder.translation;
										dynamicSphere.radius = placeHolder.scale.x;

										customSpheres["dynamic"].push_back(dynamicSphere);
									}
									else if(name == "dynamic_box")
									{
										hasDynamicBox = true;
										dynamicBox.pos = placeHolder.translation;
										dynamicBox.halfSize = placeHolder.scale;
										dynamicBox.rotation = dmat3(dquat(placeHolder.rotation));
										dynamicBox.rotationInverse = inverse(dynamicBox.rotation);

										customBoxes["dynamic"].push_back(dynamicBox);
									}
									else
									{
										bool found = false;
										std::string test;

										std::vector<std::string> splitName = splitString(name, '_');

										if(splitName.size() >= 2 && splitName[1].size() >= 3)
										{
											std::string type = splitName[1];

											test = "sphere";
											if(type.compare(0, test.length(), test) == 0)
											{
												ModelSphere sphere;
												sphere.pos = placeHolder.translation;
												sphere.radius = placeHolder.scale.x;
												customSpheres[splitName[0]].push_back(sphere);
												if(splitName[0] == "static")
												{
													staticSpheres.push_back(sphere);
												}
												//else
												//{
													customSpheres[splitName[0]].push_back(sphere);
												//}
												found = true;
											}

											test = "box";
											if(type.compare(0, test.length(), test) == 0)
											{
												ModelBox box;
												box.pos = placeHolder.translation;
												box.halfSize = placeHolder.scale;
												box.rotation = dmat3(dquat(placeHolder.rotation));
												if(splitName[0] == "static")
												{
													staticBoxes.push_back(box);
												}
												else if(splitName[0] == "pathNode")
												{
													pathNodeBoxes.push_back(box);
												}
												else if(splitName[0] == "walkNode")
												{
													pathWalkNodeBoxes.push_back(box);
												}
												else if(splitName[0] == "pathCollider")
												{
													pathColliderBoxes.push_back(box);
												}
												//else
												//{
													customBoxes[splitName[0]].push_back(box);
												//}
												found = true;
											}

											test = "cone";
											if(type.compare(0, test.length(), test) == 0)
											{
												ModelCone cone;
												cone.pos = placeHolder.translation;
												cone.radius = placeHolder.scale.x;
												cone.height = placeHolder.scale.z * 2.0;
												cone.rotation = placeHolder.rotation;
												mat3 baseRotation = mat3(cone.rotation);
												cone.rotation = rotate(cone.rotation, -0.5f * (float)M_PI, vec3(1.0, 0.0, 0.0));
												if(splitName[0] == "static")
												{
													staticCones.push_back(cone);
												}
												////else
												//{
													customCones[splitName[0]].push_back(cone);
											//	}
												found = true;
											}
										}


										if(!found)
										{
											max.x = MAX(max.x, placeHolder.translation.x + rotatedScale.x);
											max.y = MAX(max.y, placeHolder.translation.y + rotatedScale.y);
											max.z = MAX(max.z, placeHolder.translation.z + rotatedScale.z);
											min.x = MIN(min.x, placeHolder.translation.x - rotatedScale.x);
											min.y = MIN(min.y, placeHolder.translation.y - rotatedScale.y);
											min.z = MIN(min.z, placeHolder.translation.z - rotatedScale.z);
										}
									}
                                }
                            }
                        }
                    }
                    
                    
                    std::map<int, int> nodeIndicesToBoneIndicesMap;
                    
                    
                    const rapidjson::Value& nodesIn = document["nodes"];
                    for (rapidjson::SizeType i = 0; i < nodesIn.Size(); i++)
                    {
                        if(nodesIn[i].HasMember("mesh"))
                        {
							
							bool isShadowVolume = false;
							if(nodesIn[i].HasMember("name"))
							{
								std::string name = nodesIn[i]["name"].GetString();
								if(name == "shadowVolume")
								{
									isShadowVolume = true;
								}
							}
                            int meshIndex = nodesIn[i]["mesh"].GetInt();
                            
                            if(pathIndex == 0 && nodesIn[i].HasMember("skin"))
                            {
                                int skinIndex = nodesIn[i]["skin"].GetInt();
                                
                                const rapidjson::Value& skin = document["skins"][skinIndex];
                                int inverseBindMatricesIndex = skin["inverseBindMatrices"].GetInt();
                                GLTFAccessor* inverseBindMatricesAccessor = &accessors[inverseBindMatricesIndex];
                                GLTFBufferView* inverseBindMatricesBufferView = &bufferViews[inverseBindMatricesAccessor->bufferView];
                                const char* inverseBindMatricesBuffer = buffers[inverseBindMatricesBufferView->buffer];
                                
                                
                                const rapidjson::Value& jointsIn = skin["joints"];
                                
                                if(jointsIn.Size() > MAX_BONES)
                                {
                                    MJLog("ERROR loading GLTF file, too many bones: %d (of %d allowed) file:%s", jointsIn.Size(), MAX_BONES, pathname.c_str());
                                    return;
                                }
                                for (rapidjson::SizeType i = 0; i < jointsIn.Size(); i++)
                                {
                                    nodeIndicesToBoneIndicesMap[jointsIn[i].GetInt()] = i;
                                    
                                    mat4 inverseBindMatrix;
                                    
                                    size_t inverseBindMatrixBufferLocation0 = inverseBindMatricesBufferView->byteOffset + i * inverseBindMatricesAccessor->componentSize * inverseBindMatricesAccessor->numberOfComponents;
                                    
                                    for(int j = 0; j < 16; j++)
                                    {
                                        inverseBindMatrix[j / 4][j %4] = *((float*)&(inverseBindMatricesBuffer[inverseBindMatrixBufferLocation0 + j * inverseBindMatricesAccessor->componentSize]));
                                    }
                                    
                                    inverseBindMatrices.push_back(inverseBindMatrix);
									inverseBindMatrixOffsets.push_back(inverse(mat3(inverseBindMatrix)) * vec3(inverseBindMatrix[3]));
                                }
                                
                                hasBones = !nodeIndicesToBoneIndicesMap.empty();
                                
                                if(hasBones)
                                {
									const rapidjson::Value& jointsIn = skin["joints"];
									if(jointsIn.Size() > 0)
									{
										int rootNodeIndex = jointsIn[0].GetInt();
										const rapidjson::Value& rootNode = document["nodes"][rootNodeIndex];
                                            
										ModelBone rootBone;
										rootBone.boneIndex = nodeIndicesToBoneIndicesMap[rootNodeIndex];
										recursivelyLoadBone(&rootBone, rootNode, document, nodeIndicesToBoneIndicesMap, modelPlaceHolders, bonesIndicesByName);
										defaultSkeleton.rootBones.push_back(rootBone);
										defaultSkeleton.bonesByIndex[rootBone.boneIndex] = &(defaultSkeleton.rootBones[defaultSkeleton.rootBones.size() - 1]);
									}
                                }
                            }
                            
                            const GLTFMesh& mesh = meshes[meshIndex];
                            for(const GLTFPrimitive& primitive : mesh.primitives)
                            {
                                if(primitive.material == discardMaterial)
                                {
                                    continue;
                                }
                                GLTFAccessor* indexAccessor = &accessors[primitive.indexAccessor];
                                GLTFAccessor* positionAccessor = &accessors[primitive.positionAccessor];
                                GLTFAccessor* normalAccessor = &accessors[primitive.normalAccessor];
                                GLTFAccessor* jointAccessor = nullptr;
                                GLTFAccessor* uvAccessor = nullptr;
								GLTFAccessor* weightAccessor = nullptr;
                                
                                
                                GLTFBufferView* indexBufferView = &bufferViews[indexAccessor->bufferView];
                                GLTFBufferView* positionBufferView = &bufferViews[positionAccessor->bufferView];
                                GLTFBufferView* normalBufferView = &bufferViews[normalAccessor->bufferView];
                                GLTFBufferView* jointBufferView = nullptr;
                                GLTFBufferView* uvBufferView = nullptr;
								GLTFBufferView* weightBufferView = nullptr;
                                
                                const char* indexBuffer = buffers[indexBufferView->buffer];
                                const char* positionBuffer = buffers[positionBufferView->buffer];
                                const char* normalBuffer = buffers[normalBufferView->buffer];
                                const char* jointBuffer = nullptr;
                                const char* uvBuffer = nullptr;
								const char* weightBuffer = nullptr;
                                
                                if(primitive.jointAccessor != -1)
                                {
                                    jointAccessor = &accessors[primitive.jointAccessor];
                                    jointBufferView = &bufferViews[jointAccessor->bufferView];
                                    jointBuffer = buffers[jointBufferView->buffer];
                                }
                                
                                
                                if(primitive.texCoord0Accessor != -1)
                                {
                                    uvAccessor = &accessors[primitive.texCoord0Accessor];
                                    uvBufferView = &bufferViews[uvAccessor->bufferView];
                                    uvBuffer = buffers[uvBufferView->buffer];
                                }

								if(primitive.weight0Accessor != -1)
								{
									weightAccessor = &accessors[primitive.weight0Accessor];
									weightBufferView = &bufferViews[weightAccessor->bufferView];
									weightBuffer = buffers[weightBufferView->buffer];
								}
                                
                                max.x = MAX(max.x, positionAccessor->max.x);
                                max.y = MAX(max.y, positionAccessor->max.y);
                                max.z = MAX(max.z, positionAccessor->max.z);
                                min.x = MIN(min.x, positionAccessor->min.x);
                                min.y = MIN(min.y, positionAccessor->min.y);
                                min.z = MIN(min.z, positionAccessor->min.z);
                                
                                const std::string& materialType = materials[primitive.material];
								u8vec4 material = materialManager->getShaderMaterial(materialType);
								u8vec4 materialB = materialManager->getShaderMaterialB(materialType);
                                
                                size_t indexCount = indexAccessor->count;

								bool isDecalMaterial = false;//materialManager->getIsDecalForMaterialType(materialTypeIndex);
                                
                                
                                int faceCount = (int)indexCount / 3;
                                
                                for(int f = 0 ; f < faceCount; f++)
                                {
                                    ModelFace currentFace;
                                    currentFace.shaderMaterial = material;
									currentFace.shaderMaterialB = materialB;
                                    currentFace.materialTypeIndex = materialManager->materialIDsByName[materialType];
									bool discardFaceDueToEdgeDecalInfo = false;
                                    
                                    for(int vIndex = 0; vIndex < 3; vIndex++)
                                    {
                                        int indexBufferIndex = f * 3 + vIndex;
                                        size_t indexBufferLocation = indexBufferView->byteOffset + indexBufferIndex * indexAccessor->componentSize * indexAccessor->numberOfComponents;
                                        uint32_t index = getIntegerFromBuffer(indexAccessor->componentType, indexBuffer, indexBufferLocation);
                                        
                                        if(hasBones)
                                        {
											currentFace.boneMixes[2 - vIndex] = 0;
											if(compositePathBoneJoint >= 0)
											{
												currentFace.bones[2 - vIndex][0] = compositePathBoneJoint;
												currentFace.bones[2 - vIndex][1] = compositePathBoneJoint;
											}
											else
											{
												if(jointBuffer)
												{
													size_t jointBufferLocation = jointBufferView->byteOffset + index * jointAccessor->componentSize * jointAccessor->numberOfComponents;
													uint32_t joint = getIntegerFromBuffer(jointAccessor->componentType, jointBuffer, jointBufferLocation);
													currentFace.bones[2 - vIndex][0] = joint;
													currentFace.bones[2 - vIndex][1] = joint;

													if(jointAccessor->numberOfComponents > 1)
													{
														if(weightBuffer && weightAccessor->numberOfComponents > 1)
														{
															vec2 weights;

															size_t bufferLocationX = weightBufferView->byteOffset + index * weightAccessor->componentSize * weightAccessor->numberOfComponents;
															size_t bufferLocationY = bufferLocationX + weightAccessor->componentSize;

															weights.x = *((float*)&(weightBuffer[bufferLocationX]));
															weights.y = *((float*)&(weightBuffer[bufferLocationY]));


															if(weights.x < 0.99 && weights.x > 0.01)
															{
																//MJLog("%s raw weights:%.2f, %.2f", debugName.c_str(), weights.x, weights.y)
																size_t jointBufferLocationB = jointBufferLocation + jointAccessor->componentSize;
																uint32_t jointB = getIntegerFromBuffer(jointAccessor->componentType, jointBuffer, jointBufferLocationB);
																currentFace.bones[2 - vIndex][1] = jointB;

																currentFace.boneMixes[2 - vIndex] = clamp((int)(255 * (1.0 - weights.x)), 0, 255);
																//MJLog("mix:%d", currentFace.boneMixes[2 - vIndex])
															}
														}
													}

												}
											}
                                        }
                                        
                                        vec3 position;
                                        
                                        size_t positionBufferLocationX = positionBufferView->byteOffset + index * positionAccessor->componentSize * positionAccessor->numberOfComponents;
                                        size_t positionBufferLocationY = positionBufferLocationX + positionAccessor->componentSize;
                                        size_t positionBufferLocationZ = positionBufferLocationY + positionAccessor->componentSize;
                                        
                                        position.x = *((float*)&(positionBuffer[positionBufferLocationX]));
                                        position.y = *((float*)&(positionBuffer[positionBufferLocationY]));
                                        position.z = *((float*)&(positionBuffer[positionBufferLocationZ]));
                                        
                                        if(hasBones)
                                        {

											//position = inverseBindMatrices[currentFace.bones[2 - vIndex][0]] * vec4(position, 1.0);

											//position = mat3(inverseBindMatrices[currentFace.bones[2 - vIndex][0]]) * position;
											//position = position + vec3(inverseBindMatrices[currentFace.bones[2 - vIndex][0]][3]);
                                        }
                                        
                                        currentFace.verts[2 - vIndex] = position;
                                        
                                        vec3 normal;
                                        
                                        size_t normalBufferLocationX = normalBufferView->byteOffset + index * normalAccessor->componentSize * normalAccessor->numberOfComponents;
                                        size_t normalBufferLocationY = normalBufferLocationX + normalAccessor->componentSize;
                                        size_t normalBufferLocationZ = normalBufferLocationY + normalAccessor->componentSize;
                                        
                                        normal.x = *((float*)&(normalBuffer[normalBufferLocationX]));
                                        normal.y = *((float*)&(normalBuffer[normalBufferLocationY]));
                                        normal.z = *((float*)&(normalBuffer[normalBufferLocationZ]));
                                        
                                        if(hasBones)
                                        {


                                           // vec3 newNormal = mat3(inverseBindMatrices[currentFace.bones[2 - vIndex][0]]) * normal;
                                           // normal = normalize(vec3(newNormal));
											//vec3 newNormal = (normalize(toQuat(inverseBindMatrices[currentFace.bones[2 - vIndex][0]]))) * normal;
											//normal = normalize(vec3(newNormal));
                                        }
                                        
                                        currentFace.normals[2 - vIndex] = normal;
                                        
                                        if(hasUVs && uvAccessor)
                                        {
                                            vec2 uv;
                                            
                                            size_t bufferLocationX = uvBufferView->byteOffset + index * uvAccessor->componentSize * uvAccessor->numberOfComponents;
                                            size_t bufferLocationY = bufferLocationX + uvAccessor->componentSize;
                                            
                                            uv.x = *((float*)&(uvBuffer[bufferLocationX]));
                                            uv.y = 1.0 - *((float*)&(uvBuffer[bufferLocationY]));
                                            
                                            currentFace.uvs[2 - vIndex] = uv;
                                        }
										else
										{
											currentFace.uvs[2 - vIndex] = vec2(0,0);
										}
                                    }

									if(hasUVs)
									{
										vec3& v0 = currentFace.verts[0];
										vec3& v1 = currentFace.verts[1];
										vec3& v2 = currentFace.verts[2];

										vec2& uv0 = currentFace.uvs[0];
										vec2& uv1 = currentFace.uvs[1];
										vec2& uv2 = currentFace.uvs[2];

										vec3 deltaPos1 = v1-v0;
										vec3 deltaPos2 = v2-v0;

										vec2 deltaUV1 = uv1-uv0;
										vec2 deltaUV2 = uv2-uv0;

										float deltaTotal = (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

										if(epsilonEqual(deltaTotal, 0.0f, 0.000001f))
										{
											vec3 tangent = normalize(currentFace.verts[1] - currentFace.verts[0]);
											for(int i = 0; i < 3; i++)
											{
												currentFace.tangents[i] = tangent;
											}
										}
										else
										{
											float r = 1.0f / deltaTotal;
											vec3 tangent = (deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y)*r;

											for(int i = 0; i < 3; i++)
											{
												currentFace.tangents[i] = tangent;
											}
										}
									}
									else
									{
										vec3 tangent = normalize(currentFace.verts[1] - currentFace.verts[0]);
										for(int i = 0; i < 3; i++)
										{
											currentFace.tangents[i] = tangent;
										}
									}
                                    
                                    
                                    vec3 ca = currentFace.verts[2] - currentFace.verts[0];
                                    vec3 ba = currentFace.verts[1] - currentFace.verts[0];
                                    currentFace.faceNormal = normalize(cross(ca, ba));
                                    
                                    if(!isDecalMaterial && materialManager->getHasEdgeDecal(materialType))
                                    {
                                        ModelEdgeDecal edgeDecal;
                                        EdgeDecalInfo decalInfo = materialManager->getEdgeDecalInfo(materialType);
										int texIndex = 0;

										discardFaceDueToEdgeDecalInfo = decalInfo.discardFaces;
                                        
                                        for(int v = 0; v < 3; v++)
                                        {
                                            int indexA = v;
                                            int indexB = (v + 1) % 3;
                                            vec3 edgeVertABase = currentFace.verts[indexA];
                                            vec3 edgeVertBBase = currentFace.verts[indexB];

											if(hasUVs && !approxEqual(currentFace.uvs[indexA].x, currentFace.uvs[indexB].x))
											{
												if(currentFace.uvs[indexA].x > currentFace.uvs[indexB].x)
												{
													indexA = indexB;
													indexB = v;
													edgeVertABase = currentFace.verts[indexA];
													edgeVertBBase = currentFace.verts[indexB];
												}
											}
                                            else if(edgeVertBBase.y * 100.0 + edgeVertBBase.x + edgeVertBBase.z * 0.13 > edgeVertABase.y * 100.0 + edgeVertABase.x + edgeVertABase.z * 0.13)
                                            {
                                                indexA = indexB;
                                                indexB = v;
                                                edgeVertABase = currentFace.verts[indexA];
                                                edgeVertBBase = currentFace.verts[indexB];
                                            }
                                            vec3 normalA = currentFace.normals[indexA];
                                            vec3 normalB = currentFace.normals[indexB];



											int16_t baseAX = (edgeVertABase.x + 0.0005) * 1000;
											int16_t baseAY = (edgeVertABase.y + 0.0005) * 1000;
											int16_t baseAZ = (edgeVertABase.z + 0.0005) * 1000;

											uint64_t hashAX = baseAX;
											uint64_t hashAY = baseAY;
											uint64_t hashAZ = baseAZ;

											uint64_t hashA = (hashAX & 0xffff) | ((hashAY & 0xffff) << 16) | ((hashAZ & 0xffff) << 32);

											int16_t baseBX = (edgeVertBBase.x + 0.0005) * 1000;
											int16_t baseBY = (edgeVertBBase.y + 0.0005) * 1000;
											int16_t baseBZ = (edgeVertBBase.z + 0.0005) * 1000;

											uint64_t hashBX = baseBX;
											uint64_t hashBY = baseBY;
											uint64_t hashBZ = baseBZ;

											uint64_t hashB = (hashBX & 0xffff) | ((hashBY & 0xffff) << 16) | ((hashBZ & 0xffff) << 32);
                                            
                                            
                                            bool found = false;
                                            int e = 0;
                                            for(ModelEdgeDecal& otherEdge : edgeDecals)
                                            {

												if((hashA == otherEdge.posHashA && hashB == otherEdge.posHashB) || (hashB == otherEdge.posHashA && hashA == otherEdge.posHashB))
                                                {
                                                    otherEdge.partnerFound = true;
                                                    found = true;
                                                    if(otherEdge.materialTypeIndex != materialManager->materialIDsByName[materialType])
                                                    {
                                                        badEdges.insert(e);
                                                    }
                                                    break;
                                                }
                                                e++;
                                            }
                                            
                                            if(!found)
                                            {

												if(hasBones)
												{

													edgeDecal.bones[0][0] = currentFace.bones[indexA][0];
													edgeDecal.bones[1][0] = currentFace.bones[indexB][0];

													edgeDecal.bones[0][1] = currentFace.bones[indexA][1];
													edgeDecal.bones[1][1] = currentFace.bones[indexB][1];

													edgeDecal.boneMixes[0] = currentFace.boneMixes[indexA];
													edgeDecal.boneMixes[1] = currentFace.boneMixes[indexB];
												}

                                                vec3 edgeVertA = edgeVertABase - normalA * 0.001f;
                                                vec3 edgeVertB = edgeVertBBase - normalB * 0.001f;
                                                
                                                edgeDecal.shaderMaterial = material;
												edgeDecal.shaderMaterialB = materialB;
												edgeDecal.materialTypeIndex = materialManager->materialIDsByName[materialType];
                                                edgeDecal.partnerFound = false;
                                                
                                                edgeDecal.baseVerts[0] = edgeVertABase;
                                                edgeDecal.baseVerts[1] = edgeVertBBase;
                                                
                                                edgeDecal.normals[0] = normalA;
                                                edgeDecal.normals[1] = normalB;
                                                
                                                edgeDecal.verts[0] = edgeVertA;
                                                edgeDecal.verts[1] = edgeVertB;

												edgeDecal.posHashA = hashA;
												edgeDecal.posHashB = hashB;
                                                
                                              //  vec3 perpVec = normalize(cross(edgeVertBBase - edgeVertABase, currentFace.faceNormal));
                                                
                                                float sizeModifier = 2.0;//length(edgeVertBBase - edgeVertABase);
												edgeDecal.sizeModifier = sizeModifier;
                                                
                                                edgeDecal.verts[2] = edgeVertA + normalA * decalInfo.size.x * sizeModifier;// + perpVec * decalInfo.size.y * sizeModifier;
                                                edgeDecal.verts[3] = edgeVertB + normalB * decalInfo.size.x * sizeModifier;// + perpVec * decalInfo.size.y * sizeModifier;

												edgeDecal.faceNormal = normalize(cross(edgeDecal.verts[2] - edgeVertA, edgeVertB - edgeVertA));

												edgeDecal.texLocationIndex = 0;//texIndex % decalInfo.texLocations.size();
												texIndex++;
												dvec4& texLocation = decalInfo.texLocations[edgeDecal.texLocationIndex];
                                                
                                                edgeDecal.uvs[0] = vec2(texLocation.x, texLocation.y);
                                                edgeDecal.uvs[1] = vec2(texLocation.z, texLocation.y);
                                                edgeDecal.uvs[2] = vec2(texLocation.x, texLocation.w);
                                                edgeDecal.uvs[3] = vec2(texLocation.z, texLocation.w);
                                                
                                                edgeDecals.push_back(edgeDecal);
                                            }
                                        }
                                    }
                                    
                                    
                                    if(!primitive.morphTargets.empty())
                                    {
                                        for(const GLTFMorphTarget& morphTarget : primitive.morphTargets)
                                        {
                                            GLTFAccessor* mtPositionAccessor = &accessors[morphTarget.positionAccessor];
                                            GLTFAccessor* mtNormalAccessor = &accessors[morphTarget.normalAccessor];
                                            
                                            GLTFBufferView* mtPositionBufferView = &bufferViews[mtPositionAccessor->bufferView];
                                            GLTFBufferView* mtNormalBufferView = &bufferViews[mtNormalAccessor->bufferView];
                                            
                                            const char* mtPositionBuffer = buffers[mtPositionBufferView->buffer];
                                            const char* mtNormalBuffer = buffers[mtNormalBufferView->buffer];
                                            
                                            MorphTarget target;
                                            
                                            for(int vIndex = 0; vIndex < 3; vIndex++)
                                            {
                                                int indexBufferIndex = f * 3 + vIndex;
                                                size_t indexBufferLocation = indexBufferView->byteOffset + indexBufferIndex * indexAccessor->componentSize * indexAccessor->numberOfComponents;
                                                uint32_t index = getIntegerFromBuffer(indexAccessor->componentType, indexBuffer, indexBufferLocation);
                                                
                                                vec3 mtPosition;
                                                
                                                size_t mtPositionBufferLocationX = mtPositionBufferView->byteOffset + index * mtPositionAccessor->componentSize * mtPositionAccessor->numberOfComponents;
                                                size_t mtPositionBufferLocationY = mtPositionBufferLocationX + mtPositionAccessor->componentSize;
                                                size_t mtPositionBufferLocationZ = mtPositionBufferLocationY + mtPositionAccessor->componentSize;
                                                
                                                mtPosition.x = *((float*)&(mtPositionBuffer[mtPositionBufferLocationX]));
                                                mtPosition.y = *((float*)&(mtPositionBuffer[mtPositionBufferLocationY]));
                                                mtPosition.z = *((float*)&(mtPositionBuffer[mtPositionBufferLocationZ]));
                                                
                                                target.verts[2 - vIndex] = mtPosition;
                                                
                                                vec3 mtNormal;
                                                
                                                size_t mtNormalBufferLocationX = mtNormalBufferView->byteOffset + index * mtNormalAccessor->componentSize * mtNormalAccessor->numberOfComponents;
                                                size_t mtNormalBufferLocationY = mtNormalBufferLocationX + mtNormalAccessor->componentSize;
                                                size_t mtNormalBufferLocationZ = mtNormalBufferLocationY + mtNormalAccessor->componentSize;
                                                
                                                mtNormal.x = *((float*)&(mtNormalBuffer[mtNormalBufferLocationX]));
                                                mtNormal.y = *((float*)&(mtNormalBuffer[mtNormalBufferLocationY]));
                                                mtNormal.z = *((float*)&(mtNormalBuffer[mtNormalBufferLocationZ]));
                                                
                                                target.normals[2 - vIndex] = mtNormal;
                                            }
                                            
                                            
                                            currentFace.morphTargets.push_back(target);
                                            
                                        }
                                    }
                                    
									if(!discardFaceDueToEdgeDecalInfo)
									{
										if(isDecalMaterial)
										{
											decalFaces.push_back(currentFace);
										}
										else if(isShadowVolume)
										{
											shadowVolumeFaces.push_back(currentFace);
										}
										else
										{
											faces.push_back(currentFace);
										}
									}

									if(!isShadowVolume && !isDecalMaterial)
									{
										allFaces.push_back(currentFace); //allFaces is used for main thread geometry
									}
                                    
                                }
                                
                            }
                            
                            
                        }
                    }
                    
                    
                    
                    if(pathIndex == 0 && hasBones)
                    {
                        if(document.HasMember("animations") && document["animations"].Size() > 0)
                        {
                            const rapidjson::Value& animation = document["animations"][0];
                            
                            const rapidjson::Value& channelsIn = animation["channels"];
							//MJLog("channelsIn.Size():%d", channelsIn.Size())
                            for (rapidjson::SizeType i = 0; i < channelsIn.Size(); i++)
                            {
                                int samplerIndex = channelsIn[i]["sampler"].GetInt();
                                const rapidjson::Value& target = channelsIn[i]["target"];
                                int targetNodeIndex = target["node"].GetInt();
                                std::string transformType = target["path"].GetString();
                                
                                const rapidjson::Value& sampler = animation["samplers"][samplerIndex];
                                
                                int timeAccessorIndex = sampler["input"].GetInt();
                                int valueAccessorIndex = sampler["output"].GetInt();
                                
                                GLTFAccessor* timeAccessor = &accessors[timeAccessorIndex];
                                GLTFBufferView* timeBufferView = &bufferViews[timeAccessor->bufferView];
                                const char* timeBuffer = buffers[timeBufferView->buffer];
                                
                                GLTFAccessor* valueAccessor = &accessors[valueAccessorIndex];
                                GLTFBufferView* valueBufferView = &bufferViews[valueAccessor->bufferView];
                                const char* valueBuffer = buffers[valueBufferView->buffer];
                                
                                if(timeAccessor->count != valueAccessor->count)
                                {
                                    MJLog("ERROR:timeAccessor->count != valueAccessor->count in %s. This could mean there is a gap between key frames, or that it is using beizer interpolation. Only linear or constant interpolation is supported.", pathname.c_str());
                                }
								else
								{
									//MJLog("timeAccessor->count:%d", timeAccessor->count)
									for(int j = 0; j < timeAccessor->count; j++)
									{
										size_t timeBufferLocation = timeBufferView->byteOffset + j * timeAccessor->componentSize * timeAccessor->numberOfComponents;
										float timeValue = *((float*)&(timeBuffer[timeBufferLocation]));
										int timeValueInt = MAX(round(timeValue) - 1, 0); //er I dunno. Dodgy maybe...

										//MJLog("timeValue:%.2f timeValueInt:%d", timeValue, timeAccessor->count)
                                    
										if(keyframes.count(timeValueInt) == 0)
										{
											keyframes[timeValueInt] = defaultSkeleton;
										}
										ModelSkeleton* skeleton = &(keyframes[timeValueInt]);
										setupBoneIndexMapForSkeleton(skeleton);
                                    
										ModelBone* bone = skeleton->bonesByIndex[nodeIndicesToBoneIndicesMap[targetNodeIndex]];
										if(!bone)
										{
											MJLog("could not find bone with node index %d. Sapiens requires that all bones are in a heirachy with a single root/body bone at the top level.", targetNodeIndex);
										}
										else
										{
											if(transformType == "translation")
											{
												vec3 translation;
                                            
												size_t valueBufferLocationX = valueBufferView->byteOffset + j * valueAccessor->componentSize * valueAccessor->numberOfComponents;
												size_t valueBufferLocationY = valueBufferLocationX + valueAccessor->componentSize;
												size_t valueBufferLocationZ = valueBufferLocationY + valueAccessor->componentSize;
                                            
												translation.x = *((float*)&(valueBuffer[valueBufferLocationX]));
												translation.y = *((float*)&(valueBuffer[valueBufferLocationY]));
												translation.z = *((float*)&(valueBuffer[valueBufferLocationZ]));
                                            
												bone->translation = translation;
											}
											else if(transformType == "rotation")
											{
												if(valueAccessor->componentType == 5126)
												{
													quat rotation;
                                                
													size_t valueBufferLocationX = valueBufferView->byteOffset + j * valueAccessor->componentSize * valueAccessor->numberOfComponents;
													size_t valueBufferLocationY = valueBufferLocationX + valueAccessor->componentSize;
													size_t valueBufferLocationZ = valueBufferLocationY + valueAccessor->componentSize;
													size_t valueBufferLocationW = valueBufferLocationZ + valueAccessor->componentSize;
                                                
													rotation.x = *((float*)&(valueBuffer[valueBufferLocationX]));
													rotation.y = *((float*)&(valueBuffer[valueBufferLocationY]));
													rotation.z = *((float*)&(valueBuffer[valueBufferLocationZ]));
													rotation.w = *((float*)&(valueBuffer[valueBufferLocationW]));
                                                
													bone->rotation = rotation;
												}
												else
												{
													MJLog("unsupported rotation type:%d", valueAccessor->componentType);
												}
											}
										}
                                    
										//size_t timeBufferLocation = timeBufferView->byteOffset + j * timeAccessor->componentSize * timeAccessor->numberOfComponents;
										// float timeValue = *((float*)&(timeBuffer[timeBufferLocation]));
                                    
									}
								}
                            }
                        }
                    }
                }
                catch (const std::runtime_error& ex)
                {
                    MJLog("failed to load GLTF file, probably due to it missing some expected data. Did you remember to assign materials? %s - %s\n", pathname.c_str(), ex.what());
                }
            }
        }
        pathIndex++;
    }

	std::vector<ModelEdgeDecal> edgesCopy = edgeDecals;
	edgeDecals.clear();

	int index = 0;
	for(ModelEdgeDecal edgeDecal : edgesCopy)
	{
		if(badEdges.count(index) == 0 && edgeDecal.partnerFound)
		{
            EdgeDecalInfo decalInfo = materialManager->getEdgeDecalInfo(materialManager->materialNamesByID[edgeDecal.materialTypeIndex]);
			if(!decalInfo.discardFaces)
			{
				edgeDecals.push_back(edgeDecal);
			}

			if(decalInfo.texLocations.size() > 1)
			{
				vec3 perpVec = normalize(cross(edgeDecal.baseVerts[1] - edgeDecal.baseVerts[0], edgeDecal.verts[2] - edgeDecal.baseVerts[0]));

				vec3 perpVecA0 = normalize(perpVec + edgeDecal.normals[0] * 0.2f); //todo make this configurable
				vec3 perpVecA1 = normalize(perpVec + edgeDecal.normals[1] * 0.2f);

				vec3 perpVecB0 = normalize(-perpVec + edgeDecal.normals[0] * 0.2f);
				vec3 perpVecB1 = normalize(-perpVec + edgeDecal.normals[1] * 0.2f);


				int texLocationIndexA = (edgeDecal.texLocationIndex + 1) % decalInfo.texLocations.size();
				dvec4& texLocationA = decalInfo.texLocations[texLocationIndexA];

				edgeDecal.uvs[0] = vec2(texLocationA.x, texLocationA.y + (texLocationA.w - texLocationA.y) * 0.5);
				edgeDecal.uvs[1] = vec2(texLocationA.z, texLocationA.y + (texLocationA.w - texLocationA.y) * 0.5);
				edgeDecal.uvs[2] = vec2(texLocationA.x, texLocationA.y);
				edgeDecal.uvs[3] = vec2(texLocationA.z, texLocationA.y);

				edgeDecal.verts[0] = edgeDecal.baseVerts[0] + edgeDecal.normals[0] * decalInfo.size.y * decalInfo.size.x * edgeDecal.sizeModifier;
				edgeDecal.verts[1] = edgeDecal.baseVerts[1] + edgeDecal.normals[1] * decalInfo.size.y * decalInfo.size.x * edgeDecal.sizeModifier;

				edgeDecal.verts[2] = edgeDecal.baseVerts[0] + edgeDecal.normals[0] * decalInfo.size.y * decalInfo.size.x * edgeDecal.sizeModifier + perpVecA0 * decalInfo.size.x * edgeDecal.sizeModifier;
				edgeDecal.verts[3] = edgeDecal.baseVerts[1] + edgeDecal.normals[1] * decalInfo.size.y * decalInfo.size.x * edgeDecal.sizeModifier + perpVecA1 * decalInfo.size.x * edgeDecal.sizeModifier;
				edgeDecal.faceNormal = normalize(cross(edgeDecal.verts[2] - edgeDecal.baseVerts[0], edgeDecal.baseVerts[1] - edgeDecal.baseVerts[0]));

				edgeDecals.push_back(edgeDecal);

				edgeDecal.uvs[0] = vec2(texLocationA.x, texLocationA.y + (texLocationA.w - texLocationA.y) * 0.5);
				edgeDecal.uvs[1] = vec2(texLocationA.z, texLocationA.y + (texLocationA.w - texLocationA.y) * 0.5);
				edgeDecal.uvs[2] = vec2(texLocationA.x, texLocationA.w);
				edgeDecal.uvs[3] = vec2(texLocationA.z, texLocationA.w);

				edgeDecal.verts[2] = edgeDecal.baseVerts[0] + edgeDecal.normals[0] * decalInfo.size.y * decalInfo.size.x * edgeDecal.sizeModifier + perpVecB0 * decalInfo.size.x * edgeDecal.sizeModifier;
				edgeDecal.verts[3] = edgeDecal.baseVerts[1] + edgeDecal.normals[1] * decalInfo.size.y * decalInfo.size.x * edgeDecal.sizeModifier + perpVecB1 * decalInfo.size.x * edgeDecal.sizeModifier;
				edgeDecal.faceNormal = normalize(cross(edgeDecal.verts[2] - edgeDecal.baseVerts[0], edgeDecal.baseVerts[1] - edgeDecal.baseVerts[0]));

				edgeDecals.push_back(edgeDecal);
			}

		}
		index++;
	}

	//MJLog("model:%s edgeDecals:%d", debugName.c_str(), edgeDecals.size());
    
	//MJLog("%s - max:(%.2f,%.2f,%.2f) min:(%.2f,%.2f,%.2f)", debugName.c_str(), max.x, max.y, max.z, min.x, min.y, min.z);
    
    center = min + (max - min) * 0.5f;


	if(hasDynamicBox && staticBoxes.empty() && staticSpheres.empty())
	{
		staticBoxes.push_back(dynamicBox);
	}
	if(hasDynamicSphere && staticBoxes.empty() && staticSpheres.empty())
	{
		staticSpheres.push_back(dynamicSphere);
	}

	if(pathColliderBoxes.empty())
	{
		pathColliderBoxes = staticBoxes;
	}
	if(pathColliderSpheres.empty())
	{
		pathColliderSpheres = staticSpheres;
	}
}

Model::~Model()
{
    
}
