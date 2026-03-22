//=============================================================================================================
/**
 * @file     rthpis.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief     RtHpi class declaration.
 *
 */

#ifndef RT_HPIS_RTPROCESSING_H
#define RT_HPIS_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"
#include <inv/hpi/inv_sensor_set.h>
#include <inv/hpi/inv_signal_model.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QSharedPointer>
#include <QVector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

namespace INVLIB {
    class InvSensorSet;
    class InvHpiFit;
    struct HpiFitResult;
    class InvHpiModelParameters;
}

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
/**
 * Real-time HPI worker.
 *
 * @brief Background worker thread that runs continuous HPI coil localization.
 */
class DSPSHARED_EXPORT RtHpiWorker : public QObject
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Creates the real-time HPI worker object.
     *
     * @param[in] pFiffInfo        Associated Fiff Information.
     */
    explicit RtHpiWorker(const INVLIB::InvSensorSet sensorSet);

    //=========================================================================================================
    /**
     * Perform one single HPI fit.
     *
     * @param[in] matData            Data to estimate the HPI positions from.
     * @param[in] matProjectors      The projectors to apply. Bad channels are still included.
     * @param[in] vFreqs             The frequencies for each coil.
     * @param[in] pFiffInfo          Associated Fiff Information.
     */
    void doWork(const Eigen::MatrixXd& matData,
                const Eigen::MatrixXd& matProjectors,
                const INVLIB::InvHpiModelParameters& hpiModelParameters,
                const Eigen::MatrixXd& matCoilsHead);

protected:
    //=========================================================================================================
    QSharedPointer<INVLIB::InvHpiFit>              m_pHpiFit;             /**< Holds the HpiFit object. */

signals:
    void resultReady(const INVLIB::HpiFitResult &fitResult);
};

//=============================================================================================================
/**
 * Real-time Head Coil Positions estimation.
 *
 * @brief Controller that manages RtHpiWorker for continuous head position tracking.
 */
class DSPSHARED_EXPORT RtHpi : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtHpi> SPtr;             /**< Shared pointer type for RtHpi. */
    typedef QSharedPointer<const RtHpi> ConstSPtr;  /**< Const shared pointer type for RtHpi. */

    //=========================================================================================================
    /**
     * Creates the real-time HPIS estimation object.
     *
     * @param[in] p_pFiffInfo        Associated Fiff Information.
     * @param[in] parent     Parent QObject (optional).
     */
    explicit RtHpi(const INVLIB::InvSensorSet sensorSet,
                   QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the Real-time HPI estimation object.
     */
    ~RtHpi();

    //=========================================================================================================
    /**
     * Slot to receive incoming data.
     *
     * @param[in] data  Data to estimate the HPI positions from.
     */
    void append(const Eigen::MatrixXd &data);

    //=========================================================================================================
    /**
     * Set the coil frequencies.
     *
     * @param[in] vCoilFreqs  The coil frequencies.
     */
    void setModelParameters(INVLIB::InvHpiModelParameters hpiModelParameters);

    //=========================================================================================================
    /**
     * Set the new projection matrix.
     *
     * @param[in] matProjectors  The new projection matrix.
     */
    void setProjectionMatrix(const Eigen::MatrixXd& matProjectors);

    //=========================================================================================================
    /**
     * Set the new projection matrix.
     *
     * @param[in] matProjectors  The new projection matrix.
     */
    void setHpiDigitizer(const Eigen::MatrixXd& matCoilsHead);

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
     * Handles the results.
     */
    void handleResults(const INVLIB::HpiFitResult &fitResult);

    QThread m_workerThread;         /**< The worker thread. */
    Eigen::MatrixXd m_matCoilsHead;           /**< Vector contains the HPI coil frequencies. */
    Eigen::MatrixXd m_matProjectors;        /**< Holds the matrix with the SSP and compensator projectors.*/
    INVLIB::InvSensorSet m_sensorSet;
    INVLIB::InvHpiModelParameters m_modelParameters;

signals:
    void newHpiFitResultAvailable(const INVLIB::HpiFitResult &fitResult);
    void operate(const Eigen::MatrixXd& matData,
                 const Eigen::MatrixXd& matProjectors,
                 const INVLIB::InvHpiModelParameters& hpiModelParameters,
                 const Eigen::MatrixXd& matCoilsHead);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // RT_HPIS_RTPROCESSING_H
