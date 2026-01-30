//
//  ClientModel.h
//  Ambience
//
//  Created by David Frampton on 20/08/18.
//Copyright © 2018 Majic Jungle. All rights reserved.
//

#ifndef ClientModel_h
#define ClientModel_h

#include "Model.h"

#include "Vulkan.h"

VULKAN_PACK(
     struct ModelBoneUniforms{
         float boneRotations[MAX_BONES][4];
         float boneTranslations[MAX_BONES][4];
     });

struct ModelBoneMatrices {
	mat4 boneMatrices[MAX_BONES];
};

struct ModelBoneOverride {
	quat rotationGoal;
	quat interpolatedRotationGoal = quat(1,0,0,0);
	bool initialized = false;
	bool additive = false;
	float randomRoll = 0.0f;
	float rate = 0.0f;
	float goalRate = 0.0f;
	bool transitionTowardRemoval = false;
	float removalTransitionFraction = 1.0f;
};

struct BoneLimits {
	bool hasLimits[2] = {false, false};
	vec2 limits[2];
	float rate = 1.0f;
	float randomRoll = 0.0f;
	uint32_t randomSeedCounter = 0;
};


struct KeyframeComposite {
	std::vector<std::string> boneNames;
	std::set<int> boneIndexes;
	int frameIndex;
};

struct Keyframe {
	int index;
	double speed;
	double duration;
	//MJLuaRef* trigger = nullptr;
	double easeIn = 1.0;
	double easeOut= 1.0;
	double randomVariance = 0.0;
	std::vector<KeyframeComposite> composites;
};


struct AnimatingObject {
	int groupIndex;
	int animationTypeIndex[3] = {-1,-1,-1};
	int animationFrameIndex[2] = {0,0};
	bool isInterpolatingAnimations = false;
	double intraAnimationInterpolationFraction = 0.0;
	double interpolationFractions[2] = {0.0,0.0};
	double speedMultipler = 1.0;

	int randomDurationMultiplierIndex[2] = {0,1};
	int lowResSkipCount = 0;

	Keyframe* keyframes[2][4] = {{nullptr,nullptr,nullptr,nullptr},{nullptr,nullptr,nullptr,nullptr}};

	ModelBoneMatrices matrices;
	std::map<int, ModelBoneOverride> overrides;
};

void loadInterpolatedBoneUniformsForSkeletons(ModelBoneUniforms* uniforms,
	mat3 objectRotation,
	ModelBoneMatrices* matrices,
	std::vector<mat4>& inverseBindMatrices,
	std::vector<vec3>& inverseBindMatrixOffsets,
	std::map<int, ModelBoneOverride>& overrides,
	std::map<int, BoneLimits>& limits,
	ModelSkeleton& skeletonAA,
	ModelSkeleton& skeletonA,
	ModelSkeleton& skeletonB,
	ModelSkeleton& skeletonBB,
	double scale,
	float lerp,
	float duration,
	float dt,
	bool lowRes = false);

void load2DInterpolatedBoneUniformsForSkeletons(ModelBoneUniforms* uniforms,
	mat3 objectRotation,
	ModelBoneMatrices* matrices,
	std::vector<mat4>& inverseBindMatrices,
	std::vector<vec3>& inverseBindMatrixOffsets,
	std::map<int, ModelBoneOverride>& overrides,
	std::map<int, BoneLimits>& limits,
	double scale,
	float yLerp,

	ModelSkeleton& skeletonAA1,
	ModelSkeleton& skeletonA1,
	ModelSkeleton& skeletonB1,
	ModelSkeleton& skeletonBB1,
	float xLerp1,
	float duration1,

	ModelSkeleton& skeletonAA2,
	ModelSkeleton& skeletonA2,
	ModelSkeleton& skeletonB2,
	ModelSkeleton& skeletonBB2,
	float xLerp2,
	float duration2,
	float dt);

void loadInterpolatedBoneUniformsForAnimatingObject(ModelBoneUniforms* uniforms,
	mat3 objectRotation,
	Model* model,
	AnimatingObject& animatingObject,
	std::map<int, BoneLimits>& limits,
	double scale,
	float dt,
	bool lowRes);

MJVMABuffer createModelBuffer(Vulkan* vulkan, VkCommandBuffer commandBuffer, Model* model);
MJVMABuffer createModelShadowBuffer(Vulkan* vulkan, VkCommandBuffer commandBuffer, Model* model);
MJVMABuffer createModelEdgeBuffer(Vulkan* vulkan, VkCommandBuffer commandBuffer, Model* model);

/*class ClientModel : public Model {

public: // members
    
protected: // members

public: // functions
    ClientModel();
    ~ClientModel();

protected: // functions

};*/

#endif /* ClientModel_h */
