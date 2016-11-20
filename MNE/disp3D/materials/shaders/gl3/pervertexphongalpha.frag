#version 400 core

in vec3 gColor;

out vec4 fragColor;

void main()
{
	fragColor = vec4(gColor, 1.0);
}
