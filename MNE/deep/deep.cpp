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
//        NDShape inputShape = inputVar.Shape().AppendShape({1, numSamples});
//        ValuePtr inputValue = MakeSharedObject<Value>(MakeSharedObject<NDArrayView>(inputShape, inputData, true));
        ValuePtr inputValue = Value::CreateBatch(inputVar.Shape(), inputData, device);


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


//*************************************************************************************************************

void Deep::exampleTrain()
{

    DeviceDescriptor device = DeviceDescriptor::CPUDevice();

    size_t outDim = 15;

    size_t inDim = 10;

    auto W = Parameter(NDShape({ outDim, inDim }), DataType::Float, GlorotUniformInitializer(), device);

    auto x = InputVariable(NDShape({ inDim }), DataType::Float, { Axis::DefaultBatchAxis() });


    size_t batchSize = 3;

    std::vector<float> inputData(inDim * batchSize);

    for (size_t i = 0; i < inputData.size(); ++i)
        inputData[i] = (float)rand() / RAND_MAX;

    auto inputDataValue = Value::CreateBatch(x.Shape(), inputData, device);

    std::vector<float> rootGradientData(outDim * batchSize, 1);


    //    auto userDefinedTimes = UserTimesFunction::Create(W, x, L"UserDefinedTimes");
    //    auto rootGradientValue = Value::CreateBatch(userDefinedTimes->Output().Shape(), rootGradientData, device);

//    std::unordered_map<Variable, ValuePtr> outputValues = { { userDefinedTimes->Output(), nullptr } };
//    auto backPropState = userDefinedTimes->Forward({ { x, inputDataValue } }, outputValues, device, { userDefinedTimes->Output() });

//    std::unordered_map<Variable, ValuePtr> inputGradientValues = { { W, nullptr } };

//    userDefinedTimes->Backward(backPropState, { { userDefinedTimes->Output(), rootGradientValue } }, inputGradientValues);
//    auto userDefinedTimesOutputValue = outputValues[userDefinedTimes->Output()];
//    auto userDefinedTimesInputGradientValue = inputGradientValues[W];


    // Compare against the CNTK built-in implementation

    auto builtInTimes = Times(W, x, L"BuiltInTimes");

    auto rootGradientValue = Value::CreateBatch(builtInTimes->Output().Shape(), rootGradientData, device);


    std::unordered_map<Variable, ValuePtr> outputValues = { { builtInTimes->Output(), nullptr } };

    auto backPropState = builtInTimes->Forward({ { x, inputDataValue } }, outputValues, device, { builtInTimes->Output() });

    std::unordered_map<Variable, ValuePtr> inputGradientValues = { { W, nullptr } };

    builtInTimes->Backward(backPropState, { { builtInTimes->Output(), rootGradientValue } }, inputGradientValues);

    auto builtInTimesOutputValue = outputValues[builtInTimes->Output()];

    auto builtInTimesInputGradientValue = inputGradientValues[W];



//    const double relativeTolerance = 0.001f;

//    const double absoluteTolerance = 0.000001f;



//    if (!Internal::AreEqual(*userDefinedTimesOutputValue, *builtInTimesOutputValue, relativeTolerance, absoluteTolerance))

//        std::runtime_error("UserTimesOp's Forward result does not match built-in result");



//    if (!Internal::AreEqual(*userDefinedTimesInputGradientValue, *builtInTimesInputGradientValue, relativeTolerance, absoluteTolerance))

//        std::runtime_error("UserTimesOp's Forward result does not match built-in result");

}


//*************************************************************************************************************

size_t Deep::inputDimensions()
{
    const std::wstring inputNodeName = L"features";

    Variable inputVar;
    if (!GetInputVariableByName(m_pModel, inputNodeName, inputVar)) {
        fprintf(stderr, "Input variable %S is not available.\n", inputNodeName.c_str());
        throw("Input variable not found error.");
    }

    return inputVar.Shape().TotalSize();
}


//*************************************************************************************************************

size_t Deep::outputDimensions()
{
    const std::wstring outputNodeName = L"out.z";

    Variable outputVar;
    if (!GetOutputVaraiableByName(m_pModel, outputNodeName, outputVar)) {
        fprintf(stderr, "Output variable %S is not available.\n", outputNodeName.c_str());
        throw("Output variable not found error.");
    }

    return outputVar.Shape().TotalSize();
}


//*************************************************************************************************************

void Deep::runEvaluation(FunctionPtr model, const DeviceDescriptor &device, const CNTK::Variable& inputVar, const ValuePtr& inputValue, const CNTK::Variable& outputVar, ValuePtr& outputValue)
{
    std::unordered_map<Variable, ValuePtr> outputs = {{outputVar, outputValue}};
    model->Forward({{inputVar, inputValue}}, outputs, device);
    outputValue = outputs[outputVar];
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

bool Deep::evalModel(const DeviceDescriptor &device, const MatrixXf& input, MatrixXf& output)
{
    OutputFunctionInfo(m_pModel);

    fprintf(stderr, "Evaluate model on device=%d\n", device.Id());

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
    const std::wstring outputNodeName = L"out.z";

    Variable outputVar;
    if (!GetOutputVaraiableByName(m_pModel, outputNodeName, outputVar)) {
        fprintf(stderr, "Output variable %S is not available.\n", outputNodeName.c_str());
        throw("Output variable not found error.");
    }

    ValuePtr outputValue;

    //
    // Evaluate
    //
    runEvaluation(m_pModel,device,inputVar,inputValue,outputVar,outputValue);

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

FunctionPtr Deep::FullyConnectedDNNLayerWithSharedParameters(Variable input, const Parameter &timesParam, const Parameter &plusParam, const std::function<FunctionPtr (const FunctionPtr &)> &nonLinearity)
{
    assert(input.Shape().Rank() == 1);

    // Todo: assume that timesParam has matched outputDim and inputDim
    auto timesFunction = Times(timesParam, input);

    // Todo: assume that timesParam has matched outputDim
    auto plusFunction = Plus(plusParam, timesFunction);

    return nonLinearity(plusFunction);
}


//*************************************************************************************************************

FunctionPtr Deep::FullyConnectedFeedForwardClassifierNetWithSharedParameters(Variable input, size_t numHiddenLayers, const Parameter &inputTimesParam, const Parameter &inputPlusParam, const Parameter hiddenLayerTimesParam[], const Parameter hiddenLayerPlusParam[], const Parameter &outputTimesParam, const std::function<FunctionPtr (const FunctionPtr &)> &nonLinearity)
{
    assert(numHiddenLayers >= 1);
    auto classifierRoot = FullyConnectedDNNLayerWithSharedParameters(input, inputTimesParam, inputPlusParam, nonLinearity);

    for (size_t i = 1; i < numHiddenLayers; ++i)
    {
        classifierRoot = FullyConnectedDNNLayerWithSharedParameters(classifierRoot, hiddenLayerTimesParam[i - 1], hiddenLayerPlusParam[i - 1], nonLinearity);
    }

    // Todo: assume that outputTimesParam has matched output dim and hiddenLayerDim
    classifierRoot = Times(outputTimesParam, classifierRoot);
    return classifierRoot;
}


//*************************************************************************************************************

bool Deep::trainModel()
{
    QString fileName("./mne_deep_models/trainModel.v2");


    DeviceDescriptor device = DeviceDescriptor::CPUDevice();

    const size_t inputDim = 937;
    const size_t numOutputClasses = 9304;
    const size_t numHiddenLayers = 6;
    const size_t hiddenLayersDim = 2048;

    // Define model parameters that should be shared among evaluation requests against the same model
    auto inputTimesParam = Parameter(NDArrayView::RandomUniform<float>({hiddenLayersDim, inputDim}, -0.5, 0.5, 1, device));
    auto inputPlusParam = Parameter({hiddenLayersDim}, 0.0f, device);
    Parameter hiddenLayerTimesParam[numHiddenLayers - 1] = {
        Parameter(NDArrayView::RandomUniform<float>({hiddenLayersDim, hiddenLayersDim}, -0.5, 0.5, 1, device)),
        Parameter(NDArrayView::RandomUniform<float>({hiddenLayersDim, hiddenLayersDim}, -0.5, 0.5, 1, device)),
        Parameter(NDArrayView::RandomUniform<float>({hiddenLayersDim, hiddenLayersDim}, -0.5, 0.5, 1, device)),
        Parameter(NDArrayView::RandomUniform<float>({hiddenLayersDim, hiddenLayersDim}, -0.5, 0.5, 1, device)),
        Parameter(NDArrayView::RandomUniform<float>({hiddenLayersDim, hiddenLayersDim}, -0.5, 0.5, 1, device))
    };
    Parameter hiddenLayerPlusParam[numHiddenLayers - 1] = {
        Parameter({hiddenLayersDim}, 0.0f, device),
        Parameter({hiddenLayersDim}, 0.0f, device),
        Parameter({hiddenLayersDim}, 0.0f, device),
        Parameter({hiddenLayersDim}, 0.0f, device),
        Parameter({hiddenLayersDim}, 0.0f, device),
    };
    auto outputTimesParam = Parameter(NDArrayView::RandomUniform<float>({numOutputClasses, hiddenLayersDim}, -0.5, 0.5, 1, device));





    auto inputVar = InputVariable({inputDim}, DataType::Float, L"Features");

//    if(!loadModel(fileName, device)) {
        fprintf(stderr, "Constructing model %s.\n",fileName.toUtf8().constData());

        m_pModel = FullyConnectedFeedForwardClassifierNetWithSharedParameters(  inputVar,
                                                                                numHiddenLayers,
                                                                                inputTimesParam,
                                                                                inputPlusParam,
                                                                                hiddenLayerTimesParam,
                                                                                hiddenLayerPlusParam,
                                                                                outputTimesParam,
                                                                                std::bind(Sigmoid, std::placeholders::_1, L""));
//    }

    FunctionPtr z = m_pModel;

    Variable labels = InputVariable({numOutputClasses}, DataType::Float, L"Labels");
    FunctionPtr loss = CrossEntropyWithSoftmax(z,labels);
    FunctionPtr eval_error = ClassificationError(z, labels);

    double learning_rate = 0.5;
    LearningRateSchedule lr_schedule = LearningRateSchedule(learning_rate, LearningRateSchedule::UnitType::Minibatch);
    std::vector<LearnerPtr> learner; learner.push_back(SGDLearner(z->Parameters(),lr_schedule));

    TrainerPtr trainer = CreateTrainer(z,loss,eval_error,learner);

    z->SaveModel(fileName.toStdWString());
















    size_t batchSize = 1;

    std::vector<float> inputData(inputDim * batchSize);
    for (size_t i = 0; i < inputData.size(); ++i)
        inputData[i] = (float)rand() / RAND_MAX;

//    for (int m = 0; m < batchSize; ++m) {
//        for (int n = 0; n < inputDim; ++n) {
//            printf("%d: %f =? %f\n",dataIndex,input(m,n),static_cast<float>(dataIndex % 255));
//            inputData[dataIndex++] = input(m,n);
//        }
//    }

    ValuePtr inputDataValue = Value::CreateBatch(inputVar.Shape(), inputData, device);
//    std::unordered_map<Variable, ValuePtr> inputValues = { { inputVar, inputDataValue } };


    std::vector<float> outputData(numOutputClasses * batchSize, 1);
    for (size_t i = 0; i < outputData.size(); ++i)
        outputData[i] = (float)rand() / RAND_MAX;

//    z->Output().Shape()
    ValuePtr outputDataValue = Value::CreateBatch(labels.Shape(), outputData, device);
//    std::unordered_map<Variable, ValuePtr> outputValues = { { labels, outputDataValue } };


    std::unordered_map<Variable, ValuePtr> inOutValues = { { inputVar, inputDataValue }, { labels, outputDataValue } };

    qDebug() << "Before Training";

    trainer->TrainMinibatch(inOutValues,device);



    double training_loss_val = trainer->PreviousMinibatchLossAverage();

    double eval_error_val = trainer->PreviousMinibatchEvaluationAverage();

    size_t minibatch_samples = trainer->PreviousMinibatchSampleCount();


    qDebug() << "training_loss_val" << training_loss_val << "; eval_error_val" << eval_error_val << "; minibatch_samples" << minibatch_samples;




    qDebug() << "After Training";

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
