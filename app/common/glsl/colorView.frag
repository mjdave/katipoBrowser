

layout(location = 0) in vec4 outColor;
layout(location = 1) in vec4 outTimer;
layout(location = 2) in vec4 outShaderUniformA;
layout(location = 3) in vec4 outShaderUniformB;

layout(location = 0) out vec4 data;

void main(void)
{
    data = outColor;
}
