
#define reflectanceMultiplier 0.67

const vec3 sunPos = normalize(vec3(0.6,1.0,0.7));


const float uiLightZOffset = 0.01;
const float uiLightIntensity = 80.0;

#define UI_LIGHT_COUNT 2


const float uiLightRotationSpeeds[UI_LIGHT_COUNT] = {
	1.0,
	-0.4,
	//0.67,
	//1.13,
	/*0.25,
	1.37,
	1.21,
	0.89*/
};

const vec3 uiLightPositions[UI_LIGHT_COUNT] = {
    normalize(vec3(0.1,0.03,uiLightZOffset)),
    normalize(vec3(-0.04,0.06,uiLightZOffset)),
    //normalize(vec3(0.05,0.04,uiLightZOffset)),
    //normalize(vec3(0.02,0.05,uiLightZOffset))
    /*normalize(vec3(-0.1,0.02,uiLightZOffset)),
    normalize(vec3(-0.3,-0.3,uiLightZOffset)),
    normalize(vec3(-0.1,-0.04,uiLightZOffset)),
    normalize(vec3(-0.02,0.05,uiLightZOffset))*/
};

vec3 uiLightGetPosition(int lightIndex, float animationTimer)
{
	float timer = animationTimer * 0.25;//pow(min(animationTimer * 4.0, 3.0) / 3.0, 0.125);
	float rotation = (timer + 20.65) * uiLightRotationSpeeds[lightIndex] * 0.5;
	float cosTheta = cos(rotation);
	float sinTheta = sin(rotation);
	vec3 basePos = uiLightPositions[lightIndex];
	return normalize(vec3((basePos.x * cosTheta - basePos.y * sinTheta) * 4.0, 
	(basePos.x * sinTheta + basePos.y * cosTheta) * 0.5 + uiLightZOffset * 30.0, basePos.z));
}

/*

	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	float exposure = texture(exposureSampler, vec2(0.5,0.5)).g;
	color = color * exposure * exposure * exposure * 0.05;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
    float gamma = 1.13 + exposure * 0.01;
	color = pow(color, vec3(1.0 / gamma));
	return color;
*/


const vec3 coeff = vec3(0.299,0.587,0.114);

vec3 adjustLightMapLookupForUI(vec3 color) {
    float lum = dot(color, coeff);
	return mix(color, vec3(lum), 0.8) * 0.5;
}

vec3 adjustLightMapLookupForGameObjectIcon(vec3 color) {
    float lum = dot(color, coeff);
	return mix(color, vec3(lum), 0.8) * 0.8;//pow(vec3(lum) * 0.4, vec3(0.8));
}

const float vibranceAmount = 0.2;
vec3 vibrance(vec3 color)
{
    float lum = dot(color, coeff);
    vec3 mask = (color - vec3(lum));
    mask = clamp(mask, 0.0, 1.0);
    float lumMask = dot(coeff, mask);
    lumMask = 1.0 - lumMask;
    return mix(vec3(lum), color, 1.0 + vibranceAmount * lumMask);
}

vec3 uiHDR(vec3 color) {

	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	color = color * 50.0;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
    float gamma = 1.28;
	color = pow(color, vec3(1.0 / gamma));
	return vibrance(color);
}


float uiLightSpecular(vec3 N, vec3 V, vec3 L, float roughness)
{
    vec3 H = normalize(V + L);

    float dotNH = max(dot(N, H), 0.0);

    float alpha = min(roughness * roughness, 1.0);

    float alphaSqr = alpha * alpha;
    float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0;
    float D = alphaSqr / (denom * denom);

    return D / 4.0;
}