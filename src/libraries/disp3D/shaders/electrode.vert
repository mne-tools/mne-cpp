#version 440

// sEEG / ECoG electrodes: instanced unit spheres (contacts) and unit cylinders (lead bodies),
// with the per-instance layout shared with the dipole pipeline

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 2) in vec4 modelRow0;
layout(location = 3) in vec4 modelRow1;
layout(location = 4) in vec4 modelRow2;
layout(location = 5) in vec4 modelRow3;
layout(location = 6) in vec4 color;
layout(location = 7) in float isSelected;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_color;
layout(location = 3) out float v_selected;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 cameraPos;
    vec3 lightDir;
    float lightingEnabled;
} ubuf;

void main()
{
    mat4 model = mat4(modelRow0, modelRow1, modelRow2, modelRow3);
    vec4 world = model * vec4(position, 1.0);
    v_worldPos = world.xyz;
    // Exact for these meshes: cylinders scale x and z alike, and their normals lie in the xz-plane or along y
    v_normal = mat3(model) * normal;
    v_color = color.rgb;
    v_selected = isSelected;
    gl_Position = ubuf.mvp * world;
}
