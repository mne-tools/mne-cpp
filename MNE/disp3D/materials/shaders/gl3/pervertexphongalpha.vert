#version 400 core

in vec3 vertexPosition;
in vec3 vertexColor;

out vec3 vPosition;
out vec3 vColor;

void main()
{
    vPosition = vertexPosition;
	vColor = vertexColor;
}