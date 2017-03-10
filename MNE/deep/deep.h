//=============================================================================================================
/**
* @file     deep.h
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
* @brief    Deep class declaration.
*
*/

#ifndef DEEP_H
#define DEEP_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deep_global.h"


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
* Deep cntk wrapper descritpion
*
* @brief Deep cntk wrapper to train and evaluate models
*/
class DEEPSHARED_EXPORT Deep
{
public:
    typedef QSharedPointer<Deep> SPtr;            /**< Shared pointer type for Deep. */
    typedef QSharedPointer<const Deep> ConstSPtr; /**< Const shared pointer type for Deep. */

    //=========================================================================================================
    /**
    * Default constructor
    */
    Deep();

    //=========================================================================================================
    /**
    * Destructs Deep
    */
    virtual ~Deep();

    //=========================================================================================================
    /**
    * Whether the GPU should be used
    *
    * @param [in] useGPU    Flag if GPU should be used.
    */
    void setUseGPU(bool useGPU);

    //=========================================================================================================
    /**
    * Evaluate the network in several runs
    *
    * @param [in] evalFunc      Function to evaluate
    * @param [in] device        Device to use
    */
    static void RunEvaluationClassifier(CNTK::FunctionPtr evalFunc, const CNTK::DeviceDescriptor& device);

    //=========================================================================================================
    /**
    * Print the ouput function info to stderr stream
    *
    * @param [in] func      Function to evaluate
    */
    static void OutputFunctionInfo(CNTK::FunctionPtr func);

    //=========================================================================================================
    /**
    * Setup fully connected linear layer
    *
    * @param [in] input
    * @param [in] outputDim
    * @param [in] device
    * @param [in] outputName
    */
    inline static CNTK::FunctionPtr SetupFullyConnectedLinearLayer(CNTK::Variable input, size_t outputDim, const CNTK::DeviceDescriptor& device, const std::wstring& outputName = L"");

    //=========================================================================================================
    /**
    * Setup fully connected deep neural network layer
    *
    * @param [in] input
    * @param [in] outputDim
    * @param [in] device
    * @param [in] nonLinearity
    */
    inline static CNTK::FunctionPtr SetupFullyConnectedDNNLayer(CNTK::Variable input, size_t outputDim, const CNTK::DeviceDescriptor& device, const std::function<CNTK::FunctionPtr(const CNTK::FunctionPtr&)>& nonLinearity);

    //=========================================================================================================
    /**
    * Shows how to use Clone() to share function parameters among multi evaluation threads.
    *
    * It first creates a new function with parameters, then spawns multi threads. Each thread uses Clone() to create a new
    * instance of function and then use this instance to do evaluation.
    * All cloned functions share the same parameters.
    *
    * @param [in] device
    * @param [in] threadCount   Numbers of threads to use
    */
    void MultiThreadsEvaluationWithClone(const CNTK::DeviceDescriptor& device, const int threadCount);


    void testClone();


protected:
    //=========================================================================================================
    /**
    * Searches for a varibale by name. Returns true when variable was found.
    *
    * @param [in] variableLists     List of variables to search for the variable by name
    * @param [in] varName           Name of variable to find
    * @param [out] var              The variable, if name was found
    *
    * @return true when variable was found, false otherwise.
    */
    static bool GetVariableByName(std::vector<CNTK::Variable> variableLists, std::wstring varName, CNTK::Variable& var)
    {
        for (std::vector<CNTK::Variable>::iterator it = variableLists.begin(); it != variableLists.end(); ++it)
        {
            if (it->Name().compare(varName) == 0)
            {
                var = *it;
                return true;
            }
        }
        return false;
    }

    //=========================================================================================================
    /**
    * Searches for a input variable by name. Returns true when found
    *
    * @param [in] evalFunc          Input function to search for the respective input variable.
    * @param [in] varName           Name of variable to find
    * @param [out] var              The variable, if name was found
    *
    * @return true when variable was found, false otherwise.
    */
    inline static bool GetInputVariableByName(CNTK::FunctionPtr evalFunc, std::wstring varName, CNTK::Variable& var)
    {
        return GetVariableByName(evalFunc->Arguments(), varName, var);
    }

private:
    bool m_bUseGPU = false;     /**< Should the GPU be used? */

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // DEEP_H
