attribute vec3 vertexPosition;
attribute vec3 vertexNormal;

attribute vec3 geometryPosition; //from InstancedCustomMesh

varying vec3 worldPosition;
varying vec3 worldNormal;

uniform mat4 modelMatrix;
uniform mat3 modelNormalMatrix;
uniform mat4 mvp;

void main()
{
    vec4 pos = vec4(vertexPosition.xyz, 1.0) + vec4(geometryPosition, 0.0);

    worldNormal = normalize( modelNormalMatrix * vertexNormal );
    worldPosition = vec3( modelMatrix * pos );

    gl_Position = mvp * pos;
}
