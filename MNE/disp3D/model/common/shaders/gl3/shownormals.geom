#version 400 core

uniform mat4 mvp;

layout(points) in;

layout(line_strip, max_vertices = 2) out;

uniform float normal_length = 0.0005f;

in Vertex
{
  vec4 normal;
  vec4 color;
  vec4 position;
} vVertex[];

out vec4 vertex_color;

void main()
{
	gl_Position = mvp * vVertex[0].position;
	vertex_color = vec4(1.0, 0.0, 0.0, 1.0);
	EmitVertex();

	gl_Position = mvp * (vVertex[0].position + vVertex[0].normal * normal_length);
	vertex_color = vec4(1.0, 0.0, 0.0, 1.0);
	EmitVertex();

	EndPrimitive();
}

