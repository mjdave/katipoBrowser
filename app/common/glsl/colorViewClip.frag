

layout(location = 0) in vec4 outColor;
layout(location = 1) in vec4 outTimer;
layout(location = 2) in vec4 outShaderUniformA;
layout(location = 3) in vec4 outShaderUniformB;
layout(location = 4) in vec4 outClipPos;

layout(location = 0) out vec4 data;


void main(void)
{
    if(outClipPos.x > 1.0 || outClipPos.x < 0.0 || outClipPos.y > 1.0 || outClipPos.y < 0.0)
    {
        discard;
    }
    data = outColor;
}
