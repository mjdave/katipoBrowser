#include "hueShift.frag"


layout(binding = 0) uniform UniformBufferObject {
    mat4 modelMatrix;
    vec4 color;
    vec4 size;
    vec4 animationTimer;
    vec4 shaderUniformA;
    vec4 shaderUniformB;
    mat4 clipMatrix;
} ubo;

layout(binding = 1) uniform cameraUniformBufferObject {
  mat4 proj;
  mat4 view;
  mat4 worldView;
  mat4 worldRotation;
	vec4 camOffsetPos;
	vec4 camOffsetPosWorld;
    vec4 extraData;
} camera;

layout(location = 0) in vec4 outColor;
layout(location = 1) in vec4 outTimer;
layout(location = 2) in vec4 outShaderUniformA;
layout(location = 3) in vec4 outShaderUniformB;

layout(location = 0) out vec4 data;

void main(void)
{
    vec3 hueShiftResult = hueShift(outColor.rgb, camera.extraData.y * 0.2);
    data = vec4(hueShiftResult, outColor.a);
}
