#version 440

// Slice fragment shader: MRI slice rendering with intensity windowing
// and background discard for clean compositing with the holographic brain.

layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in vec3 v_worldPos;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D sliceTexture;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    mat4 sliceToWorld;
    float opacity;
    float windowCenter;
    float windowWidth;
    float _pad0;
};

void main() {
    float intensity = texture(sliceTexture, v_texCoord).r;

    // Apply intensity windowing (CT/MR convention)
    float lo = windowCenter - windowWidth * 0.5;
    float hi = windowCenter + windowWidth * 0.5;
    float mapped = clamp((intensity - lo) / max(hi - lo, 0.001), 0.0, 1.0);

    // Discard near-black background voxels so the holographic brain
    // (and other transparent geometry) is not occluded by empty space.
    if (mapped < 0.05)
        discard;

    // Modulate alpha by intensity: brighter anatomy is more visible,
    // darker regions fade out for a smooth transition to the brain surface.
    // The wide smoothstep range keeps even bright anatomy semi-transparent
    // so the holographic brain's additive glow shines through.
    float intensityAlpha = smoothstep(0.05, 0.8, mapped) * 0.7;
    float alpha = opacity * intensityAlpha;

    fragColor = vec4(vec3(mapped), alpha);
}
