#version 150 core

in vec3 vertexPosition;
in vec3 vertexNormal;

in mat4 instanceModelMatrix; //from GeometryMultiplier

out vec3 worldPosition;
out vec3 worldNormal;

uniform mat3 modelNormalMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
    vec4 pos = vec4(vertexPosition.xyz, 1.0);

    worldNormal = normalize( modelNormalMatrix * vertexNormal );
    worldPosition = vec3( instanceModelMatrix * pos);

    gl_Position = projectionMatrix * viewMatrix * instanceModelMatrix * pos;
}
