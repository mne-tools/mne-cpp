//=============================================================================================================
/**
* @file     deepeval.h
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
* @brief    DeepEval class declaration.
*
*/

#ifndef DEEPEVAL_H
#define DEEPEVAL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deep_global.h"


//*************************************************************************************************************
//=============================================================================================================
// CNTK INCLUDES
//=============================================================================================================

#include <Eval.h>
#include <CNTKLibrary.h>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DEEPLIB
//=============================================================================================================

namespace DEEPLIB
{

//=============================================================================================================
/**
* DeepEval cntk wrapper descritpion
*
* @brief DeepEval cntk wrapper to evaluate pretrained models
*/
class DEEPSHARED_EXPORT DeepEval
{
public:
    typedef QSharedPointer<DeepEval> SPtr;            /**< Shared pointer type for DeepEval. */
    typedef QSharedPointer<const DeepEval> ConstSPtr; /**< Const shared pointer type for DeepEval. */

    //=========================================================================================================
    /**
    * Default constructor
    */
    DeepEval();

    //=========================================================================================================
    /**
    * Constructor
    *
    * @param [in] sModelFilename    The model filename to set
    */
    DeepEval(const QString &sModelFilename);

    //=========================================================================================================
    /**
    * Destructs DeepEval
    */
    virtual ~DeepEval();

    //=========================================================================================================
    /**
    * Returns the current set model file name
    *
    * @return the current model file name
    */
    const QString& getModelFilename() const;

    //=========================================================================================================
    /**
    * Set the model filename
    *
    * @param [in] sModelFilename    The model filename to set
    */
    void setModelFilename(const QString &sModelFilename);

    //=========================================================================================================
    /**
    * Evaluate the MNE Deep Model specified by the model file name
    *
    * @param [in] inputs    The input vector
    * @param [in] outputs   The ouptut vector
    *
    * @return true when MNE Deep model was sucessfully evaluated.
    */
    bool evalModel(const Eigen::VectorXf& inputs, Eigen::VectorXf& outputs);

    //=========================================================================================================
    /**
    * Evaluate the MNE Deep Model specified by the model file name
    *
    * @param [in] inputs    The input vector
    * @param [in] outputs   The ouptut vector
    *
    * @return true when MNE Deep model was sucessfully evaluated.
    */
    bool evalModel(std::vector<float>& inputs, std::vector<float>& outputs);

    //=========================================================================================================
    /**
    * Loads the Deep Model set by the model file name
    *
    * @return true when MNE Deep model was sucessfully loaded.
    */
    bool loadModel();

    //=========================================================================================================
    /**
    * Returns the Input Dimensions
    *
    * @return the Input dimensions
    */
    size_t inputDimensions();

    //=========================================================================================================
    /**
    * Returns the Output Dimensions
    *
    * @return the Output dimensions
    */
    size_t outputDimensions();



    static bool GetVariableByName(std::vector<CNTK::Variable> variableLists, std::wstring varName, CNTK::Variable& var);

    inline static bool GetInputVariableByName(CNTK::FunctionPtr evalFunc, std::wstring varName, CNTK::Variable& var);

    inline static bool GetOutputVaraiableByName(CNTK::FunctionPtr evalFunc, std::wstring varName, CNTK::Variable& var);


    static void runEvaluation(CNTK::FunctionPtr evalFunc, const CNTK::DeviceDescriptor& device, const CNTK::Variable& inputVar, const CNTK::ValuePtr& inputValue, const CNTK::Variable& outputVar, CNTK::ValuePtr& outputValue);

    void OutputFunctionInfo(CNTK::FunctionPtr func);



    /// <summary>
    /// Shows how to use LoadLegacyModel() and Clone() to share function parameters among multi evaluation threads.
    /// </summary>
    /// <description>
    /// It first loads a model, then spawns multi threads. Each thread uses Clone() to create a new
    /// instance of function and then use this instance to do evaluation.
    /// All cloned functions share the same parameters.
    /// Note: It uses the model trained by Examples\Image\GettingStarted\01_OneHidden.cntk as example. Instructions
    /// to train the model is described in Examples\Image\GettingStarted\README.md.
    /// The pre-trained model file 01_OneHidden needs to be in the current directory.
    /// </description>
    void loadModel_v2(const CNTK::DeviceDescriptor &device);

    void evalModel_v2(const CNTK::DeviceDescriptor& device, const int threadCount);

    void testEval();

private:
    QString m_sModelFilename;       /**< Model filename */

    Microsoft::MSR::CNTK::IEvaluateModel<float>* m_pModel_v1;  /**< The loaded model */

    CNTK::FunctionPtr m_pModelFunction_v2;  /**< The loaded v2 model */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // DEEPEVAL_H
