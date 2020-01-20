//=============================================================================================================
/**
 * @file     deepmodelcreator.cpp
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
 * @brief    DeepModelCreator class implementation.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

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

DeepModelCreator::DeepModelCreator()
{
}


//*************************************************************************************************************

DeepModelCreator::~DeepModelCreator()
{

}


//*************************************************************************************************************

FunctionPtr DeepModelCreator::fullyConnectedFeedForwardClassifierNet(Variable input, size_t numHiddenLayers, const Parameter &inputTimesParam, const Parameter &inputPlusParam, const Parameter hiddenLayerTimesParam[], const Parameter hiddenLayerPlusParam[], const Parameter &outputTimesParam, const std::function<FunctionPtr (const FunctionPtr &)> &nonLinearity)
{
    assert(numHiddenLayers >= 1);
    FunctionPtr h = fullyConnectedDNNLayer(input, inputTimesParam, inputPlusParam, nonLinearity);

    for (size_t i = 1; i < numHiddenLayers; ++i) {
        h = fullyConnectedDNNLayer(h, hiddenLayerTimesParam[i - 1], hiddenLayerPlusParam[i - 1], nonLinearity);
    }

    // Todo: assume that outputTimesParam has matched output dim and hiddenLayerDim
    h = Times(outputTimesParam, h, L"labels");// Output
    return h;
}


//*************************************************************************************************************

FunctionPtr DeepModelCreator::fullyConnectedDNNLayer(Variable input, const Parameter &timesParam, const Parameter &plusParam, const std::function<FunctionPtr (const FunctionPtr &)> &nonLinearity)
{
    assert(input.Shape().Rank() == 1);

    // Todo: assume that timesParam has matched outputDim and inputDim
    auto timesFunction = Times(timesParam, input);

    // Todo: assume that timesParam has matched outputDim
    auto plusFunction = Plus(plusParam, timesFunction);

    return nonLinearity(plusFunction);
}


//*************************************************************************************************************
//=============================================================================================================
// Network definitions
//=============================================================================================================

FunctionPtr DeepModelCreator::FFN_1(const size_t inputDim, const size_t numOutputClasses, const DeviceDescriptor &device)
{
    const size_t numHiddenLayers = 2;
    const size_t hiddenLayersDim = 50;

    // Define model parameters that should be shared among evaluation requests against the same model
    Parameter inputTimesParam = Parameter(NDArrayView::RandomUniform<float>({hiddenLayersDim, inputDim}, -0.5, 0.5, 1, device));
    Parameter inputPlusParam = Parameter({hiddenLayersDim}, 0.0f, device);
    Parameter hiddenLayerTimesParam[numHiddenLayers - 1] = {
        Parameter(NDArrayView::RandomUniform<float>({hiddenLayersDim, hiddenLayersDim}, -0.5, 0.5, 1, device))
    };

    Parameter hiddenLayerPlusParam[numHiddenLayers - 1] = {
        Parameter({hiddenLayersDim}, 0.0f, device)
    };

    Parameter outputTimesParam = Parameter(NDArrayView::RandomUniform<float>({numOutputClasses, hiddenLayersDim}, -0.5, 0.5, 1, device));

    Variable inputVar = InputVariable({inputDim}, DataType::Float, L"features");

    return fullyConnectedFeedForwardClassifierNet(  inputVar,
                                                    numHiddenLayers,
                                                    inputTimesParam,
                                                    inputPlusParam,
                                                    hiddenLayerTimesParam,
                                                    hiddenLayerPlusParam,
                                                    outputTimesParam,
                                                    std::bind(Sigmoid, std::placeholders::_1, L""));
}


//*************************************************************************************************************

FunctionPtr DeepModelCreator::DNN_1(const size_t inputDim, const size_t numOutputClasses, const DeviceDescriptor& device)
{
    const size_t numHiddenLayers = 6;
    const size_t hiddenLayersDim = 10;

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
        Parameter({hiddenLayersDim}, 0.0f, device)
    };
    Parameter outputTimesParam = Parameter(NDArrayView::RandomUniform<float>({numOutputClasses, hiddenLayersDim}, -0.5, 0.5, 1, device));

    Variable inputVar = InputVariable({inputDim}, DataType::Float, L"features");

    return fullyConnectedFeedForwardClassifierNet(  inputVar,
                                                    numHiddenLayers,
                                                    inputTimesParam,
                                                    inputPlusParam,
                                                    hiddenLayerTimesParam,
                                                    hiddenLayerPlusParam,
                                                    outputTimesParam,
                                                    std::bind(Sigmoid, std::placeholders::_1, L""));

}
