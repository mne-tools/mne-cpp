#version 440

// Electrode vertex shader: instanced contact spheres + shaft cylinders
// Per-vertex: position, normal
// Per-instance (contacts only): contact position, radius, RGBA colour, selected flag

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

// Instance attributes (only used for contact sphere instancing)
layout(location = 2) in vec3  instPos;
layout(location = 3) in float instRadius;
layout(location = 4) in vec4  instColor;
layout(location = 5) in float instSelected;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec4 v_color;
layout(location = 2) out vec3 v_worldPos;
layout(location = 3) out float v_selected;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float isSelected;
    vec3 lightDir;
    float useInstancing; // 1.0 = contact spheres (instanced), 0.0 = shaft cylinders
    float opacity;
    float _pad0;
    float _pad1;
    float _pad2;
};

void main() {
    vec3 worldPos;

    if (useInstancing > 0.5) {
        // Contact sphere: scale unit sphere by radius and translate to contact position
        worldPos = instPos + position * instRadius;
        v_color = instColor;
        v_selected = instSelected;
    } else {
        // Shaft cylinder: position is pre-transformed
        worldPos = position;
        v_color = vec4(0.6, 0.6, 0.6, 1.0); // Default shaft grey
        v_selected = 0.0;
    }

    v_worldPos = worldPos;
    v_normal = normalize(normal);
    gl_Position = mvp * vec4(worldPos, 1.0);
}
