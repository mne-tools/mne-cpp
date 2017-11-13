#version 430 core

in vec3 vertexPosition;
in vec3 vertexColor;
in vec3 vertexNormal;
in float InterpolatedSignal;

out vec3 worldPosition;
out vec3 worldNormal;
out vec4 color;

uniform mat4 modelMatrix;
uniform mat3 modelNormalMatrix;
uniform mat4 mvp; //need for the QCamera and camera controller to work

uniform float fThresholdX;
uniform float fThresholdZ;

uniform uint ColormapType;

//FORWARD DECLARATIONS
float linearSlope(float x, float m, float n);
vec3 colorMapHot(float x);
vec3 colorMapJet(float x);

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0);

    worldNormal = normalize( modelNormalMatrix * vertexNormal );
    worldPosition = vec3( modelMatrix * vec4( vertexPosition, 1.0 ) );

    //calc thresholds
    float fSample = abs(InterpolatedSignal);
    float fTresholdDiff = fThresholdZ - fThresholdX;

    vec3 tempColor = vertexColor;

    if(fSample >= fThresholdX) {
        //Check lower and upper thresholds and normalize to one
        if(fSample >= fThresholdZ) {
            fSample = 1.0;
        } else {
            if(fSample != 0.0 && fTresholdDiff != 0.0 ) {
                fSample = (fSample - fThresholdX) / (fTresholdDiff);
            } else {
                fSample = 0.0;
            }
        }

        //@TODO Add more colormaps
        if(ColormapType == 0)
        {
            tempColor = colorMapHot(fSample);
        }
        else
        {
            tempColor = colorMapJet(fSample);
        }


    }


    //Set output color
    color = vec4(tempColor, 1.0);

    //dont display verts with no activity
    if(fSample <= fThresholdX)
    {
        color.a = 0.0;
    }
}


//*************************************************************************

float linearSlope(float x, float m, float n)
{
    return m * x + n;
}


//*************************************************************************

//color maps
vec3 colorMapJet(float x)
{
    vec3 outColor;

    if(x < 0.125)
    {
        //red
        outColor.x = 0.0;
        //green
        outColor.y = 0.0;
        //blue
        outColor.z = linearSlope(x, 4.0, 0.5);
    }
    else if(x >= 0.125 && x < 0.375)
    {
        //red
        outColor.x = 0.0;
        //green
        outColor.y = linearSlope(x, 4.0, -0.5);
        //blue
        outColor.z = 1.0;
    }
    else if(x >= 0.375 && x < 0.625)
    {
        //red
        outColor.x = linearSlope(x, 4.0, -1.5);
        //green
        outColor.y = 1.0;
        //blue
        outColor.z = linearSlope(x, -4.0, 2.5);
    }
    else if(x >= 0.625 && x < 0.875)
    {
        //red
        outColor.x = 1.0;
        //green
        outColor.y = linearSlope(x, -4.0, 3.5);
        //blue
        outColor.z = 0.0;
    }
    else
    {
        //red
        outColor.x = linearSlope(x, -4.0, 4.5);
        //green
        outColor.y = 0.0;
        //blue
        outColor.z = 0.0;
    }

    return outColor;
}


//*************************************************************************

vec3 colorMapHot(float x)
{
    vec3 outColor;
    if(x < 0.375)
    {
        //red
        outColor.x = linearSlope(x, 2.5621, 0.0392);
        //green
        outColor.y = 0.0;
        //blue
        outColor.z = 0.0;
    }
    else if(x >= 0.375 && x < 0.75)
    {
        //red
        outColor.x = 1.0;
        //green
        outColor.y = linearSlope(x, 2.6667, -1.0);
        //blue
        outColor.z = 0.0;
    }
    else
    {
        //red
        outColor.x = 1.0;
        //green
        outColor.y = 1.0;
        //blue
        outColor.z = linearSlope(x,4.0,-3.0);
    }

    return outColor;
}


//*************************************************************************
