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
    float isSelected;
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
    vec3 shell_blue = vec3(0.0, 0.4, 0.8);
    vec3 data_color = v_color;
    
    // === FRESNEL EFFECT ===
    float N_dot_V = abs(dot(N, V)); // Use abs for double-sided visibility
    float fresnel = pow(1.0 - N_dot_V, 2.5);
    
    // === LIGHTING (Subtle for Hologram) ===
    float diff = max(dot(N, L), 0.0);
    float rim = pow(1.0 - N_dot_V, 3.0);
    
    // === FINAL COLOR CALCULATION ===
    // Shell: Only glow at edges
    vec3 shell_final = shell_blue * (fresnel * 1.5 + rim * 0.5);
    
    // Data: More solid but still with a glow
    vec3 data_final = data_color * (1.0 + fresnel * 2.0);
    
    vec3 base_rgb = mix(shell_final, data_final, is_data);
    
    // === ALPHA CONTROL ===
    // Shell is very transparent in middle, glows at edge
    float alpha_shell = 0.1 + 0.7 * fresnel;
    
    // Data is more prominent
    float alpha_data = 0.4 + 0.5 * fresnel;
    
    float final_alpha = mix(alpha_shell, alpha_data, is_data);
    
    // --- Selection Highlight (Silver Rim) ---
    if (isSelected > 0.5) {
        float selectionRim = pow(1.0 - N_dot_V, 3.0);
        vec3 whiteColor = vec3(1.0, 1.1, 1.2); // Cool White
        base_rgb += whiteColor * 0.2; // Constant boost
        base_rgb += whiteColor * selectionRim * 1.0;
        final_alpha = clamp(final_alpha + 0.1 + selectionRim * 0.5, 0.0, 1.0);
    }
    
    // Modulate RGB by alpha for additive blending
    fragColor = vec4(base_rgb * final_alpha, final_alpha);
}
