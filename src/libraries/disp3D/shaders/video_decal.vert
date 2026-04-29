#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in vec4 annotColor;
layout(location = 4) in float surfaceId;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_normal;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec4 focusAndSize;      // xyz = decal centre, w = side length
    vec4 axisUAndOpacity;   // xyz = local U axis, w = opacity
    vec4 axisVAndOffset;    // xyz = local V axis, w = normal offset
    vec4 axisNAndDepth;     // xyz = local normal, w = half-depth
    vec4 cameraPosAndFacing; // xyz = camera position, w = min facing dot
    vec4 borderColor;
};

void main() {
    v_worldPos = position;
    v_normal = normalize(normal);
    gl_Position = mvp * vec4(position + normalize(normal) * axisVAndOffset.w, 1.0);
}
