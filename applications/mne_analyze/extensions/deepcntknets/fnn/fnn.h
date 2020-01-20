//=============================================================================================================
/**
 * @file     fnn.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the FNN class.
 *
 */

#ifndef FNN_H
#define FNN_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fnn_global.h"

#include "../../deepcntk/IDeepCNTKNet.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DEEPLIB
{
    class Deep;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FNNCONFIGURATION
//=============================================================================================================

namespace FNNCONFIGURATION
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
 * DeepCNTK Extension
 *
 * @brief The DeepCNTK class provides a Machine Learning Capbilities.
 */
class FNNSHARED_EXPORT FNN : public DEEPCNTKEXTENSION::IDeepCNTKNet
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "deepcntkextension/1.0" FILE "fnn.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(DEEPCNTKEXTENSION::IDeepCNTKNet)

public:
    //=========================================================================================================
    /**
    * Constructs a FNN.
    */
    FNN();

    //=========================================================================================================
    /**
    * Destroys the FNN.
    */
    ~FNN();

    //=========================================================================================================
    /**
    * Initializes the network configuration.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Is called when network configuration unloaded.
    */
    virtual void unload();

    //=========================================================================================================
    /**
    * Returns the CNTK model.
    *
    * @return the CNTK model.
    */
    virtual QSharedPointer<DEEPLIB::Deep> getModel() const;

    //=========================================================================================================
    /**
    * Returns the network configuration name.
    *
    * @return the name of the configuration.
    */
    virtual QString getName() const;

    //=========================================================================================================
    /**
    * Trains the network configuration.
    */
    virtual void train();

    //=========================================================================================================
    /**
    * Evaluates the network configuration.
    */
    virtual void eval();

private:
    // Deep Model
    QSharedPointer<DEEPLIB::Deep>   m_pDeep;            /**< CNTK Wrapper */
};

} // NAMESPACE

#endif // FNN_H
