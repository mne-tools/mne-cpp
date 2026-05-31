#version 440

layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_color;
layout(location = 3) in vec3 v_viewDir;
layout(location = 4) in float v_curvature;
layout(location = 5) in vec3 v_annotColor;
layout(location = 6) in float v_surfaceId;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float isSelected;
    vec3 lightDir;
    float tissueType;  // 0=Unknown, 1=Brain, 2=Skin, 3=OuterSkull, 4=InnerSkull
    float lightingEnabled;
    float overlayMode; // 0=Surface, 1=Annotation, 2=Scientific, 3=SourceEstimate
    float selectedSurfaceId; // WORKAROUND(QRhi-GLES2): merged-draw surface selection
    float _pad2;
};

void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(v_viewDir);
    
    // Select effective vertex colour from overlayMode.
    // WORKAROUND(QRhi-GLES2): In the merged-draw path, non-brain surfaces
    // (BEM, sensors, field maps) get surfaceId >= 100.  Always use their
    // vertex colour so field-map colours are visible.
    vec3 effectiveColor;
    if (v_surfaceId >= 99.5) {
        effectiveColor = v_color;
    } else if (overlayMode < 0.5) {
        effectiveColor = vec3(v_curvature); // Surface: neutral curvature grey
    } else if (overlayMode < 1.5) {
        // Annotation: fall back to white when no annotation loaded (black)
        effectiveColor = (dot(v_annotColor, v_annotColor) > 0.001)
                       ? v_annotColor
                       : vec3(1.0);
    } else {
        effectiveColor = v_color; // Scientific / STC
    }

    // === DATA DETECTION (Saturation) ===
    float gray_val = dot(effectiveColor, vec3(0.299, 0.587, 0.114));
    vec3 diff = effectiveColor - vec3(gray_val);
    float satSq = dot(diff, diff);
    float is_data = smoothstep(0.0025, 0.04, satSq);
    
    // === COLOR PALETTE ===
    vec3 shell_blue = vec3(0.0, 0.4, 0.8);
    vec3 data_color = effectiveColor;
    
    // === FRESNEL EFFECT ===
    float N_dot_V = abs(dot(N, V)); // Use abs for double-sided visibility
    float fresnel = pow(1.0 - N_dot_V, 2.5);
    
    // === EDGE GLOW ===
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
    // WORKAROUND(QRhi-GLES2): On WASM merged-draw path, isSelected is 0;
    // instead selectedSurfaceId >= 0 and v_surfaceId identifies the surface.
    bool highlighted = (isSelected > 0.5)
                    || (selectedSurfaceId >= 0.0 && abs(v_surfaceId - selectedSurfaceId) < 0.5);
    if (highlighted) {
        float selectionRim = pow(1.0 - N_dot_V, 3.0);
        vec3 whiteColor = vec3(1.0, 1.1, 1.2); // Cool White
        base_rgb += whiteColor * 0.2; // Constant boost
        base_rgb += whiteColor * selectionRim * 1.0;
        final_alpha = clamp(final_alpha + 0.1 + selectionRim * 0.5, 0.0, 1.0);
    }
    
    // Modulate RGB by alpha for additive blending
    fragColor = vec4(base_rgb * final_alpha, final_alpha);
}
