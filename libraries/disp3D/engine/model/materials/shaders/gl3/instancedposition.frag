#version 150 core

#pragma include light.inc.frag

struct Material {
    vec3 kd;            // Diffuse reflectivity
    vec3 ks;            // Specular reflectivity
    float shininess;    // Specular shininess factor
    float alpha;
};

uniform Material material;

uniform vec3 eyePosition;

in vec3 worldPosition;
in vec3 worldNormal;
in vec3 color;

out vec4 fragColor;


void main()
{
    vec3 diffuseColor, specularColor;
    adsModel(worldPosition, worldNormal, eyePosition, material.shininess, diffuseColor, specularColor);
    fragColor = vec4( color + material.kd * diffuseColor + material.ks * specularColor, material.alpha );
}
