#version 400 core

layout( points ) in;
layout( points, max_vertices = 1 ) out;

in vec3 color; // Output from vertex shader for each vertex
out vec3 fColor; // Output to fragment shader

void main()
{
	fColor = color; // Point has only one vertex
	
    gl_Position = gl_in[0].gl_Position + vec4(-0.00001, 0.00001, 0.0, 0.0);
    EmitVertex();

//    gl_Position = gl_in[0].gl_Position + vec4(0.00001, 0.00001, 0.0, 0.0);
//    EmitVertex();

    EndPrimitive();
}