#include "hueShift.frag"

layout(binding = 0) uniform UniformBufferObject {
  mat4 modelMatrix;
  vec4 color;
  vec4 size;
  vec4 userData;
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

layout(binding = 2) uniform sampler2D texMap;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec2 greyScale;
layout(location = 3) in vec4 outClipPos;

layout(location = 0) out vec4 outColor;

void main() {
    if(outClipPos.x > 1.0 || outClipPos.x < 0.0 || outClipPos.y > 1.0 || outClipPos.y < 0.0)
    {
        discard;
    }
    vec4 tex = texture(texMap, fragTexCoord);
    outColor = vec4(hueShift(tex.rgb, camera.extraData.y * 0.2 + ubo.userData.x), tex.a) * fragColor;
}