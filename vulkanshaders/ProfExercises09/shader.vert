#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 interPos;
layout(location = 1) out vec4 interColor;
layout(location = 2) out vec3 interNormal;
layout(location = 3) out vec3 interText3D;

layout(push_constant) uniform PushConstants {
    mat4 modelMat;
    mat4 normMat;
} pc;

layout(set=0, binding=0) uniform UBOVertex {
    mat4 viewMat;
    mat4 projMat;
} ubo;

void main() {
    vec4 pos = vec4(position, 1.0);
    vec4 vpos = ubo.viewMat*pc.modelMat*pos;
    pos = ubo.projMat*vpos;

    interPos = vec3(vpos);
    interColor = color;
    mat3 normMat = mat3(pc.normMat);
    interNormal = normMat*normal; 

    interText3D = vec3(pc.modelMat*vec4(position,1.0));

    gl_Position = pos;  
} 
