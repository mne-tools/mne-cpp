//=============================================================================================================
/**
* @file     deepmodelcreator.h
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
* @brief    DeepModelCreator class declaration.
*
*/

#ifndef DEEPMODELCREATOR_H
#define DEEPMODELCREATOR_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deep_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// CNTK INCLUDES
//=============================================================================================================

#include <CNTKLibrary.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DEEPLIB
//=============================================================================================================

namespace DEEPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
* CNTK Model Creator
*
* @brief CNTK Model Creator
*/
class DEEPSHARED_EXPORT DeepModelCreator
{
public:
    typedef QSharedPointer<DeepModelCreator> SPtr;            /**< Shared pointer type for DeepModelCreator. */
    typedef QSharedPointer<const DeepModelCreator> ConstSPtr; /**< Const shared pointer type for DeepModelCreator. */

    //=========================================================================================================
    /**
    * Default constructor
    */
    DeepModelCreator();

    //=========================================================================================================
    /**
    * Destructs DeepModelCreator
    */
    virtual ~DeepModelCreator();


//    //=========================================================================================================
//    /**
//    * Setup fully connected linear layer
//    *
//    * @param [in] input
//    * @param [in] outputDim
//    * @param [in] device
//    * @param [in] outputName
//    */
//    static CNTK::FunctionPtr SetupFullyConnectedLinearLayer(CNTK::Variable input, size_t outputDim, const CNTK::DeviceDescriptor &device, const std::wstring &outputName = L"")
//    {
//        assert(input.Shape().Rank() == 1);
//        size_t inputDim = input.Shape()[0];

//        auto timesParam = CNTK::Parameter(CNTK::NDArrayView::RandomUniform<float>({outputDim, inputDim}, -0.05, 0.05, 1, device));
//        auto timesFunction = CNTK::Times(timesParam, input);

//        auto plusParam = CNTK::Parameter(CNTK::NDArrayView::RandomUniform<float>({outputDim}, -0.05, 0.05, 1, device));
//        return CNTK::Plus(plusParam, timesFunction, outputName);
//    }


//    //=========================================================================================================
//    /**
//    * Setup fully connected deep neural network layer
//    *
//    * @param [in] input
//    * @param [in] outputDim
//    * @param [in] device
//    * @param [in] nonLinearity
//    */
//    static CNTK::FunctionPtr SetupFullyConnectedDNNLayer(CNTK::Variable input, size_t outputDim, const CNTK::DeviceDescriptor &device, const std::function<CNTK::FunctionPtr (const CNTK::FunctionPtr &)> &nonLinearity)
//    {
//        return nonLinearity(SetupFullyConnectedLinearLayer(input, outputDim, device));
//    }




    static CNTK::FunctionPtr FullyConnectedFeedForwardClassifierNet(CNTK::Variable input,
                                                                    size_t numHiddenLayers,
                                                                    const CNTK::Parameter& inputTimesParam,
                                                                    const CNTK::Parameter& inputPlusParam,
                                                                    const CNTK::Parameter hiddenLayerTimesParam[],
                                                                    const CNTK::Parameter hiddenLayerPlusParam[],
                                                                    const CNTK::Parameter& outputTimesParam,
                                                                    const std::function<CNTK::FunctionPtr(const CNTK::FunctionPtr&)>& nonLinearity);


private:
    static CNTK::FunctionPtr FullyConnectedDNNLayer(CNTK::Variable input,
                                                    const CNTK::Parameter& timesParam,
                                                    const CNTK::Parameter& plusParam,
                                                    const std::function<CNTK::FunctionPtr(const CNTK::FunctionPtr&)>& nonLinearity);


// Pre-Configured networks
public:
    static CNTK::FunctionPtr DNN_1( const size_t inputDim, const size_t numOutputClasses, const CNTK::DeviceDescriptor& device = CNTK::DeviceDescriptor::DefaultDevice());


};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // DEEPMODELCREATOR_H
