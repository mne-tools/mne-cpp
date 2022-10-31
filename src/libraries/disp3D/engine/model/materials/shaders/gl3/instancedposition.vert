#version 150 core

in vec3 vertexPosition;
in vec3 vertexNormal;

in mat4 instanceModelMatrix; //from GeometryMultiplier
in vec4 instanceColor;       //from GeometryMultiplier

out vec3 worldPosition;
out vec3 worldNormal;
out vec4 color;

uniform mat3 modelNormalMatrix;
uniform mat4 modelMatrix;
uniform mat4 mvp;

void main()
{
    color = instanceColor;

    vec4 pos = vec4(vertexPosition.xyz, 1.0);

    worldNormal = normalize( modelNormalMatrix * vertexNormal );
    worldPosition = vec3( modelMatrix * instanceModelMatrix * pos);

    gl_Position = mvp * instanceModelMatrix * pos;
}
