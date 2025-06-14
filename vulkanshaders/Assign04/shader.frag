#version 450

// Input from vertex shader
layout(location = 0) in vec4 fragColor;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Simply output the color passed from vertex shader
    outColor = fragColor;
}