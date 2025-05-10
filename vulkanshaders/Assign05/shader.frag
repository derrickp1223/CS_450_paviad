#version 450

// Input from vertex shader
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec4 interPos;
layout(location = 2) in vec3 interNormal;

// Output color
layout(location = 0) out vec4 outColor;

// Constants
const float PI = 3.14159265359;

// Point light structure
struct PointLight {
    vec4 pos;    // Position in world space
    vec4 vpos;   // Position in view space
    vec4 color;  // Light color
};

// UBO for fragment shader data
layout(set = 0, binding = 1) uniform UBOFragment {
    PointLight light;
    float metallic;
    float roughness;    
} ubo;

// Calculate Fresnel reflectance at angle zero
vec3 getFresnelAtAngleZero(vec3 albedo, float metallic) {
    // Start with default value for insulators
    vec3 F0 = vec3(0.04);
    // Interpolate between insulator and metal based on metallic value
    F0 = mix(F0, albedo, metallic);
    return F0;
}

// Calculate Fresnel reflectance using Schlick's approximation
vec3 getFresnel(vec3 F0, vec3 L, vec3 H) {
    float cosAngle = max(0.0, dot(L, H));
    // Schlick approximation for Fresnel reflectance
    return F0 + (1.0 - F0) * pow(1.0 - cosAngle, 5.0);
}

// Calculate Normal Distribution Function using GGX/Trowbridge-Reitz
float getNDF(vec3 H, vec3 N, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(0.0, dot(N, H));
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return a2 / denom;
}

// Schlick's approximation for geometric attenuation
float getSchlickGeo(vec3 B, vec3 N, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float NdotB = max(0.0, dot(N, B));
    
    return NdotB / (NdotB * (1.0 - k) + k);
}

// Calculate Geometry Function for shadowing and masking
float getGF(vec3 L, vec3 V, vec3 N, float roughness) {
    float GL = getSchlickGeo(L, N, roughness);
    float GV = getSchlickGeo(V, N, roughness);
    
    return GL * GV;
}

void main() {
    // Normalize the interpolated normal
    vec3 N = normalize(interNormal);
    
    // Calculate light vector (from fragment to light)
    vec3 L = normalize(vec3(ubo.light.vpos) - vec3(interPos));
    
    // Set base color from fragment color
    vec3 baseColor = vec3(fragColor);
    
    // Calculate view vector (from fragment to camera, which is at origin in view space)
    vec3 V = normalize(-vec3(interPos));
    
    // Calculate Fresnel reflectance at angle zero
    vec3 F0 = getFresnelAtAngleZero(baseColor, ubo.metallic);
    
    // Calculate half vector
    vec3 H = normalize(V + L);
    
    // Calculate Fresnel reflectance
    vec3 F = getFresnel(F0, L, H);
    
    // Set specular color
    vec3 kS = F;
    
    // Calculate diffuse color
    vec3 kD = (1.0 - kS) * (1.0 - ubo.metallic) * baseColor / PI;
    
    // Calculate specular reflection
    float NDF = getNDF(H, N, ubo.roughness);
    float G = getGF(L, V, N, ubo.roughness);
    
    // Complete specular term
    vec3 specular = kS * NDF * G / (4.0 * max(0.0, dot(N, L)) * max(0.0, dot(N, V)) + 0.0001);
    
    // Calculate final color
    vec3 finalColor = (kD + specular) * vec3(ubo.light.color) * max(0.0, dot(N, L));
    
    // Output final color
    outColor = vec4(finalColor, 1.0);
}