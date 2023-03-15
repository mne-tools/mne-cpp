#version 150 core

#pragma include light.inc.frag

// TODO: Replace with a struct
uniform vec3 ka;            // Ambient reflectivity
uniform vec3 kd;            // Diffuse reflectivity
uniform vec3 ks;            // Specular reflectivity
uniform float shininess;    // Specular shininess factor
uniform float alpha;

uniform vec3 eyePosition;

in vec3 worldPosition;
in vec3 worldNormal;
in vec4 color;

out vec4 fragColor;

void main()
{
    vec3 diffuseColor, specularColor;
    adsModel(worldPosition, worldNormal, eyePosition, shininess, diffuseColor, specularColor);
    fragColor = vec4( color.rgb + kd * diffuseColor + ks * specularColor, color.a*alpha );
}
