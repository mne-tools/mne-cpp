#version 440

//=============================================================================================================
// Full-screen quad fragment shader for compositing the overlay image (bands + event lines).
// Outputs the sampled RGBA colour; the pipeline uses src-alpha blending so transparent
// pixels in the overlay image leave the underlying waveform render untouched.
//=============================================================================================================

layout(location = 0) in vec2 v_uv;

layout(binding = 1) uniform sampler2D u_tex;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = texture(u_tex, v_uv);
}
