#version 440

// Screen overlay: full-target quad carrying the 2-D labels and pane separators
// that are painted on the CPU and blended over the 3-D scene.

layout(location = 0) in vec2 position; // Normalized device coordinates
layout(location = 1) in vec2 texCoord; // UV into the overlay texture, (0,0) = top-left

layout(location = 0) out vec2 v_texCoord;

void main() {
    v_texCoord = texCoord;
    gl_Position = vec4(position, 0.0, 1.0);
}
