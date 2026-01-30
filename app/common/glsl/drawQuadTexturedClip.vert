layout(binding = 0) uniform UniformBufferObject {
  mat4 modelMatrix;
  vec4 color;
  vec4 size_greyScale;
  vec4 userData;
  vec4 texMinAndScale;
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

layout(location = 0) in vec2 position;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec2 greyScale;
layout(location = 3) out vec4 outClipPos;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = camera.proj * camera.worldView * ubo.modelMatrix * vec4(position * ubo.size_greyScale.xy, 0.0, 1.0);
    fragColor = ubo.color;
    fragTexCoord = (position * ubo.texMinAndScale.zw) + ubo.texMinAndScale.xy;
    greyScale = ubo.size_greyScale.zw;
    
    outClipPos = ubo.clipMatrix * vec4(position * ubo.size_greyScale.xy, 0.0, 1.0);
}