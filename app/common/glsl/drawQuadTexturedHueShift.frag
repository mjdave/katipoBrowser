#include "hueShift.frag"

layout(binding = 0) uniform UniformBufferObject {
  mat4 modelMatrix;
  vec4 color;
  vec4 size;
  vec4 userData; //an initial offset, given as second arg in button.create
} ubo;

layout(binding = 1) uniform cameraUniformBufferObject {
  mat4 proj;
  mat4 view;
  mat4 worldView;
  mat4 worldRotation;
	vec4 camOffsetPos;
	vec4 camOffsetPosWorld;
  vec4 extraData; //animation timer in y
} camera;

layout(binding = 2) uniform sampler2D texMap;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec2 greyScale;

layout(location = 0) out vec4 outColor;


void main() {
    vec4 tex = texture(texMap, fragTexCoord);
    vec3 hueShiftResult = hueShift(tex.rgb, camera.extraData.y * 0.2 + ubo.userData.x);
    outColor = vec4(hueShiftResult, tex.a) * fragColor;
}