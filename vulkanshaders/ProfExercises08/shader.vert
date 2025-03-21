#version 450

layout(location = 0) in vec3 position;

layout(push_constant) uniform PushConstants {
    mat4 modelMat;
} pc;

layout(set=0, binding=0) uniform UBOVertex {
    mat4 viewMat;
    mat4 projMat;
} ubo;

void main() {
    vec4 pos = vec4(position, 1.0);
    pos = ubo.projMat*ubo.viewMat*pc.modelMat*pos;

    gl_Position = pos;  
} 
