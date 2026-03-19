#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in vec4 annotColor;

layout(location = 0) out vec3 v_normal;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float isSelected;
    vec3 lightDir;
    float tissueType;
    float lightingEnabled;
    float overlayMode;
    float _pad1;
    float _pad2;
};

void main() {
    v_normal = normalize(normal);
    gl_Position = mvp * vec4(position, 1.0);
}
