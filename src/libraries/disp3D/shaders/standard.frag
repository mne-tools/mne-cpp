#version 440

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec3 v_color;
layout(location = 2) in vec3 v_worldPos;
layout(location = 3) in vec3 v_annotColor;
layout(location = 4) in float v_surfaceId;
layout(location = 5) in float v_curvatureGray;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float isSelected;
    vec3 lightDir;
    float tissueType;  // 0=Unknown, 1=Brain, 2=Skin, 3=OuterSkull, 4=InnerSkull
    float lightingEnabled;
    float overlayMode; // 0=Surface, 1=Annotation, 2=Scientific, 3=SourceEstimate
    float selectedSurfaceId; // WORKAROUND(QRhi-GLES2): merged-draw surface selection
    float _pad2;
};

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(cameraPos - v_worldPos);
    vec3 H = normalize(L + V);
    
    float NdotL = max(dot(N, L), 0.0);
    float diff = NdotL;
    
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, 32.0) * 0.3;
    
    // Select base color based on overlayMode uniform.
    // WORKAROUND(QRhi-GLES2): In the merged-draw path, non-brain surfaces
    // (BEM, sensors, field maps) get surfaceId >= 100.  These always use
    // their vertex colour directly so the field-map colours are visible
    // regardless of the brain's overlayMode.
    vec3 baseColor;
    if (v_surfaceId >= 99.5) {
        baseColor = v_color;
    } else if (overlayMode < 0.5) {
        // Surface mode: neutral curvature grey, independent from STC RGB.
        baseColor = vec3(v_curvatureGray);
    } else if (overlayMode < 1.5) {
        // Annotation mode: use annotation colour channel
        // Fall back to warm white when no annotation is loaded (black)
        baseColor = (dot(v_annotColor, v_annotColor) > 0.001)
                  ? v_annotColor
                  : vec3(1.0, 0.97, 0.94);
    } else if (overlayMode < 2.5) {
        // Scientific mode: curvature gray from primary colour
        baseColor = v_color;
    } else {
        // Source Estimate: primary colour (dynamically updated STC)
        baseColor = v_color;
    }
    
    // Richer brain tissue tones
    vec3 warmTint = vec3(1.0, 0.97, 0.94);
    baseColor *= warmTint;
    
    float ambient = 0.35;
    float diffuseStrength = 0.65;
    
    vec3 fillLightDir = normalize(vec3(0.0, -0.5, 0.3));
    float fillDiff = max(dot(N, fillLightDir), 0.0) * 0.15;
    
    vec3 finalColor = baseColor * (ambient + diff * diffuseStrength + fillDiff);
    finalColor += vec3(0.95, 0.95, 1.0) * spec;
    
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    // Selection glow
    // WORKAROUND(QRhi-GLES2): On WASM merged-draw path, isSelected is 0;
    // instead selectedSurfaceId >= 0 and v_surfaceId identifies the surface.
    bool highlighted = (isSelected > 0.5)
                    || (selectedSurfaceId >= 0.0 && abs(v_surfaceId - selectedSurfaceId) < 0.5);
    if (highlighted) {
        float fresnel = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 2.5);
        vec3 glowColor = vec3(1.0, 0.85, 0.3);
        finalColor = mix(finalColor, glowColor, 0.25);
        finalColor += glowColor * fresnel * 0.6;
    }
    
    fragColor = vec4(finalColor, 1.0);
}
