//=============================================================================================================
/**
 * @file     deepeval.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Christoph Dinh. All rights reserved.
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
