attribute vec3 vertexPosition;
attribute vec3 vertexNormal;

attribute mat4 instanceModelMatrix; //from GeometryMultiplier
attribute vec3 instanceColor;       //from GeometryMultiplier

varying vec3 worldPosition;
varying vec3 worldNormal;
varying vec3 color;

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
