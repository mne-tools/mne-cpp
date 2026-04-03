#version 440

//=============================================================================================================
// Vertex shader for ChannelDataView GPU signal rendering.
//
// Vertex format: vec2(x_sampleOffset, y_amplitude)
//   x_sampleOffset  — sample index relative to u_firstSampleInVbo
//   y_amplitude     — raw amplitude in physical units (same units as u_amplitudeMax)
//
// For raw data:    vertex k → (k, data[k])
// For min/max decimated data: vertex pairs → (colOffset, maxAmp), (colOffset, minAmp)
//=============================================================================================================

layout(location = 0) in vec2 a_vertex;   // (x_sampleOffset, y_amplitude)

layout(std140, binding = 0) uniform ChannelBlock {
    vec4  u_color;               // RGBA line colour
    float u_firstSampleInVbo;    // absolute sample index of VBO vertex 0
    float u_scrollSample;        // first visible sample (left edge)
    float u_samplesPerPixel;     // zoom: > 1 zoomed out, < 1 zoomed in
    float u_viewWidth;           // viewport width  in logical pixels
    float u_viewHeight;          // viewport height in logical pixels
    float u_channelYCenter;      // NDC y of the channel row centre
    float u_channelYRange;       // NDC height of the full channel row
    float u_amplitudeMax;        // amplitude that maps to ±45 % of the row height
    float u_showClipping;        // > 0.5 to highlight clipped segments in red
};

layout(location = 0) out float v_norm;   // normalised amplitude for clipping detection

void main()
{
    // ── Horizontal mapping ──────────────────────────────────────────────
    float sampleIdx = u_firstSampleInVbo + a_vertex.x;
    float xPixel    = (sampleIdx - u_scrollSample) / max(u_samplesPerPixel, 1e-6);
    float xNDC      = (xPixel / u_viewWidth) * 2.0 - 1.0;

    // ── Vertical mapping ────────────────────────────────────────────────
    // Normalise amplitude; clamp to ±2 so signals clamp inside adjacent rows
    float norm  = (u_amplitudeMax > 0.0)
                      ? clamp(a_vertex.y / u_amplitudeMax, -2.0, 2.0)
                      : 0.0;
    // 45 % of the half-row height per unit norm — leaves 10 % dead-band at edges
    float yNDC  = u_channelYCenter + norm * (u_channelYRange * 0.45);

    v_norm = norm;
    gl_Position = vec4(xNDC, yNDC, 0.0, 1.0);
}
