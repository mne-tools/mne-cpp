//=============================================================================================================
/**
* @file     IDeepCNTKNet.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains declaration of IDeepCNTKNet interface class.
*
*/

#ifndef IDEEPCNTKNET_H
#define IDEEPCNTKNET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <deep/IDeepConfiguration.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DEEPCNTKEXTENSION
//=============================================================================================================

namespace DEEPCNTKEXTENSION
{


//=============================================================================================================
/**
* DECLARE CLASS IDeepCNTKNet
*
* @brief The IDeepCNTKNet class provides an interface for a CNTK network configurations.
*/
class IDeepCNTKNet : public DEEPLIB::IDeepConfiguration
{
public:
    typedef QSharedPointer<IDeepCNTKNet> SPtr;               /**< Shared pointer type for IDeepCNTKNet. */
    typedef QSharedPointer<const IDeepCNTKNet> ConstSPtr;    /**< Const shared pointer type for IDeepCNTKNet. */

    //=========================================================================================================
    /**
    * Destroys the IDeepCNTKNet.
    */
    virtual ~IDeepCNTKNet() {}

    //=========================================================================================================
    /**
    * Initializes the network configuration.
    */
    virtual void init() = 0;

    //=========================================================================================================
    /**
    * Is called when network configuration unloaded.
    */
    virtual void unload() = 0;

    //=========================================================================================================
    /**
    * Returns the network configuration name.
    * Pure virtual method.
    *
    * @return the name of the configuration.
    */
    virtual QString getName() const = 0;

    //=========================================================================================================
    /**
    * Trains the network configuration.
    * Pure virtual method.
    */
    virtual void train() = 0;

    //=========================================================================================================
    /**
    * Evaluates the network configuration.
    * Pure virtual method.
    */
    virtual void eval() = 0;

protected:

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

Q_DECLARE_INTERFACE(DEEPCNTKEXTENSION::IDeepCNTKNet, "deepcntkextension/1.0")

#endif // IDEEPCNTKNET_H
