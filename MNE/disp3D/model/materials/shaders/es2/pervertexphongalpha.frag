#define FP highp

uniform FP vec3 kd;            // Diffuse reflectivity
uniform FP vec3 ks;            // Specular reflectivity
uniform FP float shininess;    // Specular shininess factor
uniform FP float alpha;

uniform FP vec3 eyePosition;

varying vec3 FP worldPosition;
varying vec3 FP worldNormal;
varying vec3 FP color;

#pragma include light.inc.frag

void main()
{
    FP vec3 diffuseColor, specularColor;
    adsModel(worldPosition, worldNormal, eyePosition, shininess, diffuseColor, specularColor);
    gl_FragColor = vec4( color + kd * diffuseColor + ks * specularColor, alpha );
}
