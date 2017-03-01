#version 400 core

in vec3 vertexPosition;
in vec3 vertexColor;
in vec3 vertexNormal;

out vec4 vPosition;
out vec3 vColor;
out vec3 vNormal;

void main()
{
    vPosition = vec4(vertexPosition, 1.);
	
	vNormal = normalize( vertexNormal );
	
	vColor = vertexColor;
}
