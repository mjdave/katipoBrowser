
layout(location = 0) in vec2 position;

layout(location = 0) out vec2 fragTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = vec4(position * 2.0 - vec2(1,1), 0.0, 1.0);
    fragTexCoord = position;
}