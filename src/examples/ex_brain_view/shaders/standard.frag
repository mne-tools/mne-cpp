#version 440

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec3 v_color;
layout(location = 2) in vec3 v_worldPos;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float isSelected; // 0.0 or 1.0 (replaces _pad1)
    vec3 lightDir;
    float _pad2;
    float lightingEnabled;
    vec3 _pad3;
};

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(cameraPos - v_worldPos);
    
    // Simple Lambertian shading
    float diff = max(dot(N, L), 0.0);
    
    // Use curvature color (black for sulci, white for gyri)
    vec3 baseColor = v_color;
    
    // Ambient + Diffuse (High brightness for gyri)
    vec3 ambient = baseColor * 0.8;
    vec3 diffuse = baseColor * diff * 0.8;
    
    vec3 finalColor = clamp(ambient + diffuse, 0.0, 1.0);
    
    // --- Suble Hover Glow (Silver Rim) ---
    if (isSelected > 0.5) {
        float fresnel = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 3.0);
        vec3 glowColor = vec3(1.0, 1.0, 1.0); // Pure White/Silver
        finalColor += glowColor * 0.15; // Base glow 
        finalColor += glowColor * fresnel * 0.85; // Rim glow
    }
    
    // Opaque
    fragColor = vec4(finalColor, 1.0);
}
