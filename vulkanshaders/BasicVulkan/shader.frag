#version 450

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {

    // Gamma-correct (linear to sRGB)
    vec4 finalColor = fragColor;
    //finalColor.rgb = pow(finalColor.rgb, vec3(2.2));

    // Output final color
    outColor = finalColor;
} 
