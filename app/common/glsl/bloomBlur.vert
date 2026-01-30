layout(location = 0) in vec2 pos;

layout(location = 0) out vec2 outPos;

void main()
{
    outPos = vec2(pos.x, pos.y);
    gl_Position = vec4(pos.x * 2.0 - 1.0, pos.y * 2.0 - 1.0, 0.0, 1.0);
}
