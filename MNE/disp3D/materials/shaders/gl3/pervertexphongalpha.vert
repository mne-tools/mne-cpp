#version 150 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec3 vertexColor;

out vec3 worldPosition;
out vec3 worldNormal;
out vec3 vColor;

uniform mat4 modelView;
uniform mat3 modelViewNormal;
uniform mat4 mvp;

void main()
{
    worldNormal = normalize( modelViewNormal * vertexNormal );
    worldPosition = vec3( modelView * vec4( vertexPosition, 1.0 ) );
	vColor = vertexColor;
	
    gl_Position = mvp * vec4( vertexPosition, 1.0 );
}
