//=============================================================================================================
/**
 * @file     IDeepCNTKNet.h
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
 * @brief    Contains declaration of IDeepCNTKNet interface class.
 *
 */

#ifndef IDEEPCNTKNET_H
#define IDEEPCNTKNET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deepcntk_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QObject>
#include <QSharedPointer>


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
// DEFINE NAMESPACE DEEPCNTKEXTENSION
//=============================================================================================================

namespace DEEPCNTKEXTENSION
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=========================================================================================================
/**
 * DECLARE CLASS IDeepCNTKNet
 *
 * @brief The IDeepCNTKNet class is the base interface class for all deep network configurations.
 */
class DEEPCNTKSHARED_EXPORT IDeepCNTKNet : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<IDeepCNTKNet> SPtr;               /**< Shared pointer type for IDeepCNTKNet. */
    typedef QSharedPointer<const IDeepCNTKNet> ConstSPtr;    /**< Const shared pointer type for IDeepCNTKNet. */

    //=========================================================================================================
    /**
     * Destroys the network configuration.
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
     * Returns the CNTK model.
     * Pure virtual method.
     *
     * @return the CNTK model.
     */
    virtual QSharedPointer<DEEPLIB::Deep> getModel() const = 0;

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

    //=========================================================================================================
    /**
     * Returns the loss of the current training session.
     *
     * @return the loss of the current training.
     */
    inline QVector<double> &currentLoss();

    //=========================================================================================================
    /**
     * Returns the loss over all training session.
     *
     * @return the loss over all training session.
     */
    inline QVector<double> &overallLoss();

    //=========================================================================================================
    /**
     * Returns the error of the current training session.
     *
     * @return the error of the current training.
     */
    inline QVector<double> &currentError();

    //=========================================================================================================
    /**
     * Returns the error over all training session.
     *
     * @return the error over all training session.
     */
    inline QVector<double> &overallError();

private:
    QVector<double> m_CurrentLoss;      /**< The Loss of the current training session */
    QVector<double> m_OverallLoss;      /**< The Loss over all training sessions */

    QVector<double> m_CurrentError;     /**< The Error of the current training session */
    QVector<double> m_OverallError;     /**< The Error over all training sessions */

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

QVector<double> &IDeepCNTKNet::currentLoss()
{
    return m_CurrentLoss;
}


//*************************************************************************************************************

QVector<double> &IDeepCNTKNet::overallLoss()
{
    return m_OverallLoss;
}


//*************************************************************************************************************

QVector<double> &IDeepCNTKNet::currentError()
{
    return m_CurrentError;
}


//*************************************************************************************************************

QVector<double> &IDeepCNTKNet::overallError()
{
    return m_OverallError;
}

} //Namespace

Q_DECLARE_INTERFACE(DEEPCNTKEXTENSION::IDeepCNTKNet, "deepcntkextension/1.0")

#endif //IDEEPCNTKNET_H
