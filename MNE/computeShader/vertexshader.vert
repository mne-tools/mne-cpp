#version 430 core

uniform mat4 mvp; //need for the QCamera and camera controller to work

in vec3 vertexPosition;
in vec3 vertexColor;
in vec3 vertexNormal;
in float YOutVec;

//out vec4 vPosition;
out vec3 vColor;
out vec3 vNormal;

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0);

        vNormal = normalize( vertexNormal );

        //TODO use some kind of colormap
        vColor = vec3(YOutVec, 0.5, 0.5);
}
