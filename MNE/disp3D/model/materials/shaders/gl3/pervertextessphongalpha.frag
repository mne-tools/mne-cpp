#version 400 core

#pragma include light.inc.frag

uniform vec3 kd;            // Diffuse reflectivity
uniform vec3 ks;            // Specular reflectivity
uniform float shininess;    // Specular shininess factor
uniform float alpha;
uniform vec3 eyePosition;

in vec3 gColor;
in vec3 gNormal;
in vec3 gPosition;

out vec4 fragColor;

void main()
{
    vec3 diffuseColor, specularColor;
	
	//Compute phong model
    adsModel(gPosition, gNormal, eyePosition, shininess, diffuseColor, specularColor);
    fragColor = vec4( gColor + kd * diffuseColor + ks * specularColor, alpha );
}
