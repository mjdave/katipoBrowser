
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

layout(location = 0) in vec2 pos;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outTimer;
layout(location = 2) out vec4 outShaderUniformA;
layout(location = 3) out vec4 outShaderUniformB;

layout(location = 4) out vec4 outClipPos;


void main(void)
{
    gl_Position = camera.proj * camera.worldView * ubo.modelMatrix * vec4(pos * ubo.size.xy, 0.0, 1.0);

    outColor = ubo.color;
    outTimer = ubo.animationTimer;
    outShaderUniformA = ubo.shaderUniformA;
    outShaderUniformB = ubo.shaderUniformA;
    
    outClipPos = ubo.clipMatrix * vec4(pos * ubo.size.xy, 0.0, 1.0);
}
