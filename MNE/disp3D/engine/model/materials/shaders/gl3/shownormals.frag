#version 150 core

out vec4 frag_color;

in vec4 vertex_color;

void main()
{
  frag_color = vertex_color;
}
