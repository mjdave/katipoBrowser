
layout(binding = 0) uniform UniformBufferObject {
  mat4 modelMatrix;
  vec4 color;
  vec4 size;
  vec4 userData;
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

layout(binding = 2) uniform sampler2D texMap;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec2 greyScale;

layout(location = 0) out vec4 outColor;

//https://gist.github.com/mairod/a75e7b44f68110e1576d77419d608786
vec3 hueShift(vec3 color, float dhue) {
	float s = sin(dhue);
	float c = cos(dhue);
	return (color * c) + (color * s) * mat3(
		vec3(0.167444, 0.329213, -0.496657),
		vec3(-0.327948, 0.035669, 0.292279),
		vec3(1.250268, -1.047561, -0.202707)
	) + dot(vec3(0.299, 0.587, 0.114), color) * (1.0 - c);
}

void main() {
    vec4 tex = texture(texMap, fragTexCoord);
    vec3 hueShiftResult = hueShift(tex.rgb, camera.extraData.y * 0.2 + ubo.userData.x);
    float strengthMultiplierR = 0.75 + sin(camera.extraData.y * 0.021) * 0.5;
    float strengthMultiplierG = 0.75 + sin(camera.extraData.y * 0.041 + 0.8) * 0.5;
    float strengthMultiplierB = 0.75 + sin(camera.extraData.y * 0.07 + 2.3) * 0.5;
    hueShiftResult.r = hueShiftResult.r * strengthMultiplierR;
    hueShiftResult.g = hueShiftResult.g * strengthMultiplierG;
    hueShiftResult.b = hueShiftResult.b * strengthMultiplierB;
    //hueShiftResult.r = pow(hueShiftResult.r, strengthMultiplierR);
    //hueShiftResult.g = pow(hueShiftResult.g, strengthMultiplierG);
    //hueShiftResult.b = pow(hueShiftResult.b, strengthMultiplierB);
    outColor = vec4(hueShiftResult, tex.a) * fragColor;
}