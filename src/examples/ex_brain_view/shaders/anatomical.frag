#version 440

layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_color;
layout(location = 3) in vec3 v_viewDir;
layout(location = 4) in float v_curvature;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform UniformBlock {
    mat4 mvp;
    vec3 cameraPos;
    float isSelected;
    vec3 lightDir;
    float tissueType;  // 0=Unknown, 1=Brain, 2=Skin, 3=OuterSkull, 4=InnerSkull
    float lightingEnabled;
    vec3 _pad3;
};

// Anatomically realistic tissue colors
vec3 getBrainColor(float curvature) {
    // Brain tissue: pinkish-gray with gyri/sulci variation
    // Gyri (convex, curvature <= 0): lighter pink-gray
    // Sulci (concave, curvature > 0): darker reddish-brown
    vec3 gyrusColor = vec3(0.85, 0.75, 0.72);   // Light pinkish-gray
    vec3 sulcusColor = vec3(0.55, 0.40, 0.38);  // Darker reddish-brown
    
    // Smooth blend based on curvature
    float t = clamp(curvature * 2.0 + 0.5, 0.0, 1.0);
    return mix(gyrusColor, sulcusColor, t);
}

vec3 getSkinColor() {
    // Realistic Caucasian skin tone
    return vec3(0.92, 0.78, 0.68);  // Peachy skin
}

vec3 getSkullColor(float isInner) {
    // Skull bone: ivory/cream with slight yellowish tint
    vec3 outerSkull = vec3(0.95, 0.90, 0.82);  // Ivory/cream outer skull
    vec3 innerSkull = vec3(0.88, 0.82, 0.75);  // Slightly darker inner skull
    return mix(outerSkull, innerSkull, isInner);
}

// Detect if a vertex color indicates selection (gold-tinted, not grayscale)
// Only valid for brain tissue where base colors are grayscale
float detectBrainSelection(vec3 color, float curvatureVal) {
    // Brain vertices start as grayscale (R ≈ G ≈ B from curvature)
    // Selected vertices are blended towards gold (R > G > B)
    
    // Check if there's a warm tint added to grayscale
    float warmth = color.r - color.b;  // Positive for warm (gold) colors
    
    // For grayscale, R-B should be near 0. Gold blend adds warmth.
    // Threshold tuned for 40% gold blend from brainsurface.cpp
    // Gold = (255, 200, 80) normalized ≈ (1.0, 0.78, 0.31)
    // Blended: R increases, B stays lower
    
    float selectionStrength = smoothstep(0.08, 0.20, warmth);
    
    return selectionStrength;
}

void main() {
    // Normalize interpolated vectors
    vec3 N = normalize(v_normal);
    vec3 V = normalize(v_viewDir);
    vec3 L = normalize(lightDir);
    vec3 H = normalize(L + V);  // Half-vector for specular
    
    int tissue = int(tissueType + 0.5);  // Round to nearest int
    
    // Detect per-vertex selection ONLY for brain tissue
    // (Skin/skull have inherent warm colors that would false-positive)
    float vertexSelected = 0.0;
    if (tissue == 1) {
        vertexSelected = detectBrainSelection(v_color, v_curvature);
    }
    
    // Combine with uniform selection (whole surface selected)
    float effectiveSelection = max(isSelected, vertexSelected);
    
    // Determine base color based on tissue type
    vec3 baseColor;
    float roughness = 0.5;
    float specIntensity = 0.3;
    
    if (tissue == 1) {
        // Brain
        baseColor = getBrainColor(v_curvature);
        roughness = 0.7;       // Matte brain surface
        specIntensity = 0.15;  // Subtle wet sheen
    } else if (tissue == 2) {
        // Skin
        baseColor = getSkinColor();
        roughness = 0.4;       // Slightly glossy skin
        specIntensity = 0.25;  // Skin has some sheen
        
        // Subsurface scattering approximation
        float NdotL_wrap = max(dot(N, L) * 0.5 + 0.5, 0.0);
        vec3 sss = vec3(1.0, 0.4, 0.3) * NdotL_wrap * 0.15;  // Reddish subsurface
        baseColor += sss;
    } else if (tissue == 3) {
        // Outer Skull
        baseColor = getSkullColor(0.0);
        roughness = 0.6;       // Slightly rough bone
        specIntensity = 0.2;
    } else if (tissue == 4) {
        // Inner Skull
        baseColor = getSkullColor(1.0);
        roughness = 0.65;
        specIntensity = 0.15;
    } else {
        // Unknown: use vertex color or default gray
        baseColor = v_color;
        roughness = 0.5;
        specIntensity = 0.3;
    }
    
    // === LIGHTING ===
    // Ambient occlusion based on curvature
    float ao = mix(0.7, 1.0, clamp(1.0 - v_curvature * 0.5, 0.0, 1.0));
    
    // Ambient
    vec3 ambient = baseColor * 0.3 * ao;
    
    // Diffuse (Lambertian)
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = baseColor * NdotL * 0.6;
    
    // Specular (Blinn-Phong with roughness-based power)
    float specPower = mix(8.0, 64.0, 1.0 - roughness);
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, specPower);
    vec3 specular = vec3(0.95, 0.95, 1.0) * spec * specIntensity;  // Cool-white highlight
    
    // Fill light from below to soften shadows
    vec3 fillDir = normalize(vec3(0.2, -0.6, 0.3));
    float fillDiff = max(dot(N, fillDir), 0.0) * 0.12;
    
    // Combine
    vec3 finalColor = ambient + diffuse + specular + baseColor * fillDiff;
    
    // === SELECTION HIGHLIGHT ===
    // Anatomically-inspired selection: increased blood flow / hyperemia effect
    // Makes selected brain region appear slightly flushed/reddened - realistic!
    if (effectiveSelection > 0.1) {
        // Calculate fresnel for subtle rim
        float NdotV = clamp(dot(N, V), 0.0, 1.0);
        float fresnel = pow(1.0 - NdotV, 2.5);
        
        // Anatomically-inspired colors: blood/hyperemia tint
        vec3 bloodTint = vec3(0.95, 0.55, 0.50);   // Reddish-pink (increased blood)
        vec3 rimColor = vec3(0.90, 0.40, 0.35);    // Deeper red for edges
        
        float sel = effectiveSelection;
        
        // 1. Subtle shift towards blood-flushed appearance (hyperemia)
        finalColor = mix(finalColor, bloodTint * 0.9, 0.25 * sel);
        
        // 2. Slight brightness increase (tissue appears more "alive")
        finalColor += vec3(0.08, 0.03, 0.02) * sel;
        
        // 3. Gentle fresnel rim (subtle, not glowing)
        finalColor += rimColor * fresnel * 0.3 * sel;
        
        // 4. Increased specular wetness (looks more vital)
        float wetSpec = pow(NdotH, 32.0) * 0.15 * sel;
        finalColor += vec3(0.95, 0.90, 0.90) * wetSpec;
    }
    
    // Clamp and output
    fragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
}
