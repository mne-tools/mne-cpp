#version 430 core

in vec3 vertexPosition;
in vec3 vertexColor;
in vec3 vertexNormal;
in vec4 OutputColor;

out vec3 worldPosition;
out vec3 worldNormal;
out vec4 color;

uniform mat4 modelMatrix;
uniform mat3 modelNormalMatrix;
uniform mat4 mvp; //need for the QCamera and camera controller to work

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0);

    worldNormal = normalize( modelNormalMatrix * vertexNormal );
    worldPosition = vec3( modelMatrix * vec4( vertexPosition, 1.0 ) );

    color = OutputColor;

}
