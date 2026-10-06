
layout(binding = 0) uniform UniformBufferObject {
    mat4 modelMatrix;
    vec4 color; //colorA
    vec4 size;
    vec4 animationTimer;
    vec4 shaderUniformA;  //colorB
    vec4 shaderUniformB;  //vec3 direction, (todo maybe offset along w)
    mat4 clipMatrix;
} ubo;

layout(location = 0) in vec2 outPos;

layout(location = 0) out vec4 data;

void main(void)
{
    float mixValue = dot(outPos, ubo.shaderUniformB.xy);
    data = mix(ubo.color, ubo.shaderUniformA, mixValue);
}
