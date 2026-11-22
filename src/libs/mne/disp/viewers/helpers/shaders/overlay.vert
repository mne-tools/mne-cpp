#version 440

//=============================================================================================================
// Full-screen quad vertex shader for compositing the overlay image (bands + event lines)
// on top of the waveform render pass.
//
// Vertex layout: vec2 pos (NDC), vec2 uv
//=============================================================================================================

layout(location = 0) in vec2 a_pos;  // NDC position:  (-1,-1) = bottom-left, (1,1) = top-right
layout(location = 1) in vec2 a_uv;   // Texture UV:    ( 0, 0) = top-left,    (1,1) = bottom-right

layout(location = 0) out vec2 v_uv;

void main()
{
    v_uv        = a_uv;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
