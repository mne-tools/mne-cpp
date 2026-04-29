#version 440

layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_normal;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D videoTex;

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
    vec3 d = v_worldPos - focusAndSize.xyz;
    vec3 viewDir = normalize(cameraPosAndFacing.xyz - v_worldPos);
    bool frontFacing = dot(normalize(v_normal), viewDir) > cameraPosAndFacing.w;

    float side = max(focusAndSize.w, 0.0001);
    float depth = dot(d, axisNAndDepth.xyz);
    if (abs(depth) > axisNAndDepth.w)
        discard;

    vec2 local = vec2(dot(d, axisUAndOpacity.xyz), dot(d, axisVAndOffset.xyz)) / side;

    vec2 a = abs(local);
    if (a.x > 0.5 || a.y > 0.5)
        discard;

    float edgeDist = 0.5 - max(a.x, a.y);

    if (!frontFacing) {
        float border = 1.0 - smoothstep(0.0, 0.012, edgeDist);
        float cross = 1.0 - smoothstep(0.0, 0.006, min(abs(local.x), abs(local.y)));
        float marker = max(border, cross * smoothstep(0.22, 0.0, max(abs(local.x), abs(local.y))));
        if (marker <= 0.01)
            discard;
        fragColor = vec4(borderColor.rgb, 0.55 * marker);
        return;
    }

    vec2 uv = local + vec2(0.5);
    uv.y = 1.0 - uv.y;

    vec3 videoRgb = texture(videoTex, uv).rgb;

    float edgeAlpha = smoothstep(0.0, 0.015, edgeDist);
    float border = 1.0 - smoothstep(0.0, 0.012, edgeDist);
    vec3 col = mix(videoRgb, borderColor.rgb, border * borderColor.a);

    fragColor = vec4(col, axisUAndOpacity.w * edgeAlpha);
}
