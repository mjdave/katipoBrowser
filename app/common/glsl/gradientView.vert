
layout(binding = 0) uniform UniformBufferObject {
    mat4 modelMatrix;
    vec4 color; //colorA
    vec4 size;
    vec4 animationTimer;
    vec4 shaderUniformA;  //colorB
    vec4 shaderUniformB;  //vec3 direction, (todo maybe offset along w)
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

layout(location = 0) in vec2 pos;

layout(location = 0) out vec2 outPos;

void main(void)
{
    gl_Position = camera.proj * camera.worldView * ubo.modelMatrix * vec4(pos * ubo.size.xy, 0.0, 1.0);

    outPos = pos;
}
