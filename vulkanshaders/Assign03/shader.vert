#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

// Push constant for model matrix
layout(push_constant) uniform UPushVertex {
    mat4 modelMat;
} pc;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
} 
