#version 440

layout(location = 0) in vec3 v_normal;
layout(location = 0) out vec4 fragColor;

void main() {
    // Map normal components from [-1, 1] to [0, 1] for RGB visualization.
    // X -> Red, Y -> Green, Z -> Blue
    vec3 normalColor = v_normal * 0.5 + 0.5;
    fragColor = vec4(normalColor, 1.0);
}
