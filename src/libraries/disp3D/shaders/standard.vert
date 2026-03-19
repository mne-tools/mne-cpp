#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in vec4 annotColor;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec3 v_color;
layout(location = 2) out vec3 v_worldPos;
layout(location = 3) out vec3 v_annotColor;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float isSelected;
    vec3 lightDir;
    float tissueType;  // 0=Unknown, 1=Brain, 2=Skin, 3=OuterSkull, 4=InnerSkull
    float lightingEnabled;
    float overlayMode; // 0=Surface, 1=Annotation, 2=Scientific, 3=SourceEstimate
    float _pad1;
    float _pad2;
};

void main() {
    v_worldPos = position;
    v_normal = normalize(normal);
    v_color = color.rgb;
    v_annotColor = annotColor.rgb;
    gl_Position = mvp * vec4(position, 1.0);
}
