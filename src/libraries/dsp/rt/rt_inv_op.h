//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rt_inv_op.h
 * @since March 2026
 * @brief Real-time recomputation of the linear inverse operator from updated noise covariance.
 *
 * RtInvOp recomputes a regularised @ref MNELIB::MNEInverseOperator whenever
 * a new noise-covariance estimate is published by @ref RtCov, so a live
 * source-localisation pipeline can adapt to changes in the sensor noise
 * floor (subject movement, ambient electromagnetic noise, channel removal)
 * without dropping a sample. The forward solution, source-orientation
 * settings and SNR / depth-prior parameters are held constant; only the
 * covariance-dependent factors are reassembled.
 *
 * The actual whitening, SVD and assembly run on a worker @c QThread; the
 * resulting inverse operator is delivered via a Qt signal so downstream
 * applyInverse calls see a consistent operator at all times.
 */

#ifndef RT_INV_OP_RTPROCESSING_H
#define RT_INV_OP_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"

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

/**
 * @brief Input bundle for the real-time inverse operator worker containing noise covariance, forward solution, and settings.
 */
struct RtInvOpInput {
    QSharedPointer<FIFFLIB::FiffInfo>           pFiffInfo;
    QSharedPointer<MNELIB::MNEForwardSolution>  pFwd;
    FIFFLIB::FiffCov                            noiseCov;
};

//=============================================================================================================
/**
 * Real-time inverse operator worker.
 *
 * @brief Background worker thread that recomputes the MNE inverse operator when covariance updates arrive.
 */
class DSPSHARED_EXPORT RtInvOpWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Perform actual inverse operator creation.
     *
     * @param[in] inputData  Data to estimate the inverse operator from.
     */
    void doWork(const RtInvOpInput &inputData);

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever a new inverse operator was estimated.
     *
     * @param[in] invOp  The final inverse operator estimation.
     */
    void resultReady(const MNELIB::MNEInverseOperator& invOp);
};

//=============================================================================================================
/**
 * Real-time inverse dSPM, sLoreta inverse operator estimation
 *
 * @brief Controller that manages RtInvOpWorker for online inverse operator updates.
 */
class DSPSHARED_EXPORT RtInvOp : public QObject
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

#endif // RT_INV_OP_RTPROCESSING_H
