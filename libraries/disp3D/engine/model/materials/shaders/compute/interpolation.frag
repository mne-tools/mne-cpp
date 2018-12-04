#version 430 core

#pragma include light.inc.frag

// TODO: Replace with a struct
uniform vec3 kd;            // Diffuse reflectivity
uniform vec3 ks;            // Specular reflectivity
uniform float shininess;    // Specular shininess factor
uniform float alpha;

uniform vec3 eyePosition;

in vec3 worldPosition;
in vec3 worldNormal;
in vec4 color;

out vec4 fragColor;

void main(void)
{
    //discard fragments with no activity
    if(color.a == 0.0)
    {
        discard;
    }

    //Lightning
    vec3 diffuseColor, specularColor;
    adsModel(worldPosition, worldNormal, eyePosition, shininess, diffuseColor, specularColor);
    fragColor = vec4( color.xyz + kd * diffuseColor + ks * specularColor, alpha );
}

