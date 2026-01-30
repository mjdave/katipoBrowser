
layout(binding = 2) uniform sampler2D texMap;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 tex = texture(texMap, fragTexCoord);
    outColor = vec4(1.0,1.0,1.0,tex.a * 0.2) * fragColor;// * vec4(1.0, 1.0,tex.a,0.5);
}