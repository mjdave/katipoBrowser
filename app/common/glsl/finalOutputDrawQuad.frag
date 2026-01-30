
layout(binding = 0) uniform sampler2D texMap;
layout(binding = 1) uniform sampler2D bloomTex;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const float bloomIntensity = 5.0;

void main() {
    vec4 bloomValue = texture(bloomTex, fragTexCoord) * bloomIntensity;
   // bloomValue = bloomValue * bloomValue;
    //outColor = texture(texMap, fragTexCoord) + vec4(bloomValue.xyz, 1.0) * 2.0;
    outColor = mix(texture(texMap, fragTexCoord), vec4(vec3(0.8) + bloomValue.xyz * 0.3, 1.0), min((bloomValue.x + bloomValue.y + bloomValue.z) * 0.12, 1.0));// + vec4(bloomValue.xyz * bloomValue.xyz, 0.0);
}