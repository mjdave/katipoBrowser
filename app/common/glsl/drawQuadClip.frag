
layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec4 outClipPos;

void main() {
    if(outClipPos.x > 1.0 || outClipPos.x < 0.0 || outClipPos.y > 1.0 || outClipPos.y < 0.0)
    {
        discard;
    }
    outColor = fragColor;
}