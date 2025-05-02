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
    mat4 normMat;  // Added normal matrix
} pc;

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;  // Added normal input

// Output to fragment shader
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 interPos;  // Added interpolated position
layout(location = 2) out vec3 interNormal;  // Added interpolated normal

void main() {
    // Transform vertex position using model, view, and projection matrices
    // Apply RIGHT-TO-LEFT multiplication order
    gl_Position = ubo.projMat * ubo.viewMat * pc.modelMat * vec4(inPosition, 1.0);
    
    // Set interpolated position in view coordinates
    interPos = ubo.viewMat * pc.modelMat * vec4(inPosition, 1.0);
    
    // Set interpolated normal
    interNormal = mat3(pc.normMat) * inNormal;
    
    // Pass color to fragment shader
    fragColor = inColor;
}