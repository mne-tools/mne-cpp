#version 440

layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_normal;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D videoTex;
layout(binding = 2) uniform sampler2D depthTex;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec4 focusAndSize;        // xyz = decal centre, w = side length
    vec4 axisUAndOpacity;     // xyz = local U axis, w = opacity
    vec4 axisVAndOffset;      // xyz = local V axis, w = normal offset
    vec4 axisNAndDepth;       // xyz = local normal, w = half-depth
    vec4 cameraPosAndFacing;  // xyz = camera position, w = aspect ratio (w/h)
    vec4 borderColor;
    vec4 depthParams;         // x = displacementScale, y = unused, z = depthEnabled, w = reserved
};

float roundedRectSdf(vec2 p, vec2 halfSize, float radius)
{
    vec2 q = abs(p) - halfSize + vec2(radius);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - radius;
}

vec3 sampleVideoSoft(vec2 uv, float blurAmount)
{
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

// Compute a per-pixel normal from the depth map gradient.  This adds
// fine surface-lighting detail on top of the vertex displacement.
vec3 depthGradientNormal(vec2 uv, float scale)
{
    vec2 texel = vec2(1.0 / 512.0);
    float dl = texture(depthTex, uv + vec2(-texel.x, 0.0)).r;
    float dr = texture(depthTex, uv + vec2( texel.x, 0.0)).r;
    float db = texture(depthTex, uv + vec2(0.0, -texel.y)).r;
    float dt = texture(depthTex, uv + vec2(0.0,  texel.y)).r;
    float dx = (dr - dl) * scale;
    float dy = (dt - db) * scale;
    return normalize(vec3(-dx, -dy, 1.0));
}

void main() {
    vec3 d = v_worldPos - focusAndSize.xyz;

    float side = max(focusAndSize.w, 0.0001);
    float aspect = max(cameraPosAndFacing.w, 0.0001);
    float depth = dot(d, axisNAndDepth.xyz);
    if (abs(depth) > axisNAndDepth.w)
        discard;

    vec2 local = vec2(dot(d, axisUAndOpacity.xyz) / side,
                      dot(d, axisVAndOffset.xyz) / (side / aspect));

    // Soft rounded aperture
    const float cornerRadius = 0.105;
    const float feather = 0.115;
    float sdf = roundedRectSdf(local, vec2(0.5), cornerRadius);
    if (sdf > feather)
        discard;

    float apertureAlpha = 1.0 - smoothstep(-feather, feather, sdf);
    float edgeBand = smoothstep(-feather, feather, sdf);

    // Base UV from decal projection
    vec2 uv = mix(vec2(0.055), vec2(0.945), local + vec2(0.5));
    uv.y = 1.0 - uv.y;

    vec3 videoRgb = sampleVideoSoft(uv, edgeBand);
    videoRgb = pow(max(videoRgb, vec3(0.0)), vec3(0.92));

    // ── Depth-derived lighting ──────────────────────────────────────
    // Vertices are already displaced; this adds subtle per-pixel shading
    // for surface detail without significantly altering the video contrast.
    float depthEnabled = depthParams.z;
    if (depthEnabled > 0.5) {
        float reliefStrength = depthParams.x;

        vec3 depthNorm = depthGradientNormal(uv, reliefStrength * 4.0);
        vec3 axisU = axisUAndOpacity.xyz;
        vec3 axisV = axisVAndOffset.xyz;
        vec3 axisN = axisNAndDepth.xyz;
        vec3 worldNormal = normalize(depthNorm.x * axisU
                                   + depthNorm.y * axisV
                                   + depthNorm.z * axisN);

        // Subtle upper-left light for gentle shadowing
        vec3 lightDir = normalize(cameraPosAndFacing.xyz - v_worldPos
                                  + axisU * 0.3 + axisV * 0.4);
        float ndotl = max(dot(worldNormal, lightDir), 0.0);
        // High ambient (0.75) so lighting barely darkens — just adds subtle relief
        float lighting = 0.75 + 0.25 * ndotl;

        videoRgb *= lighting;
    }

    // Subtle optical falloff
    float lensFalloff = smoothstep(0.58, 0.08, length(local));
    float alpha = axisUAndOpacity.w * apertureAlpha * mix(0.62, 0.96, lensFalloff);

    // Faint cyan sheen at the lip
    float rim = (1.0 - smoothstep(0.0, 0.030, abs(sdf))) * 0.18;
    vec3 col = mix(videoRgb, borderColor.rgb, rim);
    col = mix(col, vec3(dot(col, vec3(0.299, 0.587, 0.114))), edgeBand * 0.10);

    fragColor = vec4(col, alpha);
}
