#version 150 core

in vec3 vertexPosition;
in vec3 vertexColor;
in vec3 vertexNormal;

uniform mat4 mvp;

out Vertex
{
  vec4 normal;
  vec4 color;
  vec4 position;
} vVertex;

void main()
{
	vVertex.position = vec4(vertexPosition, 1.0);
	vVertex.normal = normalize ( vec4(vertexNormal, 1.0 ) );
	vVertex.color =  vec4(vertexColor, 1.0);
}
