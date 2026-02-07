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
    float tissueType;  // 0=Unknown, 1=Brain, 2=Skin, 3=OuterSkull, 4=InnerSkull
    float lightingEnabled;
    vec3 _pad3;
};

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(cameraPos - v_worldPos);
    vec3 H = normalize(L + V); // Half-vector for Blinn-Phong
    
    // Lambertian diffuse with smoother falloff
    float NdotL = max(dot(N, L), 0.0);
    float diff = NdotL;
    
    // Blinn-Phong specular for surface sheen
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, 32.0) * 0.3;
    
    // Base color from curvature (gyri=light, sulci=dark)
    vec3 baseColor = v_color;
    
    // Richer brain tissue tones - warm ivory/cream instead of pure white
    vec3 warmTint = vec3(1.0, 0.97, 0.94); // Subtle warm tint
    baseColor *= warmTint;
    
    // Improved lighting with lower ambient for better depth perception
    float ambient = 0.35;  // Much lower ambient for visible shadows
    float diffuseStrength = 0.65;  // Stronger diffuse for depth
    
    // Add subtle fill light from below to prevent pure-black sulci
    vec3 fillLightDir = normalize(vec3(0.0, -0.5, 0.3));
    float fillDiff = max(dot(N, fillLightDir), 0.0) * 0.15;
    
    vec3 finalColor = baseColor * (ambient + diff * diffuseStrength + fillDiff);
    finalColor += vec3(0.95, 0.95, 1.0) * spec; // Cool-tinted specular
    
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    // --- Enhanced Hover Glow (Golden-Yellow for visibility) ---
    if (isSelected > 0.5) {
        float fresnel = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 2.5);
        vec3 glowColor = vec3(1.0, 0.85, 0.3); // Gold/Yellow for visibility
        finalColor = mix(finalColor, glowColor, 0.25); // Blend base with gold
        finalColor += glowColor * fresnel * 0.6; // Golden rim glow
    }
    
    // Opaque
    fragColor = vec4(finalColor, 1.0);
}
