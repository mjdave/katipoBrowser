

layout(binding = 0) uniform sampler2D texMap;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 tex = texture(texMap, fragTexCoord);
    
    //float intensity = (tex.x + tex.y + tex.z) / 3.0;
    //intensity = intensity * intensity;
    tex.rgb = tex.rgb * smoothstep(vec3(0.8), vec3(1.5), tex.rgb);

    outColor = vec4(tex.rgb,1.0);
}