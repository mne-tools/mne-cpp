#define FP highp

struct Material {
    FP vec3 kd;            // Diffuse reflectivity
    FP vec3 ks;            // Specular reflectivity
    FP float shininess;    // Specular shininess factor
    FP float alpha;
};

uniform Material material;

uniform FP vec3 eyePosition;

varying FP vec3 worldPosition;
varying FP vec3 worldNormal;
varying FP vec3 color;

#pragma include light.inc.frag

void main()
{
    FP vec3 diffuseColor, specularColor;
    adsModel(worldPosition, worldNormal, eyePosition, material.shininess, diffuseColor, specularColor);
    gl_FragColor = vec4( color + material.kd * diffuseColor + material.ks * specularColor, material.alpha );
}
