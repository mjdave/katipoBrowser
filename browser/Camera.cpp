//
//  Materials.cpp
//  Ambience
//
//  Created by David Frampton on 27/08/18.
//Copyright � 2018 Majic Jungle. All rights reserved.
//

#include "Camera.h"
#include "GCommandBuffer.h"
#include "GameConstants.h"

Camera::Camera(Vulkan* vulkan_)
{
    vulkan = vulkan_;

    cameraUBO = new CameraUBO();
    vulkan->setupDynamicBufferForMainOutput(&cameraBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(CameraUBO), "CameraUBO");

}


Camera::~Camera()
{
    delete cameraUBO;
    vulkan->destroyDynamicMultiBuffer(cameraBuffer);
}

void Camera::update(int commandBufferIndex,
	const mat4& leftProj, 
	const mat4& leftView,
	const mat4& worldRotation,
                    float dt,
                    float animationTimer)
{
	mat4 leftViewInverse = inverse(leftView);
	vec4 leftOffsetWorldScale = vec4(dvec3(leftViewInverse[3]), 1.0);

	cameraUBO->proj = leftProj;
	cameraUBO->view = leftView;
	cameraUBO->worldView = leftView;
	cameraUBO->worldView[3] = vec4(dvec3(leftView[3]), 1.0);
	cameraUBO->worldRotation = mat4(mat3(leftView));
	cameraUBO->camOffsetPos = leftOffsetWorldScale;
	cameraUBO->camOffsetPosWorld = worldRotation * leftOffsetWorldScale;
    cameraUBO->extraData = vec4(dt,animationTimer,0,0);

    MJVMABuffer buffer = cameraBuffer[commandBufferIndex];
    
    mjCopyGPUMemory(vulkan->vmaAllocator, buffer, *cameraUBO);
}




