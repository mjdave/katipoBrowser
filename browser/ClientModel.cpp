//
//  ClientModel.cpp
//  Ambience
//
//  Created by David Frampton on 20/08/18.
//Copyright © 2018 Majic Jungle. All rights reserved.
//

#include "ClientModel.h"

//#include "ClientGameObject.h"
#include "ObjectVert.h"
#include "gtx/euler_angles.hpp"

/*
void extractEulerAngleXYZ(mat3& mat, float& rotXangle, float& rotYangle, float& rotZangle) 
{
	rotXangle = atan2(-mat[1].z, mat[2].z);
	float cosYangle = sqrt(powf(mat[0].x, 2.0f) + powf(mat[0].y, 2.0f));
	rotYangle = atan2(mat[0].z, cosYangle);
	float sinXangle = sin(rotXangle);
	float cosXangle = cos(rotXangle);
	rotZangle = atan2(cosXangle * mat[1].x + sinXangle * mat[2].x, cosXangle * mat[1].y + sinXangle * mat[2].y);
}*/

void loadInterpolatedBoneUniformsRecursivelyForBones(ModelBoneUniforms* uniforms,
	mat3 objectRotation,
	ModelBoneMatrices* matrices,
	std::vector<mat4>& inverseBindMatrices,
	std::vector<vec3>& inverseBindMatrixOffsets,
	std::map<int, ModelBoneOverride>& overrides,
	std::map<int, BoneLimits>& limits,
	ModelBone& boneAA,
	ModelBone& boneA,
	ModelBone& boneB,
	ModelBone& boneBB,
	mat4& parentMatrix,
	double scale,
	float lerp,
	float duration,
	float dt,
	bool lowRes)
{
    
    vec3 incomingTranslation = daveBoneTranslationLerp(boneAA.translation, boneA.translation, boneB.translation, boneBB.translation, lerp);
    
    // quat incomingRotation = normalize(catmullRom(boneAA.rotation, boneA.rotation, boneB.rotation, boneBB.rotation, lerp));
	quat incomingRotation;
	if(lowRes)
	{
		incomingRotation = animationSlerp(boneA.rotation, boneB.rotation, lerp);
	}
	else
	{
		incomingRotation = bezierNew(boneAA.rotation, boneA.rotation, boneB.rotation, boneBB.rotation, lerp);
	}

	if(overrides.count(boneA.boneIndex) != 0)
	{
		int boneIndex = boneA.boneIndex;
		bool removed = false;
		ModelBoneOverride& override = overrides[boneIndex];

		if(lowRes)
		{
			overrides.erase(boneIndex);
			removed = true;
		}
		else
		{
			if(override.transitionTowardRemoval)
			{
				override.rate = override.rate - override.rate * clamp(dt * 3.0f, 0.0f, 1.0f);
				override.removalTransitionFraction += override.rate * dt * 3.0;
				if(override.removalTransitionFraction >= 1.0)
				{
					overrides.erase(boneIndex);
					removed = true;
				}
			}
			else
			{
				override.rate = override.rate + (override.goalRate - override.rate) * clamp(dt * 3.0f, 0.0f, 1.0f);
				override.removalTransitionFraction -= override.rate * dt * 3.0;
				if(override.removalTransitionFraction <= 0.0)
				{
					override.removalTransitionFraction = 0.0;
				}
			}
		}

		if(!removed)
		{
			quat overrideGoalToUse = override.rotationGoal;
			mat3 combinedBoneRotation = objectRotation * mat3(parentMatrix);
			mat3 combinedBoneRotationInverse = inverse(combinedBoneRotation);
			quat interpolatedRotationGoal = overrideGoalToUse;
			quat overrideRotation = incomingRotation;

			if(!override.initialized)
			{
				if(override.additive)
				{
					override.interpolatedRotationGoal = mat3(1.0f);
				}
				else
				{
					override.interpolatedRotationGoal = mat3(incomingRotation) * combinedBoneRotation;
				}
				override.initialized = true;
			}

			if(limits.count(boneIndex) != 0)
			{
				BoneLimits& boneLimits = limits[boneIndex];

				mat3 localRotationGoal;
				if(override.additive)
				{
					localRotationGoal = mat3(overrideGoalToUse);
				}
				else
				{
					localRotationGoal = combinedBoneRotationInverse * mat3(overrideGoalToUse);
				}

				if(localRotationGoal[2][1] < 1.0 && localRotationGoal[2][1] > -1.0 )
				{    
					vec2 angles = vec2(0.0,0.0);
					angles.x = -asin(localRotationGoal[2][1]);

					for(int a = 0; a < 2; a++)
					{
						if(boneLimits.hasLimits[a])
						{
							angles[a] = clamp(angles[a], boneLimits.limits[a].x, boneLimits.limits[a].y);
						}
					}

					quat rotationQuat = normalize(quat(mat3(eulerAngleYXZ(angles.y, angles.x, 0.0f))));

					if(!approxEqual(boneLimits.randomRoll, 0.0f))
					{
						if(approxEqual(override.randomRoll, 0.0f))
						{
                            uint64_t randomID = 9223468923;
							override.randomRoll = (randomValueForUniqueIDAndSeed(randomID, boneLimits.randomSeedCounter++) - 0.5f) * 2.0f * boneLimits.randomRoll;
						}
						rotationQuat = rotate(rotationQuat, override.randomRoll, vec3(0.0f,0.0f,1.0f));
					}

					localRotationGoal = mat3(rotationQuat);

					if(override.additive)
					{
						overrideGoalToUse = localRotationGoal;
					}
					else
					{
						overrideGoalToUse = combinedBoneRotation * localRotationGoal;
					}
				}
				else
				{
					if(override.additive)
					{
						overrideGoalToUse = mat3(1.0);
					}
					else
					{
						overrideGoalToUse = combinedBoneRotation * mat3(incomingRotation);
					}
				}

				interpolatedRotationGoal = normalize(animationSlerp(override.interpolatedRotationGoal, overrideGoalToUse, clamp(boneLimits.rate * override.rate * dt, 0.0f, 1.0f)));

				quat naturalRotation = quat(1,0,0,0);
				if(!override.additive)
				{
					naturalRotation = mat3(incomingRotation) * combinedBoneRotation;
				}

				if(dot(interpolatedRotationGoal, naturalRotation) < 0.5)
				{
					interpolatedRotationGoal = overrideGoalToUse;

					if(dot(interpolatedRotationGoal, naturalRotation) < 0.5)
					{
						interpolatedRotationGoal = naturalRotation;
					}
				}

				if(override.additive)
				{
					overrideRotation = mat3(incomingRotation * interpolatedRotationGoal);
				}
				else
				{
					overrideRotation =  combinedBoneRotationInverse * mat3(interpolatedRotationGoal);
				}
			}
			else
			{
				interpolatedRotationGoal = normalize(animationSlerp(override.interpolatedRotationGoal, overrideGoalToUse, clamp(8.0f * override.rate * dt * 1.0f, 0.0f, 1.0f)));

				if(override.additive)
				{
					overrideRotation = mat3(incomingRotation * interpolatedRotationGoal);
				}
				else
				{
					overrideRotation =  combinedBoneRotationInverse * mat3(interpolatedRotationGoal);
				}
			}

			override.interpolatedRotationGoal = interpolatedRotationGoal;

			if(override.removalTransitionFraction > 0.00001)
			{
				incomingRotation = normalize(animationSlerp(overrideRotation, incomingRotation, override.removalTransitionFraction));
			}
			else
			{
				incomingRotation = overrideRotation;
			}
		}

	}
    
    mat4 matrix = translate(parentMatrix, incomingTranslation * (float)scale);
    
    matrix = matrix * toMat4(incomingRotation);

	matrices->boneMatrices[boneA.boneIndex] = matrix;



	mat4& inverseBindMatrix = inverseBindMatrices[boneA.boneIndex];

	mat3 inverseMat =  inverse(mat3(matrix) * mat3(inverseBindMatrix));
	vec3 translation = inverseMat * vec3(matrix[3]) + inverseBindMatrixOffsets[boneA.boneIndex] * (float)scale;

	quat rotation = inverseMat;
    
    memcpy(&(uniforms->boneRotations[boneA.boneIndex][0]), value_ptr(rotation), sizeof(float) * 4);
    memcpy(&(uniforms->boneTranslations[boneA.boneIndex][0]), value_ptr(translation), sizeof(float) * 3);
    
    for(int i = 0; i < boneA.children.size(); i++)
    {
		loadInterpolatedBoneUniformsRecursivelyForBones(uniforms,
			objectRotation,
			matrices,
			inverseBindMatrices,
			inverseBindMatrixOffsets,
			overrides,
			limits,
			boneAA.children[i],
			boneA.children[i],
			boneB.children[i],
			boneBB.children[i],
			matrix,
			scale,
			lerp,
			duration,
			dt,
			lowRes);
    }
}

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
	bool lowRes)
{
	mat4 matrix = mat4(1.0f);
	for (int i = 0; i < skeletonA.rootBones.size(); i++)
	{
		loadInterpolatedBoneUniformsRecursivelyForBones(uniforms,
			objectRotation,
			matrices,
			inverseBindMatrices,
			inverseBindMatrixOffsets,
			overrides,
			limits,
			skeletonAA.rootBones[i],
			skeletonA.rootBones[i],
			skeletonB.rootBones[i],
			skeletonBB.rootBones[i],
			matrix,
			scale,
			lerp,
			duration,
			dt,
			lowRes);
	}
}


void loadInterpolatedBoneUniformsRecursively(ModelBoneUniforms* uniforms,
	mat3 objectRotation,
	Model* model,
	AnimatingObject& animatingObject,
	std::map<int, BoneLimits>& limits,
	double scale,
	float dt,
	bool lowRes,
	ModelBone* defaultSkeletonBone,
	bool bilerp,
	float yLerp,
	float lerps[2],
	float durations[2],
	mat4& parentMatrix,
	ModelSkeleton* skeletons[2][4])
{
	int boneIndex = defaultSkeletonBone->boneIndex;
	vec3 incomingTranslation;
	quat incomingRotation;

	if(bilerp)
	{
		ModelBone* bones[2][4];
		for(int animationIndex = 0; animationIndex < 2; animationIndex++)
		{
			for(int i = 0; i < 4; i++)
			{

				bool compositeOverrideFound = false;
				for(KeyframeComposite& composite : animatingObject.keyframes[animationIndex][i]->composites)
				{
					if(composite.boneIndexes.count(boneIndex) != 0)
					{
						ModelSkeleton& skelton = model->keyframes[composite.frameIndex];
						bones[animationIndex][i] = skelton.bonesByIndex[boneIndex];
						compositeOverrideFound =  true;
					}
				}

				if(!compositeOverrideFound)
				{
					bones[animationIndex][i] = skeletons[animationIndex][i]->bonesByIndex[boneIndex];
				}
			}
		}

		vec3 incomingTranslation1 = daveBoneTranslationLerp(bones[0][0]->translation, bones[0][1]->translation, bones[0][2]->translation, bones[0][3]->translation, lerps[0]);
		vec3 incomingTranslation2 = daveBoneTranslationLerp(bones[1][0]->translation, bones[1][1]->translation, bones[1][2]->translation, bones[1][3]->translation, lerps[1]);

		incomingTranslation = mix(incomingTranslation1, incomingTranslation2, yLerp);

		quat incomingRotation1 = bezierNew(bones[0][0]->rotation, bones[0][1]->rotation, bones[0][2]->rotation, bones[0][3]->rotation, lerps[0]);
		quat incomingRotation2 = bezierNew(bones[1][0]->rotation, bones[1][1]->rotation, bones[1][2]->rotation, bones[1][3]->rotation, lerps[1]);

		incomingRotation = normalize(animationSlerp(incomingRotation1, incomingRotation2, yLerp));
	}
	else
	{
		ModelBone* bones[4];
		for(int i = 0; i < 4; i++)
		{
			bool compositeOverrideFound = false;
			for(KeyframeComposite& composite : animatingObject.keyframes[0][i]->composites)
			{
				if(composite.boneIndexes.count(boneIndex) != 0)
				{
					ModelSkeleton& skelton = model->keyframes[composite.frameIndex];
					bones[i] = skelton.bonesByIndex[boneIndex];
					compositeOverrideFound =  true;
				}
			}
			if(!compositeOverrideFound)
			{
				bones[i] = skeletons[0][i]->bonesByIndex[boneIndex];
			}
		}

		incomingTranslation = daveBoneTranslationLerp(bones[0]->translation, bones[1]->translation, bones[2]->translation, bones[3]->translation, lerps[0]);

		if(lowRes)
		{
			incomingRotation = animationSlerp(bones[1]->rotation, bones[2]->rotation, lerps[0]);
		}
		else
		{
			incomingRotation = bezierNew(bones[0]->rotation, bones[1]->rotation, bones[2]->rotation, bones[3]->rotation, lerps[0]);
		}
	}

	if(animatingObject.overrides.count(boneIndex) != 0)
	{
		bool removed = false;
		ModelBoneOverride& override = animatingObject.overrides[boneIndex];
		if(lowRes)
		{
			animatingObject.overrides.erase(boneIndex);
			removed = true;
		}
		else
		{
			if(override.transitionTowardRemoval)
			{
				override.rate = override.rate - override.rate * clamp(dt * 2.0f, 0.0f, 1.0f);
				override.removalTransitionFraction += override.rate * dt * 3.0;
				if(override.removalTransitionFraction >= 1.0)
				{
					animatingObject.overrides.erase(boneIndex);
					removed = true;
				}
			}
			else
			{
				override.rate = override.rate + (override.goalRate - override.rate) * clamp(dt * 1.0f * override.goalRate, 0.0f, 1.0f);
				override.removalTransitionFraction -= override.rate * dt * 3.0;
				if(override.removalTransitionFraction <= 0.0)
				{
					override.removalTransitionFraction = 0.0;
				}
			}
		}

		if(!removed)
		{
			quat overrideGoalToUse = override.rotationGoal;
			mat3 combinedBoneRotation = objectRotation * mat3(parentMatrix);
			mat3 combinedBoneRotationInverse = inverse(combinedBoneRotation);
			quat interpolatedRotationGoal = overrideGoalToUse;
			quat overrideRotation = incomingRotation;

			if(!override.initialized)
			{
				if(override.additive)
				{
					override.interpolatedRotationGoal = mat3(1.0f);
				}
				else
				{
					override.interpolatedRotationGoal = mat3(incomingRotation) * combinedBoneRotation;
				}
				override.initialized = true;
			}

			if(limits.count(boneIndex) != 0)
			{
				BoneLimits& boneLimits = limits[boneIndex];

				mat3 localRotationGoal; 
				if(override.additive)
				{
					localRotationGoal = mat3(overrideGoalToUse);
				}
				else
				{
					localRotationGoal = combinedBoneRotationInverse * mat3(overrideGoalToUse);
				}

				if(localRotationGoal[2][1] < 0.99 && localRotationGoal[2][1] > -0.99 )
				{    
					vec2 angles = vec2(0.0,0.0);
					angles.x = -asin(localRotationGoal[2][1]);
					float cosYangle = sqrt(powf(localRotationGoal[0].x, 2.0f) + powf(localRotationGoal[0].y, 2.0f));
					angles.y = -atan2(localRotationGoal[0].z, cosYangle) * (1.0f - powf(fabsf(localRotationGoal[2][1]), 4.0f));

					for(int a = 0; a < 2; a++)
					{
						if(boneLimits.hasLimits[a])
						{
							angles[a] = clamp(angles[a], boneLimits.limits[a].x, boneLimits.limits[a].y);
						}
					}

					quat rotationQuat = normalize(quat(mat3(eulerAngleYXZ(angles.y, angles.x, 0.0f))));

					if(!approxEqual(boneLimits.randomRoll, 0.0f))
					{
						if(approxEqual(override.randomRoll, 0.0f))
						{
                            uint64_t randomID = 9223468923;
							override.randomRoll = (randomValueForUniqueIDAndSeed(randomID, boneLimits.randomSeedCounter++) - 0.5f) * 2.0f * boneLimits.randomRoll;
						}
						rotationQuat = rotate(rotationQuat, override.randomRoll, vec3(0.0f,0.0f,1.0f));
					}

					localRotationGoal = mat3(rotationQuat);

					if(override.additive)
					{
						overrideGoalToUse = localRotationGoal;
					}
					else
					{
						overrideGoalToUse = combinedBoneRotation * localRotationGoal;
					}
				}
				else
				{
					if(override.additive)
					{
						overrideGoalToUse = mat3(1.0);
					}
					else
					{
						overrideGoalToUse = combinedBoneRotation * mat3(incomingRotation);
					}
				}

				interpolatedRotationGoal = normalize(animationSlerp(override.interpolatedRotationGoal, overrideGoalToUse, clamp(boneLimits.rate * override.rate * dt, 0.0f, 1.0f)));


				quat naturalRotation = quat(1,0,0,0);
				if(!override.additive)
				{
					naturalRotation = mat3(incomingRotation) * combinedBoneRotation;
				}

				if(dot(interpolatedRotationGoal, naturalRotation) < 0.5)
				{
					interpolatedRotationGoal = overrideGoalToUse;

					if(dot(interpolatedRotationGoal, naturalRotation) < 0.5)
					{
						interpolatedRotationGoal = naturalRotation;
					}
				}

				if(override.additive)
				{
					overrideRotation = mat3(incomingRotation * interpolatedRotationGoal);
				}
				else
				{
					overrideRotation =  combinedBoneRotationInverse * mat3(interpolatedRotationGoal);
				}
			}
			else
			{
				interpolatedRotationGoal = normalize(animationSlerp(override.interpolatedRotationGoal, overrideGoalToUse, clamp(8.0f * override.rate * dt * 1.0f, 0.0f, 1.0f)));

				if(override.additive)
				{
					overrideRotation = mat3(incomingRotation * interpolatedRotationGoal);
				}
				else
				{
					overrideRotation =  combinedBoneRotationInverse * mat3(interpolatedRotationGoal);
				}
			}

			override.interpolatedRotationGoal = interpolatedRotationGoal;

			if(override.removalTransitionFraction > 0.00001)
			{
				incomingRotation = normalize(animationSlerp(overrideRotation, incomingRotation, override.removalTransitionFraction));
			}
			else
			{
				incomingRotation = overrideRotation;
			}
		}

	}

	mat4 matrix = translate(parentMatrix, incomingTranslation * (float)scale);

	matrix = matrix * toMat4(incomingRotation);

	animatingObject.matrices.boneMatrices[boneIndex] = matrix;

	mat4& inverseBindMatrix = model->inverseBindMatrices[boneIndex];

	mat3 inverseMat =  inverse(mat3(matrix) * mat3(inverseBindMatrix));
	vec3 translation = inverseMat * vec3(matrix[3]) + model->inverseBindMatrixOffsets[boneIndex] * (float)scale;

	quat rotation = inverseMat;

	memcpy(&(uniforms->boneRotations[boneIndex][0]), value_ptr(normalize(rotation)), sizeof(float) * 4);
	memcpy(&(uniforms->boneTranslations[boneIndex][0]), value_ptr(translation), sizeof(float) * 3);

	for(int i = 0; i < defaultSkeletonBone->children.size(); i++)
	{
		loadInterpolatedBoneUniformsRecursively(uniforms,
			objectRotation,
			model,
			animatingObject,
			limits,
			scale,
			dt,
			lowRes,
			&(defaultSkeletonBone->children[i]),
			bilerp,
			yLerp,
			lerps,
			durations,
			matrix,
			skeletons);
	}
}

void loadInterpolatedBoneUniformsForAnimatingObject(ModelBoneUniforms* uniforms,
	mat3 objectRotation,
	Model* model,
	AnimatingObject& animatingObject,
	std::map<int, BoneLimits>& limits,
	double scale,
	float dt,
	bool lowRes)
{
	if(lowRes)
	{
		animatingObject.lowResSkipCount = animatingObject.lowResSkipCount - 1;
		if(animatingObject.lowResSkipCount <= 0)
		{
			animatingObject.lowResSkipCount = 8;
		}
		else
		{
			return;
		}
	}
	ModelSkeleton* skeletons[2][4];
	float lerps[2];
	float durations[2];
	float yLerp = 0.0f;

	bool bilerp = (animatingObject.isInterpolatingAnimations && !lowRes);

	int animationCount = (bilerp ? 2 : 1);

	for(int animationIndex = 0; animationIndex < animationCount; animationIndex++)
	{
		float lerp = animatingObject.interpolationFractions[animationIndex];
		lerps[animationIndex] = lerp;//easeInEaseOut(lerp, animatingObject.keyframes[animationIndex][2]->easeIn, animatingObject.keyframes[animationIndex][2]->easeOut);

		durations[animationIndex] = 1.0 / animatingObject.keyframes[animationIndex][1]->speed;

		for(int i = 0; i < 4; i++)
		{
			skeletons[animationIndex][i] = &(model->keyframes[animatingObject.keyframes[animationIndex][i]->index]);
		}
	}

	if(bilerp)
	{
		yLerp = clamp(animatingObject.intraAnimationInterpolationFraction, 0.0, 1.0);
		yLerp = smoothstep(0.0f,1.0f,yLerp );
	}

	mat4 matrix = mat4(1.0f);
	for (int i = 0; i < model->defaultSkeleton.rootBones.size(); i++)
	{
		loadInterpolatedBoneUniformsRecursively(uniforms,
			objectRotation,
			model,
			animatingObject,
			limits,
			scale,
			dt,
			lowRes,
			&(model->defaultSkeleton.rootBones[i]),
			bilerp,
			yLerp,
			lerps,
			durations,
			matrix,
			skeletons);
	}
}

void load2DInterpolatedBoneUniformsRecursivelyForBones(ModelBoneUniforms* uniforms,
	mat3 objectRotation,
	ModelBoneMatrices* matrices,
	std::vector<mat4>& inverseBindMatrices,
	std::vector<vec3>& inverseBindMatrixOffsets,
	std::map<int, ModelBoneOverride>& overrides,
	std::map<int, BoneLimits>& limits,
	mat4& parentMatrix,
	double scale,
	float yLerp,

	ModelBone& boneAA1,
	ModelBone& boneA1,
	ModelBone& boneB1,
	ModelBone& boneBB1,
	float lerp1,
	float duration1,

	ModelBone& boneAA2,
	ModelBone& boneA2,
	ModelBone& boneB2,
	ModelBone& boneBB2,
	float lerp2,
	float duration2,

	float dt)
{
	vec3 incomingTranslation1 = daveBoneTranslationLerp(boneAA1.translation, boneA1.translation, boneB1.translation, boneBB1.translation, lerp1);
	vec3 incomingTranslation2 = daveBoneTranslationLerp(boneAA2.translation, boneA2.translation, boneB2.translation, boneBB2.translation, lerp2);

	vec3 incomingTranslation = mix(incomingTranslation1, incomingTranslation2, yLerp);

	quat incomingRotation1 = bezierNew(boneAA1.rotation, boneA1.rotation, boneB1.rotation, boneBB1.rotation, lerp1);
	quat incomingRotation2 = bezierNew(boneAA2.rotation, boneA2.rotation, boneB2.rotation, boneBB2.rotation, lerp2);

	quat incomingRotation = normalize(animationSlerp(incomingRotation1, incomingRotation2, yLerp));

	if(overrides.count(boneA1.boneIndex) != 0)
	{
		int boneIndex = boneA1.boneIndex;
		ModelBoneOverride& override = overrides[boneIndex];

		bool removed = false;

		if(override.transitionTowardRemoval)
		{
			override.rate = override.rate - override.rate * clamp(dt * 3.0f, 0.0f, 1.0f);
			override.removalTransitionFraction += override.rate * dt * 3.0;
			if(override.removalTransitionFraction >= 1.0)
			{
				overrides.erase(boneIndex);
				removed = true;
			}
		}
		else
		{
			override.rate = override.rate + (override.goalRate - override.rate) * clamp(dt * 3.0f, 0.0f, 1.0f);
			override.removalTransitionFraction -= override.rate * dt * 3.0;
			if(override.removalTransitionFraction <= 0.0)
			{
				override.removalTransitionFraction = 0.0;
			}
		}

		if(!removed)
		{
			quat overrideGoalToUse = override.rotationGoal;
			mat3 combinedBoneRotation = objectRotation * mat3(parentMatrix);
			mat3 combinedBoneRotationInverse = inverse(combinedBoneRotation);
			quat interpolatedRotationGoal = overrideGoalToUse;
			quat overrideRotation = incomingRotation;

			if(!override.initialized)
			{
				if(override.additive)
				{
					override.interpolatedRotationGoal = mat3(1.0f);
				}
				else
				{
					override.interpolatedRotationGoal = mat3(incomingRotation) * combinedBoneRotation;
				}
				override.initialized = true;
			}

			if(limits.count(boneIndex) != 0)
			{
				BoneLimits& boneLimits = limits[boneIndex];

				mat3 localRotationGoal;
				if(override.additive)
				{
					localRotationGoal = mat3(overrideGoalToUse);
				}
				else
				{
					localRotationGoal = combinedBoneRotationInverse * mat3(overrideGoalToUse);
				}

				if(localRotationGoal[2][1] < 1.0 && localRotationGoal[2][1] > -1.0 )
				{    
					vec2 angles = vec2(0.0,0.0);

					angles.x = -asin(localRotationGoal[2][1]);

					float cosYangle = sqrt(powf(localRotationGoal[0].x, 2.0f) + powf(localRotationGoal[0].y, 2.0f));
					angles.y = -atan2(localRotationGoal[0].z, cosYangle) * (1.0f - powf(fabsf(localRotationGoal[2][1]), 4.0f));

					for(int a = 0; a < 2; a++)
					{
						if(boneLimits.hasLimits[a])
						{
							angles[a] = clamp(angles[a], boneLimits.limits[a].x, boneLimits.limits[a].y);
						}
					}

					quat rotationQuat = normalize(quat(mat3(eulerAngleYXZ(angles.y, angles.x, 0.0f))));
					localRotationGoal = mat3(rotationQuat);

					if(override.additive)
					{
						overrideGoalToUse = localRotationGoal;
					}
					else
					{
						overrideGoalToUse = combinedBoneRotation * localRotationGoal;
					}
				}
				else
				{
					if(override.additive)
					{
						overrideGoalToUse = mat3(1.0);
					}
					else
					{
						overrideGoalToUse = combinedBoneRotation * mat3(incomingRotation);
					}
				}

				interpolatedRotationGoal = normalize(animationSlerp(override.interpolatedRotationGoal, overrideGoalToUse, clamp(boneLimits.rate * override.rate * dt, 0.0f, 1.0f)));

				if(override.additive)
				{
					overrideRotation = mat3(incomingRotation * interpolatedRotationGoal);
				}
				else
				{
					overrideRotation =  combinedBoneRotationInverse * mat3(interpolatedRotationGoal);
				}
			}
			else
			{
				interpolatedRotationGoal = normalize(animationSlerp(override.interpolatedRotationGoal, overrideGoalToUse, clamp(8.0f * override.rate * dt * 1.0f, 0.0f, 1.0f)));

				if(override.additive)
				{
					overrideRotation = mat3(incomingRotation * interpolatedRotationGoal);
				}
				else
				{
					overrideRotation =  combinedBoneRotationInverse * mat3(interpolatedRotationGoal);
				}
			}

			override.interpolatedRotationGoal = interpolatedRotationGoal;

			if(override.removalTransitionFraction > 0.00001)
			{
				incomingRotation = normalize(animationSlerp(overrideRotation, incomingRotation, override.removalTransitionFraction));
			}
			else
			{
				incomingRotation = overrideRotation;
			}
		}

	}


	mat4 matrix = translate(parentMatrix, incomingTranslation * (float)scale);

	matrix = matrix * toMat4(incomingRotation);

	matrices->boneMatrices[boneA1.boneIndex] = matrix;

	//quat rotation = (toQuat(matrix));
	//quat rotation = inverse(mat3(matrix));
	//vec3 translation = vec3(matrix[3]);

	mat4& inverseBindMatrix = inverseBindMatrices[boneA1.boneIndex];

	mat3 inverseMat =  inverse(mat3(matrix) * mat3(inverseBindMatrix));
	vec3 translation = inverseMat * vec3(matrix[3]) + inverseBindMatrixOffsets[boneA1.boneIndex] * (float)scale;

	quat rotation = inverseMat;

	memcpy(&(uniforms->boneRotations[boneA1.boneIndex][0]), value_ptr(rotation), sizeof(float) * 4);
	memcpy(&(uniforms->boneTranslations[boneA1.boneIndex][0]), value_ptr(translation), sizeof(float) * 3);

	for (int i = 0; i < boneA1.children.size(); i++)
	{
		load2DInterpolatedBoneUniformsRecursivelyForBones(uniforms,
			objectRotation,
			matrices,
			inverseBindMatrices,
			inverseBindMatrixOffsets,
			overrides,
			limits,
			matrix,
			scale,
			yLerp,

			boneAA1.children[i],
			boneA1.children[i],
			boneB1.children[i],
			boneBB1.children[i],
			lerp1,
			duration1,

			boneAA2.children[i],
			boneA2.children[i],
			boneB2.children[i],
			boneBB2.children[i],
			lerp2,
			duration2,
			dt);
	}
}

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
	float dt)
{
	mat4 matrix = mat4(1.0f);
	for (int i = 0; i < skeletonA1.rootBones.size(); i++)
	{
		load2DInterpolatedBoneUniformsRecursivelyForBones(uniforms,
			objectRotation,
			matrices,
			inverseBindMatrices,
			inverseBindMatrixOffsets,
			overrides,
			limits,
			matrix,
			scale,
			yLerp,

			skeletonAA1.rootBones[i],
			skeletonA1.rootBones[i],
			skeletonB1.rootBones[i],
			skeletonBB1.rootBones[i],
			xLerp1,
			duration1,

			skeletonAA2.rootBones[i],
			skeletonA2.rootBones[i],
			skeletonB2.rootBones[i],
			skeletonBB2.rootBones[i],
			xLerp2,
			duration2,
			dt);
	}
}


MJVMABuffer createModelBuffer(Vulkan* vulkan, VkCommandBuffer commandBuffer, Model* model)
{
    std::vector<ModelFace>& faces = model->faces;
    std::vector<ObjectVert> renderVerts(faces.size() * 3);
    for(int f = 0; f < faces.size(); f++)
    {
        for(int v = 0; v < 3; v++)
        {
            renderVerts[f * 3 + v].pos = faces[f].verts[v];
			renderVerts[f * 3 + v].uv = faces[f].uvs[v];
            renderVerts[f * 3 + v].normal = vec4(faces[f].normals[v] * vec3(127.0), 0);
			renderVerts[f * 3 + v].tangent = vec4(faces[f].tangents[v] * vec3(127.0), 0);
            renderVerts[f * 3 + v].material = faces[f].shaderMaterial;
			renderVerts[f * 3 + v].materialB = faces[f].shaderMaterialB;
        }
    }

    MJVMABuffer buffer;

	std::string debugName = "createModelBufferA_";
	debugName = debugName + model->debugName;

    vulkan->setupStaticBuffer(commandBuffer, &buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(ObjectVert) * renderVerts.size(), renderVerts.data(), debugName.c_str());

    return buffer;
}


MJVMABuffer createModelShadowBuffer(Vulkan* vulkan, VkCommandBuffer commandBuffer, Model* model)
{
	std::vector<ModelFace>& faces = model->shadowVolumeFaces;
	std::vector<ObjectVert> renderVerts(faces.size() * 3);
	for(int f = 0; f < faces.size(); f++)
	{
		for(int v = 0; v < 3; v++)
		{
			renderVerts[f * 3 + v].pos = faces[f].verts[v];
			renderVerts[f * 3 + v].uv = faces[f].uvs[v];
			renderVerts[f * 3 + v].normal = vec4(faces[f].normals[v] * vec3(127.0), 0);
			renderVerts[f * 3 + v].tangent = vec4(faces[f].tangents[v] * vec3(127.0), 0);
			renderVerts[f * 3 + v].material = faces[f].shaderMaterial;
			renderVerts[f * 3 + v].materialB = faces[f].shaderMaterialB;
		}
	}

	MJVMABuffer buffer;

	std::string debugName = "createModelShadowBufferA_";
	debugName = debugName + model->debugName;

	vulkan->setupStaticBuffer(commandBuffer, &buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(ObjectVert) * renderVerts.size(), renderVerts.data(), debugName.c_str());

	return buffer;
}


MJVMABuffer createModelBuffer(Vulkan* vulkan, VkCommandBuffer commandBuffer, Model* model, double scale, dmat3& rotationInverse, dvec3 translation)
{
    std::vector<ModelFace>& faces = model->faces;
    std::vector<ObjectVert> renderVerts(faces.size() * 3);
    for(int f = 0; f < faces.size(); f++)
    {
        for(int v = 0; v < 3; v++)
        {
            renderVerts[f * 3 + v].pos = (dvec3(faces[f].verts[v]) * rotationInverse * scale + translation);
			renderVerts[f * 3 + v].uv = faces[f].uvs[v];
            renderVerts[f * 3 + v].normal = vec4(faces[f].normals[v] * rotationInverse * 127.0, 0.0);
			renderVerts[f * 3 + v].tangent = vec4(faces[f].tangents[v] * rotationInverse * 127.0, 0.0);
            renderVerts[f * 3 + v].material = faces[f].shaderMaterial;
			renderVerts[f * 3 + v].materialB = faces[f].shaderMaterialB;
        }
    }

    MJVMABuffer buffer;

	std::string debugName = "createModelBufferB_";
	debugName = debugName + model->debugName;

    vulkan->setupStaticBuffer(commandBuffer, &buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(ObjectVert) * renderVerts.size(), renderVerts.data(), debugName.c_str());

    return buffer;
}

MJVMABuffer createModelEdgeBuffer(Vulkan* vulkan, VkCommandBuffer commandBuffer, Model* model)
{
    std::vector<ModelEdgeDecal>& edgeDecals = model->edgeDecals;
    if(edgeDecals.empty())
    {
        MJVMABuffer buffer = {};
        return buffer;
    }

    std::vector<ObjectDecalVert> renderVerts(edgeDecals.size() * 6);
    for(int d = 0; d < edgeDecals.size(); d++)
    {
        for(int t = 0; t < 2; t++)
        {
            for(int v = 0; v < 3; v++)
            {
                int quadVertIndex = (v + (t == 0 ? 0 : 1)) % 4;
                int normalIndex = (v + (t == 0 ? 0 : 1)) % 2;
                int offsetIndex = (v + (t == 0 ? 0 : 1)) / 2;

                renderVerts[d * 6 + t * 3 + v].pos = edgeDecals[d].verts[quadVertIndex];
                renderVerts[d * 6 + t * 3 + v].normal = vec4(edgeDecals[d].normals[normalIndex] * vec3(127.0), 0);
				renderVerts[d * 6 + t * 3 + v].faceNormal = vec4(edgeDecals[d].faceNormal * vec3(127.0), 0);
                renderVerts[d * 6 + t * 3 + v].uv = vec3(edgeDecals[d].uvs[quadVertIndex], offsetIndex);
                renderVerts[d * 6 + t * 3 + v].material = edgeDecals[d].shaderMaterial;
				renderVerts[d * 6 + t * 3 + v].materialB = edgeDecals[d].shaderMaterialB;
                renderVerts[d * 6 + t * 3 + v].decalLocalOrigin = (edgeDecals[d].verts[0] + edgeDecals[d].verts[1]) * vec3(0.5);
            }
        }
    }

    MJVMABuffer buffer;

	std::string debugName = "createModelEdgeBuffer_";
	debugName = debugName + model->debugName;

    vulkan->setupStaticBuffer(commandBuffer, &buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(ObjectDecalVert) * renderVerts.size(), renderVerts.data(), debugName.c_str());

    return buffer;
}

/*
ClientModel::ClientModel()
{
}


ClientModel::~ClientModel()
{
}
*/
