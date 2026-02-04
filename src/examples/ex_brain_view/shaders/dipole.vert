#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

// Instanced data
// mat4 takes 4 locations
layout(location = 2) in vec4 modelRow0;
layout(location = 3) in vec4 modelRow1;
layout(location = 4) in vec4 modelRow2;
layout(location = 5) in vec4 modelRow3;
layout(location = 6) in vec4 color;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec4 v_color;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 cameraPos;
    vec3 lightDir;
    float lightingEnabled;
} ubuf;

void main()
{
    mat4 model = mat4(modelRow0, modelRow1, modelRow2, modelRow3);
    vec4 pos = model * vec4(position, 1.0);
    v_position = pos.xyz;
    v_normal = mat3(model) * normal;
    v_color = color;
    gl_Position = ubuf.mvp * pos;
}
