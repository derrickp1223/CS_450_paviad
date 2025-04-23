#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 interPos;
layout(location = 1) in vec4 interColor;
layout(location = 2) in vec3 interNormal;
layout(location = 3) in vec3 interText3D;

struct PointLight {
    vec4 pos;
    vec4 vpos;
    vec4 color;
};

layout(set=0, binding=1) uniform UBOFragment {
    PointLight light;
} ubo;

float sineProcText(vec3 texcoords, float scale) {
    float factor = sin(scale*texcoords.x/texcoords.z);
    factor = (factor + 1.0)/2.0;
    factor = (factor > 0.3) ? 1.0 : 0.0;
    return factor;
}

void main() {
    vec3 N = normalize(interNormal);
    //outColor = vec4(1.0, 0.0, 0.0, 1.0);
    //outColor = interColor;
    //N = (N + 1.0)/2.0;
    //outColor = vec4(N, 1.0);

    vec3 L = vec3(ubo.light.vpos) - interPos;
    float d = length(L);
    L = normalize(L);
    float ref = 1.0;
    float att = ref/(d*d + ref);
    vec4 colorAtt = vec4(att,att,att,1.0);

    vec3 baseColor = vec3(interColor);
    float colorFactor = sineProcText(interText3D, 30);
    baseColor *= colorFactor;

    float diffuse = max(0, dot(L, N));
    vec3 diffColor = diffuse*vec3(ubo.light.color)*baseColor;

    //outColor = vec4(diffColor, 1.0);
    outColor = vec4(baseColor, 1.0);
} 
