#version 440

// Video overlay: circular cut-out fragment shader. Samples the live video
// texture inside a unit disk and draws a thin coloured ring at the rim so
// the overlay reads as a microscope iris on top of the 3-D head surface.

layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in vec2 v_localCoord;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D videoTex;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec4 borderColor;
    float opacity;
    float _pad0;
    float _pad1;
    float _pad2;
};

void main() {
    float r = length(v_localCoord);
    if (r > 1.0) discard;

    vec3 videoRgb = texture(videoTex, v_texCoord).rgb;

    // Soft ring at the rim (r in [0.92, 1.0])
    float ringT = smoothstep(0.92, 1.0, r);
    vec3 col = mix(videoRgb, borderColor.rgb, ringT * borderColor.a);

    // Soft alpha falloff at the very edge to avoid aliasing
    float edgeAlpha = 1.0 - smoothstep(0.97, 1.0, r);

    fragColor = vec4(col, opacity * edgeAlpha);
}
