#version 440

//=============================================================================================================
// Fragment shader for ChannelDataView GPU signal rendering.
// Simply outputs the per-channel solid colour from the uniform block.
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
};

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = u_color;
}
