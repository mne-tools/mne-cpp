//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     neuronalconnectivity.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Contains the declaration of the NeuronalConnectivity class.
 */

#ifndef NEURONALCONNECTIVITY_H
#define NEURONALCONNECTIVITY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuronalconnectivity_global.h"
#include <scShared/Plugins/abstractalgorithm.h>
#include <mne/base/circular_buffer.h>
#include <mne/connectivity/connectivitysettings.h>
#include <mne/connectivity/network/network.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMutex>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

namespace mne::viz::qt
{
class ConnectivitySettingsView;
}

namespace mne::scan
{
class RealTimeSourceEstimate;
class RealTimeMultiSampleArray;
class RealTimeConnectivityEstimate;
class RealTimeEvokedSet;
}

//=============================================================================================================
// DEFINE NAMESPACE NEURONALCONNECTIVITYPLUGIN
//=============================================================================================================

namespace NEURONALCONNECTIVITYPLUGIN
{

//=============================================================================================================
// NEURONALCONNECTIVITYPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class NeuronalConnectivityYourWidget;

//=============================================================================================================
/**
 * DECLARE CLASS NeuronalConnectivity
 *
 * @brief The NeuronalConnectivity class provides a NeuronalConnectivity plugin for online processing.
 */
class NEURONALCONNECTIVITYSHARED_EXPORT NeuronalConnectivity : public mne::scan::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "neuronalconnectivity.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractAlgorithm)

public:
    //=========================================================================================================
    /**
     * Constructs a NeuronalConnectivity.
     */
    NeuronalConnectivity();

    //=========================================================================================================
    /**
     * Destroys the NeuronalConnectivity.
     */
    ~NeuronalConnectivity();

    //=========================================================================================================
    /**
     * AbstractAlgorithm functions
     */
    virtual QSharedPointer<mne::scan::AbstractPlugin> clone() const;
    virtual void init();

    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    void initPluginControlWidgets();

    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual mne::scan::AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    virtual QString getBuildInfo();

    //=========================================================================================================
    /**
     * Udates the pugin with new (incoming) data.
     *
     * @param[in] pMeasurement    The incoming data in form of a generalized Measurement.
     */
    void updateSource(mne::scan::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Updates the real time multi sample array data
     *
     * @param[in] pMeasurement    The incoming data in form of a generalized Measurement.
     */
    void updateRTMSA(mne::scan::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Slot to update the fiff evoked
     *
     * @param[in] pMeasurement   The evoked to be appended.
     */
    void updateRTEV(mne::scan::Measurement::SPtr pMeasurement);

signals:
    void responsibleTriggerTypesChaged(const QStringList& lResponsibleTriggerTypes);

protected:
    //=========================================================================================================
    /**
     * Generate the node positions based on the current incoming data. Also take into account selected bad channels.
     */
    void generateNodeVertices();

    //=========================================================================================================
    /**
     * Hands a trial of the picked channels to the processing thread.
     *
     * @param[in] trial   Channels by samples.
     */
    void pushTrial(Eigen::MatrixXd trial);

    //=========================================================================================================
    /**
     * AbstractAlgorithm function
     */
    virtual void run();


    //=========================================================================================================
    /**
     * Slot called when the metric changed.
     *
     * @param[in] sMetric        The new metric.
     */
    void onMetricChanged(const QString& sMetric);

    //=========================================================================================================
    /**
     * Slot called when the number of trials changed.
     *
     * @param[in] iNumberTrials        The new number of trials.
     */
    void onNumberTrialsChanged(int iNumberTrials);

    //=========================================================================================================
    /**
     * Slot called when the window type changed.
     *
     * @param[in] windowType        The new window type.
     */
    void onWindowTypeChanged(const QString& windowType);

    //=========================================================================================================
    /**
     * Slot called when the trigger type changed.
     *
     * @param[in] triggerType        The new trigger type.
     */
    void onTriggerTypeChanged(const QString& triggerType);

    //=========================================================================================================
    /**
     * Slot called when the frequency band changed.
     *
     * @param[in] fFreqLow        The new lower frequency band.
     * @param[in] fFreqHigh       The new higher frequency band.
     */
    void onFrequencyBandChanged(float fFreqLow, float fFreqHigh);

private:
    qint32 m_iDownSample;        /**< Sampling rate. */
    qint32 m_iNumberAverages;    /**< The number of averages used to calculate the connectivity estimate. Use this only for resting state data when the averaging plugin is not connected.*/
    qint32 m_iNumberBadChannels; /**< The current number of bad channels. USed to test if new bad channels were selected. */
    float m_fFreqBandLow;        /**< The lower frequency band to average the connectivity weights from. */
    float m_fFreqBandHigh;       /**< The higher frequency band to average the connectivity weights to. */

    QString m_sAvrType;                              /**< The average type. */
    std::vector<std::string> m_sConnectivityMethods; /**< The connectivity metric to use. */

    QMutex m_mutex; /**< The mutex to guarantee thread safety. */


    std::shared_ptr<mne::CircularBuffer<Eigen::MatrixXd>> m_pTrialBuffer; /**< Trials waiting for the processing thread. */
    bool m_bSettingsChanged = false;                                      /**< Whether the processing thread starts over with new settings. */
    bool m_bRepublish = false;                                            /**< Whether the processing thread sends the networks again. */
    mne::ConnectivitySettings m_connectivitySettings;                     /**< The connectivity settings.*/

    std::shared_ptr<mne::FiffInfo> m_pFiffInfo; /**< Fiff measurement info.*/

    mne::scan::PluginInputData<mne::scan::RealTimeSourceEstimate>::SPtr m_pRTSEInput;    /**< The RealTimeSourceEstimate input.*/
    mne::scan::PluginInputData<mne::scan::RealTimeMultiSampleArray>::SPtr m_pRTMSAInput; /**< The RealTimeMultiSampleArray input.*/
    mne::scan::PluginInputData<mne::scan::RealTimeEvokedSet>::SPtr m_pRTEVSInput;        /**< The RealTimeEvoked input.*/

    mne::scan::PluginOutputData<mne::scan::RealTimeConnectivityEstimate>::SPtr m_pRTCEOutput; /**< The RealTimeSourceEstimate output.*/

    Eigen::RowVectorXi m_vecPicks; /**< The picked data channels. */
};
} // NAMESPACE

#endif // NEURONALCONNECTIVITY_H
