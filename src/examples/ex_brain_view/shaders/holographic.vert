#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_color;
layout(location = 3) out vec3 v_viewDir;
layout(location = 4) out float v_curvature;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float isSelected;
    vec3 lightDir;
    float tissueType;  // 0=Unknown, 1=Brain, 2=Skin, 3=OuterSkull, 4=InnerSkull
    float lightingEnabled;
    vec3 _pad3;
};

void main() {
    // World position
    v_worldPos = position;
    
    // Use actual vertex normal (not calculated from position!)
    v_normal = normalize(normal);
    
    // Extract curvature from color (grayscale value)
    v_curvature = color.r;
    
    // Pass color
    v_color = color.rgb;
    
    // View direction (from vertex to camera)
    v_viewDir = normalize(cameraPos - position);
    
    gl_Position = mvp * vec4(position, 1.0);
}
