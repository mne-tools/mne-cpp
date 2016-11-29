#version 400 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewProjection;

layout(triangles) in;
layout(triangle_strip, max_vertices = 32) out;

in vec4 tePosition[];
in vec3 teColor[];
in vec3 teNormal[];

out vec3 gNormal;
out vec3 gColor;
out vec3 gPosition;

vec3 CG;

void ProduceVertex(int v) 
{
	gl_Position = projectionMatrix * vec4( CG + 0.94 * ( tePosition[v].xyz - CG ), 1. );
	//gl_Position = projectionMatrix * tePosition[v];
	gNormal = normalize( teNormal[v] );
	gColor = teColor[v];
	gPosition = gl_Position.xyz;
	EmitVertex();
}

void main()
{
	CG = ( tePosition[0].xyz + tePosition[1].xyz + tePosition[2].xyz ) / 3.;
	
	ProduceVertex(0);
	ProduceVertex(1);
	ProduceVertex(2);    
}
