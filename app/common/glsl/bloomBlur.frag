layout(binding = 0) uniform UniformBufferObject {
	vec4 screenSize;
} ubo;


layout(binding = 1) uniform sampler2D inputMap;
//layout(binding = 2) uniform sampler2D original;

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 outPos;


//const float offset[3] = {0.0, 1.3846153846, 3.2307692308}; //correct
//const float weight[3] = {0.2270270270, 0.3162162162, 0.0702702703};


//const float offset[4] = {0.0, 1.3846153846 * 2.0, 3.2307692308 * 2.0, 4.3307692308 * 2.0};
//const float weight[4] = {0.2, 0.2, 0.1, 0.1};

const float offset[3] = {0.0, 1.3846153846 * 1.3, 3.2307692308 * 1.3};
const float weight[3] = {0.5, 0.15, 0.1};

vec3 getVertical(vec2 inPos)
{
    vec3 result = texture(inputMap, inPos).rgb * weight[0];
    vec2 texelSize = 0.7 / ubo.screenSize.xy;

    for (int i=1; i<3; i++) {
        vec2 offsetValue = vec2(0.0, offset[i] * texelSize.y);
        result += texture(inputMap, (inPos + offsetValue)).rgb * weight[i];
        result += texture(inputMap, (inPos - offsetValue)).rgb * weight[i];
    }

    return result;
}

void main() {
    /*vec2 texelSize = 1.67 / ubo.screenSize.xy;
    float result = 0.0;
    for (int x = -4; x < 4; ++x) 
    {
        for (int y = -4; y < 4; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(inputMap, outPos + offset).r;
        }
    }
    result = result / 64.0;
    outColor = vec4(vec3(result), 1.0);*/


    vec2 texelSize = 0.7 / ubo.screenSize.xy;
    vec3 result = getVertical(outPos) * weight[0];

    for (int i=1; i<3; i++) {
        vec2 offsetValue = vec2(offset[i] * texelSize.x, 0.0);
        result += getVertical(outPos + offsetValue) * weight[i];
        result += getVertical(outPos - offsetValue) * weight[i];
    }

    //result = result;// + texture(original, outPos).rgb * 0.4;

    outColor = vec4(result, 1.0);
}