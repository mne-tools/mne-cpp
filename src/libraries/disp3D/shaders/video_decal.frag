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

float roundedRectSdf(vec2 p, vec2 halfSize, float radius)
{
    vec2 q = abs(p) - halfSize + vec2(radius);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - radius;
}

vec3 sampleVideoSoft(vec2 uv, float blurAmount)
{
    // Keep this independent from textureSize(): Qt also bakes an ESSL 100
    // variant for broad backend compatibility, where textureSize is absent.
    vec2 radius = vec2(0.0016, 0.0024) * mix(1.0, 5.5, blurAmount);

    vec3 col = texture(videoTex, uv).rgb * 0.34;
    col += texture(videoTex, uv + vec2( radius.x, 0.0)).rgb * 0.11;
    col += texture(videoTex, uv + vec2(-radius.x, 0.0)).rgb * 0.11;
    col += texture(videoTex, uv + vec2(0.0,  radius.y)).rgb * 0.11;
    col += texture(videoTex, uv + vec2(0.0, -radius.y)).rgb * 0.11;
    col += texture(videoTex, uv + vec2( radius.x,  radius.y)).rgb * 0.055;
    col += texture(videoTex, uv + vec2(-radius.x,  radius.y)).rgb * 0.055;
    col += texture(videoTex, uv + vec2( radius.x, -radius.y)).rgb * 0.055;
    col += texture(videoTex, uv + vec2(-radius.x, -radius.y)).rgb * 0.055;
    return col;
}

void main() {
    vec3 d = v_worldPos - focusAndSize.xyz;

    float side = max(focusAndSize.w, 0.0001);
    float depth = dot(d, axisNAndDepth.xyz);
    if (abs(depth) > axisNAndDepth.w)
        discard;

    vec2 local = vec2(dot(d, axisUAndOpacity.xyz), dot(d, axisVAndOffset.xyz)) / side;

    // Soft rounded aperture. Keep the projection square enough to read as a
    // microscope viewport, but remove the hard decal edge so the video feels
    // embedded in the surface instead of pasted on top of it.
    const float cornerRadius = 0.105;
    const float feather = 0.115;
    float sdf = roundedRectSdf(local, vec2(0.5), cornerRadius);
    if (sdf > feather)
        discard;

    float apertureAlpha = 1.0 - smoothstep(-feather, feather, sdf);
    float edgeBand = smoothstep(-feather, feather, sdf);

    // Crop a little from the capture frame. Many microscope/capture feeds have
    // black pillarbox strips or instrument frame edges; pulling the UVs inward
    // keeps those from becoming a hard visible rectangle in the 3-D scene.
    vec2 uv = mix(vec2(0.055), vec2(0.945), local + vec2(0.5));
    uv.y = 1.0 - uv.y;

    vec3 videoRgb = sampleVideoSoft(uv, edgeBand);
    videoRgb = pow(max(videoRgb, vec3(0.0)), vec3(0.92)); // mild lift for surgical-light video

    // Subtle optical falloff: centre remains crisp and readable, while the
    // sides dissolve into the rendered anatomy.
    float lensFalloff = smoothstep(0.58, 0.08, length(local));
    float alpha = axisUAndOpacity.w * apertureAlpha * mix(0.62, 0.96, lensFalloff);

    // Only a very faint cyan sheen at the lip; no hard border on the front side.
    float rim = (1.0 - smoothstep(0.0, 0.030, abs(sdf))) * 0.18;
    vec3 col = mix(videoRgb, borderColor.rgb, rim);
    col = mix(col, vec3(dot(col, vec3(0.299, 0.587, 0.114))), edgeBand * 0.10);

    fragColor = vec4(col, alpha);
}
