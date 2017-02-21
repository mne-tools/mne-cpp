#version 150 core

uniform mat4 mvp;

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

uniform float normal_length = 0.0005f;

in Vertex
{
  vec4 normal;
  vec4 color;
  vec4 position;
} vVertex[];

out vec4 vertex_color;

void ProduceVertexNormal(int v) 
{
	gl_Position = mvp * vVertex[v].position;
	vertex_color = vec4(1.0, 0.0, 0.0, 1.0);
	EmitVertex();

	gl_Position = mvp * (vVertex[v].position + vVertex[v].normal * normal_length);
	vertex_color = vec4(1.0, 0.0, 0.0, 1.0);
	EmitVertex();

	EndPrimitive();
}

void main()
{
	ProduceVertexNormal(0);
	ProduceVertexNormal(1);
	ProduceVertexNormal(2);
}

