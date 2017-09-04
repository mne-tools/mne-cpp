//=============================================================================================================
/**
* @file     rtinvop.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief     RtInvOp class declaration.
*
*/

#ifndef RTINVOP_H
#define RTINVOP_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtime_global.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <mne/mne_forwardsolution.h>
#include <mne/mne_inverse_operator.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE REALTIMELIB
//=============================================================================================================

namespace REALTIMELIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;


//=============================================================================================================
/**
* Real-time inverse dSPM, sLoreta inverse operator estimation
*
* @brief Real-time inverse operator estimation
*/
class REALTIMESHARED_EXPORT RtInvOp : public QThread
{
    Q_OBJECT
public:
    typedef QSharedPointer<RtInvOp> SPtr;             /**< Shared pointer type for RtInvOp. */
    typedef QSharedPointer<const RtInvOp> ConstSPtr;  /**< Const shared pointer type for RtInvOp. */

    //=========================================================================================================
    /**
    * Creates the real-time inverse operator estimation object
    *
    * @param[in] p_pFiffInfo    Fiff measurement info
    * @param[in] p_pFwd         Forward solution
    * @param[in] parent         Parent QObject (optional)
    */
    explicit RtInvOp(FiffInfo::SPtr &p_pFiffInfo, MNEForwardSolution::SPtr &p_pFwd, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the inverse operator estimation object.
    */
    ~RtInvOp();

    //=========================================================================================================
    /**
    * Slot to receive incoming noise covariance estimations.
    *
    * @param[in] p_NoiseCov     Noise covariance estimation
    */
    void appendNoiseCov(FiffCov &p_NoiseCov);

    //=========================================================================================================
    /**
    * Stops the RtInv by stopping the producer's thread.
    *
    * @return true if succeeded, false otherwise
    */
    virtual bool stop();

    //=========================================================================================================
    /**
    * Returns true if is running, otherwise false.
    *
    * @return true if is running, false otherwise
    */
    inline bool isRunning();

signals:
    //=========================================================================================================
    /**
    * Signal which is emitted when a inverse operator is calculated.
    *
    * @param[out] p_InvOp  The inverse operator
    */
    void invOperatorCalculated(MNELIB::MNEInverseOperator::SPtr p_pInvOp);

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    QMutex      mutex;                  /**< Provides access serialization between threads. */
    bool        m_bIsRunning;           /**< Whether RtInv is running. */

    QVector<FiffCov> m_vecNoiseCov;     /**< Noise covariance matrices. */

    FiffInfo::SPtr m_pFiffInfo;         /**< The fiff measurement information. */
    MNEForwardSolution::SPtr m_pFwd;    /**< The forward solution. */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool RtInvOp::isRunning()
{
    return m_bIsRunning;
}

} // NAMESPACE

#ifndef metatype_mneinverseoperatorsptr
#define metatype_mneinverseoperatorsptr
Q_DECLARE_METATYPE(MNELIB::MNEInverseOperator::SPtr); /**< Provides QT META type declaration of the MNELIB::MNEInverseOperator type. For signal/slot usage.*/
#endif

#endif // RTINV_H
