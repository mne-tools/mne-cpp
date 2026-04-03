#version 440

//=============================================================================================================
// Fragment shader for ChannelDataView GPU signal rendering.
// Outputs per-channel solid colour, or red for clipped segments when enabled.
//=============================================================================================================

layout(std140, binding = 0) uniform ChannelBlock {
    vec4  u_color;
    float u_firstSampleInVbo;
    float u_scrollSample;
    float u_samplesPerPixel;
    float u_viewWidth;
    float u_viewHeight;
    float u_channelYCenter;
    float u_channelYRange;
    float u_amplitudeMax;
    float u_showClipping;
};

layout(location = 0) in float v_norm;    // interpolated normalised amplitude

layout(location = 0) out vec4 fragColor;

void main()
{
    // Clipping detection: highlight in red where |norm| >= 0.95
    if (u_showClipping > 0.5 && abs(v_norm) >= 0.95)
        fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    else
        fragColor = u_color;
}
