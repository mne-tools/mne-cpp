//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_hpis.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Real-time continuous Head-Position-Indicator (cHPI) coil-localisation worker.
 *
 * RtHpi runs the inverse-library HPI fitter (@ref INVLIB::InvHpiFit) on each
 * incoming data window from a background @c QThread. For every window the
 * worker extracts the narrow-band cHPI carriers at the configured coil
 * frequencies, fits a single-dipole model to each carrier amplitude /
 * phase pattern and emits an @ref INVLIB::HpiFitResult containing the
 * estimated coil positions, the head-to-device transform and per-coil
 * goodness-of-fit values.
 *
 * The continuously updated head-to-device transform is the input to
 * Maxwell movement compensation (@ref MaxwellMovementComp), to the live
 * coordinate-frame display, and to dynamic forward-solution updates. The
 * threaded design keeps the GUI responsive even when the HPI fit is run
 * at the full block rate of a multi-channel acquisition.
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
