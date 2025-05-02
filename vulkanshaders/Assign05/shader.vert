#version 450
#extension GL_ARB_separate_shader_objects : enable

// UBO for view and projection matrices
layout(binding = 0) uniform UBOVertex {
    mat4 viewMat;
    mat4 projMat;
} ubo;

// Push constants for model matrix and vertex data
layout(push_constant) uniform UPushVertex {
    vec3 pos;
    vec4 color;
    mat4 modelMat;
} pushVertex;

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

// Output to fragment shader
layout(location = 0) out vec4 fragColor;

void main() {
    // Transform vertex position using model, view, and projection matrices
    // Apply RIGHT-TO-LEFT multiplication order
    gl_Position = ubo.projMat * ubo.viewMat * pushVertex.modelMat * vec4(inPosition, 1.0);
    
    // Pass color to fragment shader
    fragColor = inColor;
}
