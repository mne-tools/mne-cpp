#version 440

// Opaque Blinn-Phong electrodes; hovered or selected contacts turn gold

layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_color;
layout(location = 3) in float v_selected;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 cameraPos;
    vec3 lightDir;
    float lightingEnabled;
} ubuf;

void main()
{
    vec3 base = v_selected > 0.5 ? vec3(1.0, 0.78, 0.24) : v_color;
    if (ubuf.lightingEnabled < 0.5) {
        fragColor = vec4(base, 1.0);
        return;
    }
    vec3 N = normalize(v_normal);
    vec3 V = normalize(ubuf.cameraPos - v_worldPos);
    vec3 L = normalize(ubuf.lightDir);
    // Light both faces, so a lead seen from behind the light is not black
    float diffuse = abs(dot(N, L));
    float specular = pow(max(dot(N, normalize(L + V)), 0.0), 48.0) * 0.35;
    fragColor = vec4(base * (0.3 + 0.7 * diffuse) + vec3(specular), 1.0);
}
