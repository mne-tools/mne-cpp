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

#include "deepmodelcreator.h"


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
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DEEPLIB;
using namespace CNTK;
using namespace Eigen;


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

//void Deep::RunEvaluationClassifier(FunctionPtr evalFunc, const DeviceDescriptor &device)
//{
//    const std::wstring inputNodeName = L"features";

//    Variable inputVar;
//    if (!GetInputVariableByName(evalFunc, inputNodeName, inputVar))
//    {
//        fprintf(stderr, "Input variable %S is not available.\n", inputNodeName.c_str());
//        throw("Input variable not found error.");
//    }

//    // Evaluate the network in several runs
//    size_t iterationCount = 4;
//    unsigned int randSeed = 2;
//    srand(randSeed);
//    size_t numSamples = 3;
//    std::vector<float> inputData(inputVar.Shape().TotalSize() * numSamples);
//    for (size_t t = 0; t < iterationCount; ++t) {
//        for (size_t i = 0; i < inputData.size(); ++i) {
//            inputData[i] = ((float)rand()) / RAND_MAX;
//        }

//        // Create input data shape. Adding sequence length and numSamples as axes.
//        // Todo: remove sequence length when only numSamples is supported.
//        // Todo: add convenience APIs to simplify data preparation here.
////        NDShape inputShape = inputVar.Shape().AppendShape({1, numSamples});
////        ValuePtr inputValue = MakeSharedObject<Value>(MakeSharedObject<NDArrayView>(inputShape, inputData, true));
//        ValuePtr inputValue = Value::CreateBatch(inputVar.Shape(), inputData, device);


//        // Define output.
//        ValuePtr outputValue;
//        auto outputVar = evalFunc->Output();
//        std::unordered_map<Variable, ValuePtr> outputs = {{outputVar, outputValue}};

//        // Evaluate the model
//        evalFunc->Forward({{inputVar, inputValue}}, outputs, device);

//        // Get output value
//        outputValue = outputs[outputVar];

//        // Todo: remove sequence length when only numSamples is supported.
//        // Todo: add convenience APIs to simplify retrieval of output results.
//        NDShape outputShape = outputVar.Shape().AppendShape({1, numSamples});
//        std::vector<float> outputData(outputShape.TotalSize());
//        NDArrayViewPtr cpuArrayOutput = MakeSharedObject<NDArrayView>(outputShape, outputData, false);
//        cpuArrayOutput->CopyFrom(*outputValue->Data());

//        assert(outputData.size() == outputVar.Shape()[0] * numSamples);
//        fprintf(stderr, "Evaluation result:\n");
//        size_t dataIndex = 0;
//        auto outputDim = outputVar.Shape()[0];
//        for (size_t i = 0; i < numSamples; i++)
//        {
//            fprintf(stderr, "Iteration:%lu, Sample %lu:\n", (unsigned long)t, (unsigned long)i);
//            fprintf(stderr, "    ");
//            dataIndex = i * outputDim;
//            for (size_t j = 0; j < std::min((size_t)10, outputDim); j++)
//            {
//                fprintf(stderr, "%f ", outputData[dataIndex++]);
//            }
//            if (outputDim > 10)
//            {
//                fprintf(stderr, "...");
//            }
//            fprintf(stderr, "\n");
//        }
//    }
//}


//*************************************************************************************************************

//void Deep::MultiThreadsEvaluationWithClone(const DeviceDescriptor &device, const int threadCount)
//{
//    using namespace std::placeholders;

//    const size_t inputDim = 937;
//    const size_t numOutputClasses = 9304;
//    const size_t numHiddenLayers = 6;
//    const size_t hiddenLayersDim = 2048;

//    auto inputVar = InputVariable({inputDim}, DataType::Float, L"features");

//    assert(numHiddenLayers >= 1);
//    auto classifierRoot = DeepModelCreator::SetupFullyConnectedDNNLayer(inputVar, hiddenLayersDim, device, std::bind(Sigmoid, _1, L""));
//    for (size_t i = 1; i < numHiddenLayers; ++i)
//    {
//        classifierRoot = DeepModelCreator::SetupFullyConnectedDNNLayer(classifierRoot, hiddenLayersDim, device, std::bind(Sigmoid, _1, L""));
//    }

//    auto outputTimesParam = Parameter(NDArrayView::RandomUniform<float>({numOutputClasses, hiddenLayersDim}, -0.5, 0.5, 1, device));
//    auto classifierFunc = Times(outputTimesParam, classifierRoot, L"classifierOutput");

//    // Now test the structure
//    if (classifierFunc->Parameters().size() != ((numHiddenLayers * 2) + 1))
//    {
//        throw std::runtime_error("MultiThreadsEvaluationWithClone: Function does not have expected Parameter count");
//    }

//    OutputFunctionInfo(classifierFunc);
//    fprintf(stderr, "MultiThreadsEvaluationWithClone on device=%d\n", device.Id());

//    // Run evaluation in parallel
//    std::vector<std::thread> threadList(threadCount);
//    for (int th = 0; th < threadCount; ++th)
//    {
//        threadList[th] = std::thread(RunEvaluationClassifier, classifierFunc->Clone(), device);
//    }

//    for (int th = 0; th < threadCount; ++th)
//    {
//        threadList[th].join();
//        fprintf(stderr, "thread %d joined.\n", th);
//        fflush(stderr);
//    }
//}


//*************************************************************************************************************

//void Deep::testClone()
//{
//    int numOfThreads = 2;

//    // Test multi-threads evaluation using clone.
//    fprintf(stderr, "\n##### Run evaluation using clone function on CPU. #####\n");
//    MultiThreadsEvaluationWithClone(DeviceDescriptor::CPUDevice(), numOfThreads);
//}


//*************************************************************************************************************

size_t Deep::inputDimensions(const std::wstring inputNodeName)
{
    Variable inputVar;
    if (!GetInputVariableByName(m_pModel, inputNodeName, inputVar)) {
        fprintf(stderr, "Input variable %S is not available.\n", inputNodeName.c_str());
        throw("Input variable not found error.");
    }

    return inputVar.Shape().TotalSize();
}


//*************************************************************************************************************

size_t Deep::outputDimensions(const std::wstring outputNodeName)
{
    Variable outputVar;
    if (!GetOutputVaraiableByName(m_pModel, outputNodeName, outputVar)) {
        fprintf(stderr, "Output variable %S is not available.\n", outputNodeName.c_str());
        throw("Output variable not found error.");
    }

    return outputVar.Shape().TotalSize();
}


//*************************************************************************************************************

void Deep::runEvaluation(FunctionPtr model, const CNTK::Variable& inputVar, const ValuePtr& inputValue, const CNTK::Variable& outputVar, ValuePtr& outputValue, const DeviceDescriptor &device)
{
    std::unordered_map<Variable, ValuePtr> outputs = {{outputVar, outputValue}};
    model->Forward({{inputVar, inputValue}}, outputs, device);
    outputValue = outputs[outputVar];
}


//*************************************************************************************************************

void Deep::setModel(FunctionPtr &model)
{
    m_pModel = model;
}


//*************************************************************************************************************

bool Deep::loadModel(const QString& modelFileName, const DeviceDescriptor &device)
{
    QFile file(modelFileName);
    if(!file.exists()) {
        qCritical("Model filename (%s) does not exist.\n", modelFileName.toUtf8().constData());
        return false;
    }

    fprintf(stderr, "Loading model %s.\n",modelFileName.toUtf8().constData());

    m_pModel = Function::LoadModel(modelFileName.toStdWString(), device);

    return true;
}


//*************************************************************************************************************

bool Deep::saveModel(const QString &fileName)
{
    if(!m_pModel)
        return false;

    m_pModel->SaveModel(fileName.toStdWString());

    return true;
}


//*************************************************************************************************************

bool Deep::evalModel(const MatrixXf& input, MatrixXf& output, const DeviceDescriptor &device)
{
    OutputFunctionInfo(m_pModel);

    fprintf(stderr, "Evaluate model on device=%d\n", device.Id());

    // Run evaluation in parallel.
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

    //
    // Input
    //
    const std::wstring inputNodeName = L"features";

    Variable inputVar;
    if (!GetInputVariableByName(m_pModel, inputNodeName, inputVar)) {
        fprintf(stderr, "Input variable %S is not available.\n", inputNodeName.c_str());
        throw("Input variable not found error.");
    }

    //Check if input data size matches the number of features
    if (inputVar.Shape().TotalSize() != static_cast<size_t>(input.cols())) {
            fprintf(stderr, "Input data size: %d, do not match feature size: %d.\n", static_cast<int>(input.rows()), static_cast<int>(inputVar.Shape().TotalSize()));
            throw("Input data size do not match input feature size.");
    }

    // Evaluate the network in several runs
    size_t numSamples = static_cast<size_t>(input.rows());
    size_t numFeatures = static_cast<size_t>(input.cols());

    std::vector<float> inputData(numFeatures * numSamples);
    size_t dataIndex = 0;
    for (int m = 0; m < numSamples; ++m) {
        for (int n = 0; n < numFeatures; ++n) {
//            printf("%d: %f =? %f\n",dataIndex,input(m,n),static_cast<float>(dataIndex % 255));
            inputData[dataIndex++] = input(m,n);
        }
    }

//    NDShape inputShape = inputVar.Shape().AppendShape({1, numSamples});
//    ValuePtr inputValue = MakeSharedObject<Value>(MakeSharedObject<NDArrayView>(inputShape, inputData, true));
    ValuePtr inputValue = Value::CreateBatch(inputVar.Shape(), inputData, device);



    //
    // Output
    //
    const std::wstring outputNodeName = L"labels";//L"out.z";

    Variable outputVar;
    if (!GetOutputVaraiableByName(m_pModel, outputNodeName, outputVar)) {
        fprintf(stderr, "Output variable %S is not available.\n", outputNodeName.c_str());
        throw("Output variable not found error.");
    }

    ValuePtr outputValue;

    //
    // Evaluate
    //
    runEvaluation(m_pModel,inputVar,inputValue,outputVar,outputValue,device);

    //
    // Put the output together
    //
    NDShape outputShape = outputVar.Shape().AppendShape({1, numSamples});
    std::vector<float> outputData(outputShape.TotalSize());
    NDArrayViewPtr cpuArrayOutput = MakeSharedObject<NDArrayView>(outputShape, outputData, false);
    cpuArrayOutput->CopyFrom(*outputValue->Data());

    // consistency check
    assert(outputData.size() == outputVar.Shape()[0] * numSamples);

    dataIndex = 0;
    size_t outputDim = outputVar.Shape()[0];
    output.resize(numSamples, outputDim);
    for (size_t i = 0; i < numSamples; i++)
    {
        for (size_t j = 0; j < outputDim; j++)
        {
//            fprintf(stderr, "%f ", output(i,j));
            output(i,j) = outputData[dataIndex++];
        }
    }
    return true;
}























//*************************************************************************************************************

bool Deep::trainMinibatch(const Eigen::MatrixXf& input, const Eigen::MatrixXf& targets, double& loss, double& error, const CNTK::DeviceDescriptor& device)
{

    if(!m_pModel) {
        return false;
    }

    FunctionPtr z = m_pModel;

    //
    // Input
    //
    const std::wstring inputNodeName = L"features";
    Variable inputFeatures;
    if (!GetInputVariableByName(z, inputNodeName, inputFeatures)) {
        fprintf(stderr, "Input variable %S is not available.\n", inputNodeName.c_str());
        throw("Input variable not found error.");
    }

    //
    // Output
    //
    Variable outputLabels = z->Output();//InputVariable({z->Output().Shape().TotalSize()}, DataType::Float, L"labels"); //z->Output();

    FunctionPtr fctLoss = CrossEntropyWithSoftmax(z,outputLabels);
    FunctionPtr fctEvalError = ClassificationError(z, outputLabels);

    double learning_rate = 0.5;
    LearningRateSchedule lr_schedule = LearningRateSchedule(learning_rate, LearningRateSchedule::UnitType::Minibatch);
    std::vector<LearnerPtr> learner; learner.push_back(SGDLearner(z->Parameters(),lr_schedule));

    TrainerPtr trainer = CreateTrainer(z,fctLoss,fctEvalError,learner);

    //
    // Consistency Checks
    //
    size_t batchSize = 0;

    if(input.rows() == targets.rows() && input.rows() > 0) {
        batchSize = input.rows();
    }
    else {
        fprintf(stderr, "Sample size of features (%d) and targets (%d) do not match or are 0.\n", static_cast<int>(input.rows()), static_cast<int>(input.rows()));
        throw("Sample size do not match.");
    }

    if(input.cols() != inputFeatures.Shape().TotalSize()) {
        fprintf(stderr, "Input feature size (%d) do not match model feature size (%d).\n", static_cast<int>(input.cols()), static_cast<int>(inputFeatures.Shape().TotalSize()));
        throw("Sample size do not match.");
    }

    if(targets.cols() != outputLabels.Shape().TotalSize()) {
        fprintf(stderr, "Target size (%d) do not match model label size (%d).\n", static_cast<int>(targets.cols()), static_cast<int>(outputLabels.Shape().TotalSize()));
        throw("Target size do not match.");
    }

    //
    // Prepare data
    //
//    qDebug() << "inputFeatures.IsInput()" << inputFeatures.IsInput() << "inputFeatures.Shape().TotalSize()" << inputFeatures.Shape().TotalSize();
//    qDebug() << "outputLabels.IsOutput()" << outputLabels.IsOutput() << "outputLabels.Shape().TotalSize()" << outputLabels.Shape().TotalSize();

    size_t inputDim = inputFeatures.Shape().TotalSize();
    size_t numOutputClasses = outputLabels.Shape().TotalSize();

    std::vector<float> inputData(inputDim * batchSize);
    //    for (size_t i = 0; i < inputData.size(); ++i)
    //        inputData[i] = (float)rand() / RAND_MAX;
    size_t dataIndex = 0;
    for (int m = 0; m < batchSize; ++m) {
        for (int n = 0; n < inputDim; ++n) {
            inputData[dataIndex++] = input(m,n);
        }
    }
    ValuePtr inputDataValue = Value::CreateBatch(inputFeatures.Shape(), inputData, device);
    //    std::unordered_map<Variable, ValuePtr> inputValues = { { inputVar, inputDataValue } };

    std::vector<float> outputData(numOutputClasses * batchSize);
    //    for (size_t i = 0; i < outputData.size(); ++i)
    //        outputData[i] = (float)rand() / RAND_MAX;
    dataIndex = 0;
    for (int m = 0; m < batchSize; ++m) {
        for (int n = 0; n < numOutputClasses; ++n) {
            outputData[dataIndex++] = targets(m,n);
        }
    }
    ValuePtr outputDataValue = Value::CreateBatch(outputLabels.Shape(), outputData, device);//    z->Output().Shape()
    //    std::unordered_map<Variable, ValuePtr> outputValues = { { outputLabels, outputDataValue } };

    std::unordered_map<Variable, ValuePtr> inOutValues = { { inputFeatures, inputDataValue }, { outputLabels, outputDataValue } };

    //
    // Train the minibatch
    //
    trainer->TrainMinibatch(inOutValues,device);

    loss = trainer->PreviousMinibatchLossAverage();
    error = trainer->PreviousMinibatchEvaluationAverage();
//    size_t minibatch_samples = trainer->PreviousMinibatchSampleCount();
//    qDebug() << "Finished minibatch training: loss" << loss << "; error" << error << "; samples" << minibatch_samples;

    return true;
}


//*************************************************************************************************************

void Deep::OutputFunctionInfo(FunctionPtr model)
{
    auto inputVariables = model->Arguments();
    fprintf(stderr, "Function '%S': Input Variables (count=%lu)\n", model->Name().c_str(), (unsigned long)inputVariables.size());
    for_each(inputVariables.begin(), inputVariables.end(), [](const Variable v) {
        fprintf(stderr, "    name=%S, kind=%d\n", v.Name().c_str(), static_cast<int>(v.Kind()));
    });

    auto outputVariables = model->Outputs();
    fprintf(stderr, "Function '%S': Output Variables (count=%lu)\n", model->Name().c_str(), (unsigned long)outputVariables.size());
    for_each(outputVariables.begin(), outputVariables.end(), [](const Variable v) {
        fprintf(stderr, "    name=%S, kind=%d\n", v.Name().c_str(), static_cast<int>(v.Kind()));
    });
}


//*************************************************************************************************************

bool Deep::GetVariableByName(std::vector<Variable> variableLists, std::wstring varName, Variable &var)
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

bool Deep::GetInputVariableByName(FunctionPtr model, std::wstring varName, Variable &var)
{
    return GetVariableByName(model->Arguments(), varName, var);
}


//*************************************************************************************************************

bool Deep::GetOutputVaraiableByName(FunctionPtr model, std::wstring varName, Variable &var)
{
    return GetVariableByName(model->Outputs(), varName, var);
}
