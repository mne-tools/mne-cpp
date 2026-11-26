#version 440

// Screen overlay: the texture holds premultiplied RGBA, blended with One / OneMinusSrcAlpha.

layout(location = 0) in vec2 v_texCoord;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2D overlayTex;

void main() {
    fragColor = texture(overlayTex, v_texCoord);
}
