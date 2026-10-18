#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in vec4 annotColor;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_color;
layout(location = 3) out vec3 v_viewDir;
layout(location = 4) out float v_curvature;
layout(location = 5) out vec3 v_annotColor;
layout(location = 6) out float v_highlight;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float isSelected;
    vec3 lightDir;
    float tissueType;  // 0=Unknown, 1=Brain, 2=Skin, 3=OuterSkull, 4=InnerSkull
    float lightingEnabled;
    float overlayMode; // 0=Surface, 1=Annotation, 2=Scientific, 3=SourceEstimate
    float selectedLabelActive;
    float _pad2;
    vec4 selectedLabel; // annotation key of the highlighted region or element
};

void main() {
    v_worldPos = position;
    v_normal = normalize(normal);
    v_curvature = color.a;
    v_color = color.rgb;
    v_annotColor = annotColor.rgb;
    v_viewDir = normalize(cameraPos - position);
    v_highlight = (selectedLabelActive > 0.5
                   && all(lessThan(abs(annotColor - selectedLabel), vec4(0.5 / 255.0)))) ? 1.0 : 0.0;
    gl_Position = mvp * vec4(position, 1.0);
}
