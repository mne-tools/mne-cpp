#version 440

// Slice vertex shader: full-screen quad textured with an MRI volume slice

layout(location = 0) in vec3 position;     // Quad corners in 3-D (slice plane)
layout(location = 1) in vec2 texCoord;     // UV coordinates into the slice texture

layout(location = 0) out vec2 v_texCoord;
layout(location = 1) out vec3 v_worldPos;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    mat4 sliceToWorld;   // Transforms unit-square slice quad into world coordinates
    float opacity;
    float windowCenter;  // Intensity windowing center (0-1 normalised)
    float windowWidth;   // Intensity windowing width  (0-1 normalised)
    float _pad0;
};

void main() {
    vec4 worldPos = sliceToWorld * vec4(position, 1.0);
    v_worldPos = worldPos.xyz;
    v_texCoord = texCoord;
    gl_Position = mvp * worldPos;
}
