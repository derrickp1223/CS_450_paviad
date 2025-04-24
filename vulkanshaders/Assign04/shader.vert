#version 450

// UBO for view and projection matrices
layout(binding = 0) uniform UBOVertex {
    mat4 view;
    mat4 projection;
} ubo;

// Push constant for model matrix
layout(push_constant) uniform UPushVertex {
    mat4 model;
} push;

// Vertex input attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

// Output to fragment shader
layout(location = 0) out vec4 fragColor;

void main() {
    // Apply model, view, and projection transformations
    gl_Position = ubo.projection * ubo.view * push.model * vec4(inPosition, 1.0);
    
    // Pass color to fragment shader
    fragColor = inColor;
}