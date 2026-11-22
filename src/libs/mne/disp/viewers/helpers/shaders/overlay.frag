#version 440

//=============================================================================================================
// Full-screen quad fragment shader for compositing the overlay image (bands + event lines).
// Outputs the sampled RGBA colour; the pipeline uses src-alpha blending so transparent
// pixels in the overlay image leave the underlying waveform render untouched.
//
// Alternating per-second bands are computed per-pixel in the shader so the overlay
// texture only needs rebuilding when annotations / events actually change — not on
// every scroll.
//=============================================================================================================

layout(location = 0) in vec2 v_uv;

layout(binding = 1) uniform sampler2D u_tex;

// Per-frame scroll / grid parameters uploaded via UBO binding 2.
layout(std140, binding = 2) uniform OverlayParams {
    float u_scrollSample;      // first visible sample (left edge)
    float u_samplesPerPixel;   // zoom: > 1 zoomed out, < 1 zoomed in
    float u_viewWidth;         // viewport width in logical pixels
    float u_sfreq;             // sampling frequency (Hz)
    float u_firstFileSample;   // absolute first sample in the file
    float u_gridEnabled;       // > 0.5 to enable alternating bands
    float u_overlayFirstSample;// first sample covered by the overlay texture
    float u_overlayTotalSamples;// total sample range covered by the overlay texture
};

layout(location = 0) out vec4 fragColor;

void main()
{
    // ── Sample the overlay texture (annotations, events, epoch markers) ─
    // Map screen UV to the wider overlay texture UV
    float screenSample = u_scrollSample + v_uv.x * u_viewWidth * u_samplesPerPixel;
    float overlayU = (screenSample - u_overlayFirstSample) / max(u_overlayTotalSamples, 1.0);
    overlayU = clamp(overlayU, 0.0, 1.0);
    fragColor = texture(u_tex, vec2(overlayU, v_uv.y));

    // ── Alternating per-second bands (computed per-pixel) ───────────────
    if (u_gridEnabled > 0.5 && u_sfreq > 0.0) {
        float secondIdx = floor((screenSample - u_firstFileSample) / u_sfreq);
        bool oddSecond = (mod(secondIdx, 2.0) > 0.5);
        if (oddSecond) {
            // 8 % opacity dark tint on odd seconds — matches the QPainter alpha(20)
            fragColor = vec4(fragColor.rgb * (1.0 - 0.08) + vec3(0.0) * 0.08,
                             max(fragColor.a, 0.08));
        }
    }
}
