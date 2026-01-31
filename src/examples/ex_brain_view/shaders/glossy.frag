#version 440

layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_color;
layout(location = 3) in vec3 v_viewDir;
layout(location = 4) in float v_curvature;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float _pad1;
    vec3 lightDir;
    float _pad2;
    float lightingEnabled;
    vec3 _pad3;
};

void main() {
    // Normalize interpolated vectors
    vec3 N = normalize(v_normal);
    vec3 V = normalize(v_viewDir);
    vec3 L = normalize(lightDir);
    
    // Use the vertex color directly (atlas colors)
    vec3 base_color = v_color;
    
    // === SIMPLE PHONG LIGHTING ===
    // Ambient
    float ao_strength = pow(v_curvature, 0.5);
    vec3 ambient = base_color * 0.3 * ao_strength;
    
    // Diffuse
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = base_color * diff * 0.6;
    
    // Specular (subtle)
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    vec3 specular = vec3(0.3) * spec;  // White specular highlights
    
    // === COMBINE ===
    vec3 final_color = ambient + diffuse + specular;
    
    // Opaque rendering for atlas
    fragColor = vec4(final_color, 1.0);
}
