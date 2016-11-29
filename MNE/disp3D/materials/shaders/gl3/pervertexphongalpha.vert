#version 400 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelViewNormal;
uniform mat4 modelNormalMatrix;

in vec3 vertexPosition;
in vec3 vertexColor;
in vec3 vertexNormal;

out vec4 vPosition;
out vec3 vColor;
out vec3 vNormal;

void main()
{
    vPosition = viewMatrix * modelMatrix * vec4(vertexPosition, 1.);
	vNormal = normalize( vertexNormal );
	
	vColor = vertexColor;
}