#version 440

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec4 v_color;
layout(location = 3) in float v_isSelected;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 cameraPos;
    vec3 lightDir;
    float lightingEnabled;
} ubuf;

void main()
{
    vec3 N = normalize(v_normal);
    vec3 V = normalize(ubuf.cameraPos - v_position);
    
    // Fresnel effect (Double-sided)
    float N_dot_V = abs(dot(N, V));
    float fresnel = pow(1.0 - N_dot_V, 2.5);
    
    // Base transparency + Fresnel glow
    float alpha = clamp(0.4 + 0.6 * fresnel, 0.0, 1.0);
    
    // Color calculation
    vec3 color = v_color.rgb * (1.2 + fresnel * 0.8);
    
    // --- Selection Highlight (Silver Rim) ---
    if (v_isSelected > 0.5) {
        float selectionRim = pow(1.0 - N_dot_V, 3.0);
        vec3 whiteColor = vec3(1.0, 1.0, 1.0);
        color += whiteColor * 0.15;
        color += whiteColor * selectionRim * 1.0;
        alpha = clamp(alpha + 0.1 + selectionRim * 0.5, 0.0, 1.0);
    }
    
    // Output Straight Alpha for SrcAlpha/OneMinusSrcAlpha blending
    fragColor = vec4(color, alpha);
}
