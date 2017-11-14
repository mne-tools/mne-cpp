#version 430 core

//numer of rows in the weight matrix
uniform uint rows;

//number of colums in the weight matrix
uniform uint cols;

//Lower threshold
uniform float fThresholdX;

//upper threshold
uniform float fThresholdZ;

//color map type
uniform uint ColormapType;

//local work group sizes
layout (local_size_x = 1, local_size_y = 1) in;

//output buffer colortable needs to be applied on this array
layout (std430, binding = 0) buffer OutputColor
{
    vec4 outputColor[];
};

//Weight matrix buffer
layout (std430, binding = 1) buffer WeightMat
{
    float weights[];
};

//Buffer with input data.
layout (std430, binding = 2) buffer InputVec
{
    float inputData[];
};


//FORWARD DECLARATIONS
float linearSlope(float x, float m, float n);
vec3 colorMapHot(float x);
vec3 colorMapJet(float x);

void main(void)
{
    uint globalId = gl_GlobalInvocationID.x + gl_NumWorkGroups.y * gl_GlobalInvocationID.y;

    //prevent out of bound
    if(globalId < rows)
    {
        //calc weightMatrix * inputVec for one output value.
        float sum = 0.0;
        for(uint i = 0; i < cols; i++)
        {
            sum += weights[globalId * cols + i] * inputData[i];
        }

        //calc thresholds
        float fSample = abs(sum);
        float fTresholdDiff = fThresholdZ - fThresholdX;

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
            outputColor[globalId] = vec4(colorMapHot(fSample), 1.0);
        }
        else
        {
            outputColor[globalId] = vec4(colorMapJet(fSample), 1.0);
        }

        //dont display verts with no activity
        if(fSample <= fThresholdX)
        {
            outputColor[globalId].a = 0.0;
        }
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
