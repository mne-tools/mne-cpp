#version 430 core

//in vec4 vPosition;
in vec3 vColor;
in vec3 vNormal;

out vec4 color;

void main(void)
{
    //TODO add phong alpha
    color = vec4(vColor.xyz, 1.0);
}
