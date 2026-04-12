#version 440

// Electrode fragment shader: Blinn-Phong with selection highlight and scalar overlay

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec4 v_color;
layout(location = 2) in vec3 v_worldPos;
layout(location = 3) in float v_selected;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float isSelected;
    vec3 lightDir;
    float useInstancing;
    float opacity;
    float _pad0;
    float _pad1;
    float _pad2;
};

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(cameraPos - v_worldPos);
    vec3 H = normalize(L + V);

    // Lighting
    float ambient = 0.15;
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 64.0) * 0.4;

    vec3 baseColor = v_color.rgb;

    // Selection highlight: bright yellow outline
    if (v_selected > 0.5) {
        baseColor = mix(baseColor, vec3(1.0, 1.0, 0.2), 0.5);
        spec += 0.3;
    }

    vec3 litColor = baseColor * (ambient + diff) + vec3(spec);
    fragColor = vec4(litColor, opacity * v_color.a);
}
