#version 150 core

in vec3 worldPosition;
in vec3 worldNormal;

in vec3 color;

out vec4 fragColor;

void main()
{
    vec3 n = normalize(worldNormal);
    vec3 s = normalize(vec3(1.0, 0.0, 1.0) - worldPosition);
    vec3 v = normalize(-worldPosition);

    float diffuse = max(dot(s, n), 0.0);

    if (diffuse > 0.95)
        diffuse = 1.0;
    else if (diffuse > 0.5)
        diffuse = 0.5;
    else if (diffuse > 0.25)
        diffuse = 0.25;
    else
        diffuse = 0.1;
diffuse = 1.0;
    fragColor = vec4(color, 0.5);
}