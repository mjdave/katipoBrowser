
//https://gist.github.com/mairod/a75e7b44f68110e1576d77419d608786
vec3 hueShift(vec3 color, float dhue) {
	float s = sin(dhue);
	float c = cos(dhue);
	vec3 hueShiftResult = (color * c) + (color * s) * mat3(
		vec3(0.167444, 0.329213, -0.496657),
		vec3(-0.327948, 0.035669, 0.292279),
		vec3(1.250268, -1.047561, -0.202707)
	) + dot(vec3(0.299, 0.587, 0.114), color) * (1.0 - c);


    float strengthMultiplierR = 0.75 + sin(dhue * 0.021) * 0.5;
    float strengthMultiplierG = 0.75 + sin(dhue * 0.041 + 0.8) * 0.5;
    float strengthMultiplierB = 0.75 + sin(dhue * 0.07 + 2.3) * 0.5;
    hueShiftResult.r = hueShiftResult.r * strengthMultiplierR;
    hueShiftResult.g = hueShiftResult.g * strengthMultiplierG;
    hueShiftResult.b = hueShiftResult.b * strengthMultiplierB;

    return hueShiftResult;
}