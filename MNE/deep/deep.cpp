//=============================================================================================================
/**
* @file     deep.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Deep class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deep.h"


//*************************************************************************************************************
//=============================================================================================================
// SYSTEM INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DEEPLIB;
using namespace CNTK;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Deep::Deep()
{
}


//*************************************************************************************************************

Deep::~Deep()
{

}


//*************************************************************************************************************

void Deep::setUseGPU(bool useGPU)
{
    m_bUseGPU = useGPU;
}


//*************************************************************************************************************

void Deep::RunEvaluationClassifier(FunctionPtr evalFunc, const DeviceDescriptor &device)
{
    const std::wstring inputNodeName = L"features";

    Variable inputVar;
    if (!GetInputVariableByName(evalFunc, inputNodeName, inputVar))
    {
        fprintf(stderr, "Input variable %S is not available.\n", inputNodeName.c_str());
        throw("Input variable not found error.");
    }

    // Evaluate the network in several runs
    size_t iterationCount = 4;
    unsigned int randSeed = 2;
    srand(randSeed);
    size_t numSamples = 3;
    std::vector<float> inputData(inputVar.Shape().TotalSize() * numSamples);
    for (size_t t = 0; t < iterationCount; ++t)
    {
        for (size_t i = 0; i < inputData.size(); ++i)
        {
            inputData[i] = ((float)rand()) / RAND_MAX;
        }

        // Create input data shape. Adding sequence length and numSamples as axes.
        // Todo: remove sequence length when only numSamples is supported.
        // Todo: add convenience APIs to simplify data preparation here.
        NDShape inputShape = inputVar.Shape().AppendShape({1, numSamples});
        ValuePtr inputValue = MakeSharedObject<Value>(MakeSharedObject<NDArrayView>(inputShape, inputData, true));

        // Define output.
        ValuePtr outputValue;
        auto outputVar = evalFunc->Output();
        std::unordered_map<Variable, ValuePtr> outputs = {{outputVar, outputValue}};

        // Evaluate the model
        evalFunc->Forward({{inputVar, inputValue}}, outputs, device);

        // Get output value
        outputValue = outputs[outputVar];

        // Todo: remove sequence length when only numSamples is supported.
        // Todo: add convenience APIs to simplify retrieval of output results.
        NDShape outputShape = outputVar.Shape().AppendShape({1, numSamples});
        std::vector<float> outputData(outputShape.TotalSize());
        NDArrayViewPtr cpuArrayOutput = MakeSharedObject<NDArrayView>(outputShape, outputData, false);
        cpuArrayOutput->CopyFrom(*outputValue->Data());

        assert(outputData.size() == outputVar.Shape()[0] * numSamples);
        fprintf(stderr, "Evaluation result:\n");
        size_t dataIndex = 0;
        auto outputDim = outputVar.Shape()[0];
        for (size_t i = 0; i < numSamples; i++)
        {
            fprintf(stderr, "Iteration:%lu, Sample %lu:\n", (unsigned long)t, (unsigned long)i);
            fprintf(stderr, "    ");
            dataIndex = i * outputDim;
            for (size_t j = 0; j < std::min((size_t)10, outputDim); j++)
            {
                fprintf(stderr, "%f ", outputData[dataIndex++]);
            }
            if (outputDim > 10)
            {
                fprintf(stderr, "...");
            }
            fprintf(stderr, "\n");
        }
    }
}


//*************************************************************************************************************

void Deep::OutputFunctionInfo(FunctionPtr func)
{
    auto inputVariables = func->Arguments();
    fprintf(stderr, "Function '%S': Input Variables (count=%lu)\n", func->Name().c_str(), (unsigned long)inputVariables.size());
    for_each(inputVariables.begin(), inputVariables.end(), [](const Variable v) {
        fprintf(stderr, "    name=%S, kind=%d\n", v.Name().c_str(), static_cast<int>(v.Kind()));
    });

    auto outputVariables = func->Outputs();
    fprintf(stderr, "Function '%S': Output Variables (count=%lu)\n", func->Name().c_str(), (unsigned long)outputVariables.size());
    for_each(outputVariables.begin(), outputVariables.end(), [](const Variable v) {
        fprintf(stderr, "    name=%S, kind=%d\n", v.Name().c_str(), static_cast<int>(v.Kind()));
    });
}


//*************************************************************************************************************

FunctionPtr Deep::SetupFullyConnectedLinearLayer(Variable input, size_t outputDim, const DeviceDescriptor &device, const std::wstring &outputName)
{
    assert(input.Shape().Rank() == 1);
    size_t inputDim = input.Shape()[0];

    auto timesParam = CNTK::Parameter(CNTK::NDArrayView::RandomUniform<float>({outputDim, inputDim}, -0.05, 0.05, 1, device));
    auto timesFunction = CNTK::Times(timesParam, input);

    auto plusParam = CNTK::Parameter(CNTK::NDArrayView::RandomUniform<float>({outputDim}, -0.05, 0.05, 1, device));
    return CNTK::Plus(plusParam, timesFunction, outputName);
}


//*************************************************************************************************************

FunctionPtr Deep::SetupFullyConnectedDNNLayer(Variable input, size_t outputDim, const DeviceDescriptor &device, const std::function<FunctionPtr (const FunctionPtr &)> &nonLinearity)
{
    return nonLinearity(SetupFullyConnectedLinearLayer(input, outputDim, device));
}


//*************************************************************************************************************

void Deep::MultiThreadsEvaluationWithClone(const DeviceDescriptor &device, const int threadCount)
{
    using namespace std::placeholders;

    const size_t inputDim = 937;
    const size_t numOutputClasses = 9304;
    const size_t numHiddenLayers = 6;
    const size_t hiddenLayersDim = 2048;

    auto inputVar = InputVariable({inputDim}, DataType::Float, L"features");

    assert(numHiddenLayers >= 1);
    auto classifierRoot = SetupFullyConnectedDNNLayer(inputVar, hiddenLayersDim, device, std::bind(Sigmoid, _1, L""));
    for (size_t i = 1; i < numHiddenLayers; ++i)
    {
        classifierRoot = SetupFullyConnectedDNNLayer(classifierRoot, hiddenLayersDim, device, std::bind(Sigmoid, _1, L""));
    }

    auto outputTimesParam = Parameter(NDArrayView::RandomUniform<float>({numOutputClasses, hiddenLayersDim}, -0.5, 0.5, 1, device));
    auto classifierFunc = Times(outputTimesParam, classifierRoot, L"classifierOutput");

    // Now test the structure
    if (classifierFunc->Parameters().size() != ((numHiddenLayers * 2) + 1))
    {
        throw std::runtime_error("MultiThreadsEvaluationWithClone: Function does not have expected Parameter count");
    }

    OutputFunctionInfo(classifierFunc);
    fprintf(stderr, "MultiThreadsEvaluationWithClone on device=%d\n", device.Id());

    // Run evaluation in parallel
    std::vector<std::thread> threadList(threadCount);
    for (int th = 0; th < threadCount; ++th)
    {
        threadList[th] = std::thread(RunEvaluationClassifier, classifierFunc->Clone(), device);
    }

    for (int th = 0; th < threadCount; ++th)
    {
        threadList[th].join();
        fprintf(stderr, "thread %d joined.\n", th);
        fflush(stderr);
    }
}


//*************************************************************************************************************

void Deep::testClone()
{
    int numOfThreads = 2;

    // Test multi-threads evaluation using clone.
    fprintf(stderr, "\n##### Run evaluation using clone function on CPU. #####\n");
    MultiThreadsEvaluationWithClone(DeviceDescriptor::CPUDevice(), numOfThreads);
}
