#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec3 v_color;
layout(location = 2) out vec3 v_worldPos;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float isSelected;
    vec3 lightDir;
    float _pad2;
    float lightingEnabled;
    vec3 _pad3;
};

void main() {
    v_worldPos = position; // Local is world here as model is identity in vertex stream (passed in matrices)
    // Actually model is part of MVP. But for brain surfaces in this app, 
    // position is already in "brain space".
    
    // Pass normal
    v_normal = normalize(normal);
    
    // Pass color (contains curvature as grayscale)
    v_color = color.rgb;
    
    gl_Position = mvp * vec4(position, 1.0);
}
