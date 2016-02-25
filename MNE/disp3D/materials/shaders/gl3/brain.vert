#version 150 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec3 vertexColor;

out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec3 vColor;

uniform mat4 modelMatrix;
uniform mat3 modelNormalMatrix;
uniform mat4 mvp;

void main()
{
    vWorldNormal = normalize( modelNormalMatrix * vertexNormal );
    vWorldPosition = vec3( modelMatrix * vec4( vertexPosition, 1.0 ) );
    vColor = vertexColor;

    gl_Position = mvp * vec4( vertexPosition, 1.0 );
}
