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
    
    // === DATA DETECTION (Saturation) ===
    float gray_val = dot(v_color, vec3(0.299, 0.587, 0.114));
    float saturation = length(v_color - vec3(gray_val));
    float is_data = smoothstep(0.05, 0.2, saturation);
    
    // === COLOR PALETTE ===
    vec3 deep_blue = vec3(0.2, 0.5, 1.0);
    vec3 electric_blue = vec3(0.05, 0.15, 1.0);
    
    vec3 shell_base = mix(v_color, deep_blue, 0.2);
    vec3 data_base = mix(v_color, electric_blue, 0.5);
    vec3 base_color = mix(shell_base, data_base, is_data);
    
    // === AMBIENT & CURVATURE ===
    float ao_strength = mix(0.1, 1.0, pow(v_curvature, 0.5));
    
    // === DIFFUSE ===
    float diff = max(dot(N, L), 0.0);
    float diff_strength = mix(0.15, 0.05, is_data);
    vec3 diffuse_color = base_color * diff * ao_strength * diff_strength;
    
    // === SPECULAR ===
    vec3 H = normalize(L + V);
    float spec_intensity = 0.5;
    float shininess = 64.0;
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular_color = vec3(1.0) * spec * spec_intensity;
    
    // === RIM LIGHT (Fresnel) ===
    float fresnel_power = 3.0;
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), fresnel_power);
    
    // Shell Rim
    vec3 shell_rim = deep_blue * fresnel * 2.0;
    
    // Data Rim
    vec3 data_rim_color = mix(v_color, electric_blue, 0.5);
    float data_rim_fresnel = pow(1.0 - max(dot(N, V), 0.0), 5.0);
    vec3 data_rim = data_rim_color * data_rim_fresnel * 8.0;
    
    vec3 rim_color = mix(shell_rim, data_rim, is_data);
    
    // === EMISSIVE & CORE ===
    float N_dot_V = max(dot(N, V), 0.0);
    
    // Shell Emission
    vec3 shell_emit = deep_blue * fresnel * 0.4;
    
    // Data Emission
    float core_metric = pow(N_dot_V, 3.0);
    vec3 data_core_color = mix(data_base, vec3(1.0), core_metric);
    vec3 data_emit = data_core_color * (0.8 + core_metric * 4.0);
    
    vec3 emissive = mix(shell_emit, data_emit, is_data);
    
    // === COMBINE ===
    vec3 final_color = diffuse_color + specular_color + rim_color + emissive;
    
    // === ALPHA / TRANSPARENCY ===
    // Increased minimum alpha to make transparency more visible from all angles
    // Shell: More transparent overall for better glass effect
    float alpha_shell = clamp(0.3 + 0.6 * fresnel, 0.0, 0.9);
    
    // Data: Increased base alpha for better visibility
    float alpha_data = clamp(0.3 + 0.4 * core_metric, 0.0, 0.7);
    
    float final_alpha = mix(alpha_shell, alpha_data, is_data);
    
    fragColor = vec4(final_color, final_alpha);
}
