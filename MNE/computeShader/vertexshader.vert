#version 430 core


in vec3 vertexPosition;
in vec3 vertexColor;
in vec3 vertexNormal;
in float YOutVec;

//out vec4 vPosition;
out vec3 vColor;
out vec3 vNormal;

void main()
{
    gl_Position = vec4(vertexPosition, 1.0);

        vNormal = normalize( vertexNormal );

        //TODO use some kind of colormap
        vColor = vec3(YOutVec, 0.0, 0.0);
}
