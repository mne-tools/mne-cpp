//=============================================================================================================
/**
* @file     deepeval.cpp
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
* @brief    DeepEval class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deepeval.h"


//*************************************************************************************************************
//=============================================================================================================
// SYSTEM INCLUDES
//=============================================================================================================

#ifdef _WIN32
#include <Windows.h>
#endif


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DEEPLIB;
using namespace Eigen;
using namespace Microsoft::MSR::CNTK;
using namespace CNTK;


// Used for retrieving the model appropriate for the element type (float / double)
template<typename ElemType>
using GetEvalProc = void(*)(IEvaluateModel<ElemType>**);


typedef std::pair<std::wstring, std::vector<float>*> MapEntry;
typedef std::map<std::wstring, std::vector<float>*> Layer;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DeepEval::DeepEval()
: m_pModel_v1(NULL)
{
}


//*************************************************************************************************************

DeepEval::DeepEval(const QString &sModelFilename)
: m_sModelFilename(sModelFilename)
, m_pModel_v1(NULL)
{
    loadModel();
}


//*************************************************************************************************************

DeepEval::~DeepEval()
{

}


//*************************************************************************************************************

const QString& DeepEval::getModelFilename() const
{
    return m_sModelFilename;
}


//*************************************************************************************************************

void DeepEval::setModelFilename(const QString &sModelFilename)
{
    m_sModelFilename = sModelFilename;
}


//*************************************************************************************************************

bool DeepEval::evalModel(const VectorXf &inputs, VectorXf &outputs)
{
    std::vector<float> stdInputs(inputs.data(), inputs.data() + inputs.size());
    std::vector<float> stdOutputs;

    bool val = evalModel(stdInputs,stdOutputs);

    outputs = VectorXf::Map(stdOutputs.data(), stdOutputs.size());

    return val;
}


//*************************************************************************************************************

bool DeepEval::evalModel(std::vector<float>& inputs, std::vector<float>& outputs)
{
    if( !m_pModel_v1 )
        return false;

    // get the model's layers dimensions
    std::map<std::wstring, size_t> inDims;
    std::map<std::wstring, size_t> outDims;
    m_pModel_v1->GetNodeDimensions(inDims, NodeGroup::nodeInput);
    m_pModel_v1->GetNodeDimensions(outDims, NodeGroup::nodeOutput);

    std::wstring inputLayerName = inDims.begin()->first;
    std::wstring outputLayerName = outDims.begin()->first;

    if(inDims[inputLayerName] != inputs.size()) {
        fprintf(stderr, "Input dimension does not fit %zu != %llu.\n", inputs.size(), inDims[inputLayerName]);
        return false;
    }

    // Setup the maps for inputs and output
    Layer inputLayer;
    inputLayer.insert(MapEntry(inputLayerName, &inputs));
    Layer outputLayer;
    outputLayer.insert(MapEntry(outputLayerName, &outputs));

    // We can call the evaluate method and get back the results (single layer)...
    m_pModel_v1->Evaluate(inputLayer, outputLayer);

    // This pattern is used by End2EndTests to check whether the program runs to complete.
    fprintf(stderr, "Evaluation complete.\n");

    return true;
}


//*************************************************************************************************************

bool DeepEval::loadModel()
{
    QFile file(m_sModelFilename);
    if(!file.exists()) {
        qCritical("Model filename (%s) does not exist.\n", m_sModelFilename.toUtf8().constData());
        return false;
    }

    fprintf(stderr, "Evaluating Model %s\n", m_sModelFilename.toUtf8().constData());

    const std::string modelFile = m_sModelFilename.toUtf8().constData();

    GetEvalF(&m_pModel_v1);

    // Load model with desired outputs
    std::string networkConfiguration;
    // Uncomment the following line to re-define the outputs (include h1.z AND the output ol.z)
    // When specifying outputNodeNames in the configuration, it will REPLACE the list of output nodes
    // with the ones specified.
    //networkConfiguration += "outputNodeNames=\"h1.z:ol.z\"\n";
    networkConfiguration += "modelPath=\"" + modelFile + "\"";
    m_pModel_v1->CreateNetwork(networkConfiguration);

    return true;
}


//*************************************************************************************************************

size_t DeepEval::inputDimensions()
{
    if(m_pModel_v1) {
        std::map<std::wstring, size_t> inDims;
        m_pModel_v1->GetNodeDimensions(inDims, NodeGroup::nodeInput);
        std::wstring inputLayerName = inDims.begin()->first;
        return inDims[inputLayerName];
    }
    return 0;
}


//*************************************************************************************************************

size_t DeepEval::outputDimensions()
{
    if(m_pModel_v1) {
        std::map<std::wstring, size_t> outDims;
        m_pModel_v1->GetNodeDimensions(outDims, NodeGroup::nodeOutput);
        std::wstring outputLayerName = outDims.begin()->first;
        return outDims[outputLayerName];
    }
    return 0;
}


//*************************************************************************************************************

bool DeepEval::GetVariableByName(std::vector<Variable> variableLists, std::wstring varName, Variable &var)
{
    for (std::vector<Variable>::iterator it = variableLists.begin(); it != variableLists.end(); ++it)
    {
        if (it->Name().compare(varName) == 0)
        {
            var = *it;
            return true;
        }
    }
    return false;
}


//*************************************************************************************************************

bool DeepEval::GetInputVariableByName(FunctionPtr evalFunc, std::wstring varName, Variable &var)
{
    return GetVariableByName(evalFunc->Arguments(), varName, var);
}


//*************************************************************************************************************

bool DeepEval::GetOutputVaraiableByName(FunctionPtr evalFunc, std::wstring varName, Variable &var)
{
    return GetVariableByName(evalFunc->Outputs(), varName, var);
}


//*************************************************************************************************************

void DeepEval::runEvaluation(FunctionPtr evalFunc, const DeviceDescriptor &device, const CNTK::Variable& inputVar, const ValuePtr& inputValue, const CNTK::Variable& outputVar, ValuePtr& outputValue)
{
    std::unordered_map<Variable, ValuePtr> outputs = {{outputVar, outputValue}};
    evalFunc->Forward({{inputVar, inputValue}}, outputs, device);
    outputValue = outputs[outputVar];
}


//*************************************************************************************************************

void DeepEval::OutputFunctionInfo(FunctionPtr func)
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

void DeepEval::loadModel_v2(const DeviceDescriptor &device)
{
    QString fileName("./mne_deep_models/examples/output/models/ex_deep_one_hidden");
    std::wstring file = fileName.toStdWString();
    m_pModelFunction_v2 = Function::LoadModel(file, device);
}


//*************************************************************************************************************

void DeepEval::evalModel_v2(const DeviceDescriptor &device, const int threadCount)
{
    // The model file will be trained and copied to the current runtime directory first.

    OutputFunctionInfo(m_pModelFunction_v2);
    fprintf(stderr, "MultiThreadsEvaluationWithLoadModel on device=%d\n", device.Id());

//    // Run evaluation in parallel.
//    std::vector<std::thread> threadList(threadCount);
//    for (int th = 0; th < threadCount; ++th)
//    {
//        threadList[th] = std::thread(runEvaluation, m_pModelFunction_v2->Clone(), device,inputVar,inputValue,outputVar,outputValue);
//    }

//    for (int th = 0; th < threadCount; ++th)
//    {
//        threadList[th].join();
//        fprintf(stderr, "thread %d joined.\n", th);
//        fflush(stderr);
//    }




    const std::wstring inputNodeName = L"features";
    const std::wstring outputNodeName = L"out.z";

    Variable inputVar;
    if (!GetInputVariableByName(m_pModelFunction_v2, inputNodeName, inputVar)) {
        fprintf(stderr, "Input variable %S is not available.\n", inputNodeName.c_str());
        throw("Input variable not found error.");
    }

    Variable outputVar;
    if (!GetOutputVaraiableByName(m_pModelFunction_v2, outputNodeName, outputVar)) {
        fprintf(stderr, "Output variable %S is not available.\n", outputNodeName.c_str());
        throw("Output variable not found error.");
    }

    // Evaluate the network in several runs
    size_t numSamples = 3;

    std::vector<float> inputData(inputVar.Shape().TotalSize() * numSamples);
    for (size_t i = 0; i < inputData.size(); ++i)
    {
        inputData[i] = static_cast<float>(i % 255);
    }

    NDShape inputShape = inputVar.Shape().AppendShape({1, numSamples});

    ValuePtr inputValue = MakeSharedObject<Value>(MakeSharedObject<NDArrayView>(inputShape, inputData, true));

    ValuePtr outputValue;







    runEvaluation(m_pModelFunction_v2,device,inputVar,inputValue,outputVar,outputValue);









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
        fprintf(stderr, "Sample %lu:\n", (unsigned long)i);
        fprintf(stderr, "Ouput:");
        for (size_t j = 0; j < outputDim; j++)
        {
            fprintf(stderr, "%f ", outputData[dataIndex++]);
        }
        fprintf(stderr, "\n");
    }




}

//*************************************************************************************************************

void DeepEval::testEval() {
    // The number of threads running evaluation in parallel.
    const int numOfThreads = 2;

    // test multi-threads evaluation with loading existing models
    fprintf(stderr, "\n##### Run evaluation using pre-trained model on CPU. #####\n");
    loadModel_v2(DeviceDescriptor::CPUDevice());
    evalModel_v2(DeviceDescriptor::CPUDevice(), numOfThreads);
}
