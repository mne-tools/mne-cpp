//=============================================================================================================
/**
 * @file     rtcov.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
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
 * @brief     RtCov class declaration.
 *
 */

#ifndef RTCOV_H
#define RTCOV_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

#include <fiff/fiff_info.h>


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QThread>


//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffCov;
}


//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{


//=============================================================================================================
// RTPROCESSINGLIB FORWARD DECLARATIONS
//=============================================================================================================

struct RtCovComputeResult {
    Eigen::VectorXd mu;
    Eigen::MatrixXd matData;
};

struct RtCovInput {
    QList<Eigen::MatrixXd>      lData;
    FIFFLIB::FiffInfo           fiffInfo;
    int                         iSamples;
};


//=============================================================================================================
/**
 * Real-time covariance worker.
 *
 * @brief Real-time covariance worker.
 */
class RTPROCESINGSHARED_EXPORT RtCovWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Perform actual covariance estimation.
     *
     * @param[in] inputData  Data to estimate the covariance from.
     */
    void doWork(const RtCovInput &inputData);

protected:
    //=========================================================================================================
    /**
     * Computer multiplication with transposed.
     *
     * @param[in] matData  Data to self multiply with.
     *
     * @return   The multiplication result.
     */
    static RtCovComputeResult compute(const Eigen::MatrixXd &matData);

    //=========================================================================================================
    /**
     * Computer multiplication with transposed.
     *
     * @param[out]   finalResult     The final covariance estimation.
     * @param[in]    tempResult      The intermediate result from the compute function
     */
    static void reduce(RtCovComputeResult& finalResult, const RtCovComputeResult &tempResult);

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenver a new covariance was estimated.
     *
     * @param[in] computedCov  The final covariance estimation.
     */
    void resultReady(const FIFFLIB::FiffCov& computedCov);
};


//=============================================================================================================
/**
 * Real-time covariance estimation
 *
 * @brief Real-time covariance estimation
 */
class RTPROCESINGSHARED_EXPORT RtCov : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtCov> SPtr;             /**< Shared pointer type for RtCov. */
    typedef QSharedPointer<const RtCov> ConstSPtr;  /**< Const shared pointer type for RtCov. */

    //=========================================================================================================
    /**
     * Creates the real-time covariance estimation object.
     *
     * @param[in] iMaxSamples      Number of samples to use for each data chunk
     * @param[in] pFiffInfo        Associated Fiff Information
     * @param[in] parent     Parent QObject (optional)
     */
    explicit RtCov(qint32 iMaxSamples,
                   QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                   QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the Real-time covariance estimation object.
     */
    ~RtCov();

    //=========================================================================================================
    /**
     * Slot to receive incoming data.
     *
     * @param[in] matDataSegment  Data to estimate the covariance from -> ToDo Replace this by shared data pointer
     */
    void append(const Eigen::MatrixXd &matDataSegment);

    //=========================================================================================================
    /**
     * Set number of estimation samples
     *
     * @param[in] samples    estimation samples to set
     */
    void setSamples(qint32 samples);

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
    void handleResults(const FIFFLIB::FiffCov& computedCov);

    QThread                 m_workerThread;             /**< The worker thread. */

    qint32                  m_iMaxSamples;              /**< Maximal amount of samples received, before covariance is estimated.*/
    qint32                  m_iNewMaxSamples;           /**< New maximal amount of samples received, before covariance is estimated.*/
    int                     m_iSamples;                 /**< The number of stored samples. */

    QList<Eigen::MatrixXd>  m_lData;                    /**< The stored data blocks. */

    QSharedPointer<FIFFLIB::FiffInfo>  m_pFiffInfo;     /**< Holds the fiff measurement information. */

signals:
    //=========================================================================================================
    /**
     * Signal which is emitted when a new covariance Matrix is estimated.
     *
     * @param[int] computedCov  The computed covariance information
     */
    void covCalculated(const FIFFLIB::FiffCov& computedCov);

    //=========================================================================================================
    /**
     * Emit this signal whenver the worker should process a new batch of stored data blocks.
     *
     * @param[in] inputData  The new batch of stored data.
     */
    void operate(const RtCovInput &inputData);

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // RTCOV_H
