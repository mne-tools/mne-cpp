#version 430 core

layout (local_size_x = 1) in;

//buffer with colors
layout (std430, binding = 0) buffer colorArray
{
    vec4 colors[];
};

void main(void)
{
    uint globalId = gl_GlobalInvocationID.x;

    colors[globalId] = vec4(0.0, 1.0, 0.0, 1.0);
}
