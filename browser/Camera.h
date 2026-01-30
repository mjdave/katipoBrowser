
#ifndef Camera_h
#define Camera_h

#include "Vulkan.h"

//struct __attribute__((packed)) CameraUBO {
struct CameraUBO {
    mat4 proj;
	mat4 view;
	mat4 worldView;
	mat4 worldRotation;
	vec4 camOffsetPos;
	vec4 camOffsetPosWorld;
    vec4 extraData; //dt, animationTimer
};

class Camera {

public: // members

	std::vector<MJVMABuffer> cameraBuffer;
	CameraUBO* cameraUBO;

protected: // members
    Vulkan* vulkan;

	bool vrEnabled;

public: // functions
	Camera(Vulkan* vulkan_);
    ~Camera();

	void update(int commandBufferIndex,
		const mat4& leftProj, 
		const mat4& leftView,
		const mat4& worldRotation,
                float dt,
                float animationTimer);

protected: // functions

};

#endif /* MaterialManager_h */
