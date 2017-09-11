#version 430 core

//numer of rows in the weight matrix (n)
//uniform uint rows;

//number of colums in the weight matrix(m)
uniform uint cols;

//TODO x workgroup is size of 1 row
layout (local_size_x = 1, local_size_y = 1) in;


//output buffer colortable needs to be applied on this array
layout (std430, binding = 0) buffer InterpolatedSignal
{
    float interpolatedSignal[];
};

//Weight matrix buffer
layout (std430, binding = 1) buffer WeightMat
{
    float weights[];
};

//Buffer with measurement data. Shared Memory!
layout (std430, binding = 2) buffer MeasurementVec
{
    float mData[];
};


void main(void)
{
    // only for 1 dimension weight matrix
    //uint globalId = gl_GlobalInvocationID.x;
    uint globalId = gl_GlobalInvocationID.x + gl_NumWorkGroups.y * gl_GlobalInvocationID.y;

    float sum = 0.0;
    for(uint i = 0; i < cols; i++)
    {
        sum += weights[globalId * cols + i] * mData[i];
    }

    interpolatedSignal[globalId] = sum;

    //yOut[uint(mod(globalId, cols))] += weights[globalId] * mData[uint(mod(globalId, rows))];


}
