#version 400 core

uniform mat3 normalMatrix;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 teColor[3];
in vec3 tePosition[3];

out vec3 gFacetNormal;
out vec3 gColor;

void main()
{
	gColor = teColor[0];
	
    vec3 A = tePosition[2] - tePosition[0];
    vec3 B = tePosition[1] - tePosition[0];
    gFacetNormal = normalMatrix * -1.0 * normalize(cross(A, B));

    gl_Position = gl_in[0].gl_Position; EmitVertex();

    gl_Position = gl_in[1].gl_Position; EmitVertex();

    gl_Position = gl_in[2].gl_Position; EmitVertex();

    EndPrimitive();
}
