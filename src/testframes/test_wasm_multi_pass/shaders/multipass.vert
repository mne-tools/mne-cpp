#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 v_color;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
};

void main() {
    v_color = inColor;
    gl_Position = mvp * vec4(position, 1.0);
}
