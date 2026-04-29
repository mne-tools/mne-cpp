#version 440

// Video overlay: textured camera-facing quad rendered on top of the scene
// to show a live RGB/video feed at a focus point.

layout(location = 0) in vec3 position;     // World-space corner position (CPU billboard)
layout(location = 1) in vec2 texCoord;     // UV into the video texture

layout(location = 0) out vec2 v_texCoord;
layout(location = 1) out vec2 v_localCoord; // [-1,1] disk coordinates

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec4 borderColor;
    float opacity;
    float _pad0;
    float _pad1;
    float _pad2;
};

void main() {
    v_texCoord = texCoord;
    v_localCoord = texCoord * 2.0 - 1.0;
    gl_Position = mvp * vec4(position, 1.0);
}
