const int MAX_LIGHTS = 8;
const int TYPE_DIRECTIONAL = 1;
const int TYPE_SPOT = 2;
struct Light {
    int type;
    vec3 position;
    vec3 color;
    float intensity;
    vec3 direction;
    vec3 attenuation;
    float cutOffAngle;
};
uniform Light lights[MAX_LIGHTS];
uniform int lightCount;

in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec3 vColor;

uniform float alpha;

void adModel(const in vec3 vpos, const in vec3 vnormal, out vec3 diffuseColor)
{
    diffuseColor = vec3(0.0);

    vec3 n = normalize( vnormal );

    int i;
    vec3 s;
    for (i = 0; i < lightCount; ++i) {
        float att = 1.0;
        if ( lights[i].type != TYPE_DIRECTIONAL ) {
            s = lights[i].position - vpos;
            if (length( lights[i].attenuation ) != 0.0) {
                float dist = length(s);
                att = 1.0 / (lights[i].attenuation.x + lights[i].attenuation.y * dist + lights[i].attenuation.z * dist * dist);
            }
            s = normalize( s );
            if ( lights[i].type == TYPE_SPOT ) {
                if ( degrees(acos(dot(-s, normalize(lights[i].direction))) ) > lights[i].cutOffAngle)
                    att = 0.0;
            }
        } else {
            s = normalize( -lights[i].direction );
        }

        float diffuse = max( dot( s, n ), 0.0 );

        diffuseColor += att * lights[i].intensity * diffuse * lights[i].color;
    }
}

void main()
{
    vec3 diffuseColor;
	
    adModel(vWorldPosition, vWorldNormal, diffuseColor);
    gl_FragColor = vec4( vColor + vColor * diffuseColor, alpha );
}