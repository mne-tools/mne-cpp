#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in vec4 annotColor;
layout(location = 4) in float surfaceId;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_normal;

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

void main() {
    vec3 n = normalize(normal);
    vec3 basePos = position + n * axisVAndOffset.w; // base normal offset

    // Pass the ORIGINAL (undisplaced) world position to the fragment
    // shader so that decal UV projection and depth culling remain correct.
    v_worldPos = basePos;
    v_normal = n;

    vec3 pos = basePos;

    // ── Vertex displacement from depth map ──────────────────────────
    // Project vertex into decal UV space and sample depth.  Vertices
    // inside the decal footprint are pushed inward (into the head)
    // proportionally to the depth value — far regions sink deepest,
    // near regions stay near the surface, creating a relief cavity.
    float depthEnabled = depthParams.z;
    if (depthEnabled > 0.5) {
        vec3 d = position - focusAndSize.xyz;
        float side   = max(focusAndSize.w, 0.0001);
        float aspect = max(cameraPosAndFacing.w, 0.0001);

        vec2 local = vec2(dot(d, axisUAndOpacity.xyz) / side,
                          dot(d, axisVAndOffset.xyz) / (side / aspect));

        // Smooth edge falloff to avoid hard geometry seams at decal border
        float edgeDist = max(abs(local.x), abs(local.y));
        float inDecal = smoothstep(0.50, 0.38, edgeDist);

        if (inDecal > 0.001) {
            vec2 uv = mix(vec2(0.055), vec2(0.945), local + vec2(0.5));
            uv.y = 1.0 - uv.y;

            float depthVal = textureLod(depthTex, uv, 3.0).r; // LOD 3 = blurred for smooth displacement
            depthVal = 1.0 - depthVal;                          // invert: 0=near(flush), 1=far(deepest)

            // Scale displacement relative to decal size for resolution independence.
            float maxDisp = depthParams.x * side * 0.15;
            float displacement = depthVal * maxDisp * inDecal;

            // Push inward (negative normal = into the head)
            pos -= n * displacement;
        }
    }

    gl_Position = mvp * vec4(pos, 1.0);
}
