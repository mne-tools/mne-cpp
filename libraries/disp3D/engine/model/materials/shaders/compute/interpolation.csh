#version 430 core

//numer of rows in the weight matrix
uniform uint rows;

//number of colums in the weight matrix
uniform uint cols;

//local work group sizes
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

//Buffer with input data.
layout (std430, binding = 2) buffer InputVec
{
    float inputData[];
};


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
            sum += weights[globalId * cols + i] * intputData[i];
        }

        interpolatedSignal[globalId] = sum;
    }
}
