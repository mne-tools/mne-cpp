#version 430 core

in vec3 vertexPositon;

uniform mat4 mvp;

void main(void)
{
    gl_Position = mvp * pos;

}
