
#ifndef Model_h
#define Model_h

#include <stdio.h>
#include <map>
#include "MathUtils.h"

#define MAX_BONES 16

class MaterialManager;

struct MorphTarget {
    vec3 verts[3];
    vec3 normals[3];
};

struct ModelFace {
    vec3 verts[3];
    vec3 normals[3];
	vec3 tangents[3];
    vec3 faceNormal;
    vec2 uvs[3];
    uint8_t bones[3][2];
	uint8_t boneMixes[3];
    u8vec4 shaderMaterial;
	u8vec4 shaderMaterialB;
	uint32_t materialTypeIndex;
    std::vector<MorphTarget> morphTargets;
};

struct ModelEdgeDecal {
    vec3 baseVerts[2];
    vec3 verts[4];
    vec3 normals[2];
    vec2 uvs[4];
	vec3 faceNormal;
	u8vec4 shaderMaterial;
	u8vec4 shaderMaterialB;
	uint8_t bones[3][2];
	uint8_t boneMixes[3];
	uint64_t posHashA;
	uint64_t posHashB;
	int materialTypeIndex;
	int texLocationIndex;
    bool partnerFound;
	float sizeModifier;
};

struct ModelBone {
    std::vector<ModelBone> children;
    int boneIndex;
    quat rotation;
    vec3 translation;
};

struct ModelSkeleton {
    std::vector<ModelBone> rootBones;
	std::map<int, ModelBone*> bonesByIndex;
};

struct ModelPlaceHolder {
    int boneIndex = 0;
    quat rotation;
    vec3 translation;
    vec3 scale;
    float boneLengthOffset;
};

struct ModelSphere {
	vec3 pos;
	float radius;
};

struct ModelBox {
	vec3 pos;
	dmat3 rotation;
	dmat3 rotationInverse; //only set for dynamicBox at the moment
	vec3 halfSize;
};

struct ModelCone {
	vec3 pos;
	quat rotation;
	float radius;
	float height;
};

class Model {
public:
    int index = 0;
    
    std::string modelPath;
    std::vector<ModelFace> faces;
	std::vector<ModelFace> shadowVolumeFaces;
    std::vector<ModelEdgeDecal> edgeDecals;
	std::vector<ModelFace> decalFaces;
	std::vector<ModelFace> allFaces;
    vec3 max;
    vec3 min;
    vec3 center;
    bool hasBones;
    bool hasVolume;
    bool hasUVs;
    int morphTargetCount;
    float windStrengthMain;
	float windStrengthDecals;
    
    ModelSkeleton defaultSkeleton;
    std::map<int, ModelSkeleton> keyframes;
	std::vector<mat4> inverseBindMatrices;
	std::vector<vec3> inverseBindMatrixOffsets;
    
    std::map<std::string, ModelPlaceHolder> modelPlaceHolders;
    std::map<std::string, int> bonesIndicesByName;

	bool hasBoundingSphere = false;
	ModelSphere boundingSphere;

	bool hasDynamicSphere = false;
	ModelSphere dynamicSphere;

	bool hasDynamicBox = false;
	ModelBox dynamicBox;

	std::vector<ModelSphere> staticSpheres;
	std::vector<ModelBox> staticBoxes;
	std::vector<ModelCone> staticCones;


	std::vector<ModelBox> pathNodeBoxes;
	std::vector<ModelBox> pathColliderBoxes;
	std::vector<ModelSphere> pathColliderSpheres;
	std::vector<ModelBox> pathWalkNodeBoxes;

	/*std::vector<ModelBox> placeBoxes;
	std::vector<ModelBox> buildClearBoxes;*/


	std::map<std::string, std::vector<ModelSphere>> customSpheres;
	std::map<std::string, std::vector<ModelBox>> customBoxes;
	std::map<std::string, std::vector<ModelCone>> customCones;


	std::string debugName;
    
public:
    Model(const std::string& modelPath_,
          MaterialManager* materialManager);

    ~Model();
    
private:
    
};

#endif /* Model_hpp */
