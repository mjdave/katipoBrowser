

layout(binding = 0) uniform UniformBufferObject {
  mat4 modelMatrix;
  vec4 color;
  vec4 size;
  vec4 userData;
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

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec4 fragColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = camera.proj * camera.worldView * ubo.modelMatrix * vec4(inPosition * ubo.size.xy, 0.0, 1.0);
    fragColor = ubo.color;
}