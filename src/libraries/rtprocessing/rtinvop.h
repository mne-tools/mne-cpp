//=============================================================================================================
/**
 * @file     rtinvop.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Christoph Dinh. All rights reserved.
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

#ifndef RTINVOP_RTPROCESSING_H
#define RTINVOP_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

#include <fiff/fiff_cov.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

namespace MNELIB {
    class MNEForwardSolution;
    class MNEInverseOperator;
}

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
// RTPROCESSINGLIB FORWARD DECLARATIONS
//=============================================================================================================

struct RtInvOpInput {
    QSharedPointer<FIFFLIB::FiffInfo>           pFiffInfo;
    QSharedPointer<MNELIB::MNEForwardSolution>  pFwd;
    FIFFLIB::FiffCov                            noiseCov;
};

//=============================================================================================================
/**
 * Real-time inverse operator worker.
 *
 * @brief Real-time inverse operator worker.
 */
class RTPROCESINGSHARED_EXPORT RtInvOpWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Perform actual inverse operator creation.
     *
     * @param[in] inputData  Data to estimate the inverser operator from.
     */
    void doWork(const RtInvOpInput &inputData);

signals:
    //=========================================================================================================
    /**
     * Wmit this signal whenver a new inverser operator was estimated.
     *
     * @param[in] invOp  The final inverser operator estimation.
     */
    void resultReady(const MNELIB::MNEInverseOperator& invOp);
};

//=============================================================================================================
/**
 * Real-time inverse dSPM, sLoreta inverse operator estimation
 *
 * @brief Real-time inverse operator estimation
 */
class RTPROCESINGSHARED_EXPORT RtInvOp : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtInvOp> SPtr;             /**< Shared pointer type for RtInvOp. */
    typedef QSharedPointer<const RtInvOp> ConstSPtr;  /**< Const shared pointer type for RtInvOp. */

    //=========================================================================================================
    /**
     * Creates the real-time inverse operator estimation object
     *
     * @param[in] p_pFiffInfo    Fiff measurement info.
     * @param[in] p_pFwd         Forward solution.
     * @param[in] parent         Parent QObject (optional).
     */
    explicit RtInvOp(QSharedPointer<FIFFLIB::FiffInfo> &p_pFiffInfo,
                     QSharedPointer<MNELIB::MNEForwardSolution> &p_pFwd,
                     QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the inverse operator estimation object.
     */
    ~RtInvOp();

    //=========================================================================================================
    /**
     * Slot to receive incoming noise covariance estimations.
     *
     * @param[in] noiseCov     Noise covariance estimation.
     */
    void append(const FIFFLIB::FiffCov &noiseCov);

    //=========================================================================================================
    /**
     * Slot to receive incoming forward solution.
     *
     * @param[in] pFwd     Forward solution.
     */
    void setFwdSolution(QSharedPointer<MNELIB::MNEForwardSolution> pFwd);

    //=========================================================================================================
    /**
     * Restarts the thread by interrupting its computation queue, quitting, waiting and then starting it again.
     */
    void restart();

    //=========================================================================================================
    /**
     * Stops the thread by interrupting its computation queue, quitting and waiting.
     */
    void stop();

protected:
    //=========================================================================================================
    /**
     * Handles the result
     */
    void handleResults(const MNELIB::MNEInverseOperator& invOp);

    QSharedPointer<FIFFLIB::FiffInfo>           m_pFiffInfo;        /**< The fiff measurement information. */
    QSharedPointer<MNELIB::MNEForwardSolution>  m_pFwd;             /**< The forward solution. */

    QThread                                     m_workerThread;     /**< The worker thread. */

signals:
    //=========================================================================================================
    /**
     * Signal which is emitted when a inverse operator is calculated.
     *
     * @param[out] invOp  The inverse operator.
     */
    void invOperatorCalculated(const MNELIB::MNEInverseOperator& invOp);

    //=========================================================================================================
    /**
     * Emit this signal whenver the worker should create a new inverse operator estimation.
     *
     * @param[in] inputData  The new covariance estimation.
     */
    void operate(const RtInvOpInput &inputData);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // RTINV_RTPROCESSING_H
