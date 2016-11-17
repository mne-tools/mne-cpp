#version 400 core

out vec4 fragColor;

in vec3 gFacetNormal;
in vec3 gTriDistance;
in vec3 gPatchDistance;

void main()
{
    fragColor = vec4(vec3(1.0,1.0,1.0), 1.0);
}
