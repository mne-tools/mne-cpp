#version 400 core

uniform mat4 mvp;
uniform mat4 projectionMatrix;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec4 tePosition[];
in vec3 teColor[];
in vec3 teNormal[];

out vec3 gNormal;
out vec3 gColor;
out vec3 gPosition;

void main()
{
	gl_Position = mvp * tePosition[0];
	gNormal = teNormal[0];
	gColor = teColor[0];
	gPosition = gl_Position.xyz;
	EmitVertex();

    gl_Position = mvp * tePosition[1];
	gNormal = teNormal[1];
	gColor = teColor[1];
	gPosition = gl_Position.xyz;
	EmitVertex();

    gl_Position = mvp * tePosition[2];
	gNormal = teNormal[2];
	gColor = teColor[2];
	gPosition = gl_Position.xyz;
	EmitVertex();

    EndPrimitive();
}
