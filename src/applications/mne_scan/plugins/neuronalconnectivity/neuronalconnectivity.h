//=============================================================================================================
/**
 * @file     neuronalconnectivity.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the NeuronalConnectivity class.
 *
 */

#ifndef NEURONALCONNECTIVITY_H
#define NEURONALCONNECTIVITY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuronalconnectivity_global.h"

#include <scShared/Plugins/abstractalgorithm.h>

#include <utils/generics/circularbuffer.h>

#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>

#include <thread>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMutex>
#include <QElapsedTimer>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

namespace DISPLIB {
    class ConnectivitySettingsView;
}

namespace RTPROCESSINGLIB {
    class RtConnectivity;
}

namespace SCMEASLIB {
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
class NEURONALCONNECTIVITYSHARED_EXPORT NeuronalConnectivity : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "neuronalconnectivity.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

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
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;
    virtual void init();

    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    void initPluginControlWidgets();

    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual SCSHAREDLIB::AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    virtual QString getBuildInfo();

    //=========================================================================================================
    /**
     * Udates the pugin with new (incoming) data.
     *
     * @param[in] pMeasurement    The incoming data in form of a generalized Measurement.
     */
    void updateSource(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Updates the real time multi sample array data
     *
     * @param[in] pMeasurement    The incoming data in form of a generalized Measurement.
     */
    void updateRTMSA(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Slot to update the fiff evoked
     *
     * @param[in] pMeasurement   The evoked to be appended.
     */
    void updateRTEV(SCMEASLIB::Measurement::SPtr pMeasurement);

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
     * AbstractAlgorithm function
     */
    virtual void run();

    //=========================================================================================================
    /**
     * Slot called when a new real-time connectivity estimate is available.
     *
     * @param[in] connectivityResult        The new connectivity estimate.
     */
    void onNewConnectivityResultAvailable(const QList<CONNECTIVITYLIB::Network>& connectivityResults,
                                          const CONNECTIVITYLIB::ConnectivitySettings& connectivitySettings);

    //=========================================================================================================
    /**
     * Slot called when the metric changed.
     *
     * @param[in] sMetric        The new metric.
     */
    void onMetricChanged(const QString &sMetric);

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
    qint32              m_iDownSample;          /**< Sampling rate. */
    qint32              m_iNumberAverages;      /**< The number of averages used to calculate the connectivity estimate. Use this only for resting state data when the averaging plugin is not connected.*/
    qint32              m_iNumberBadChannels;   /**< The current number of bad channels. USed to test if new bad channels were selected. */
    float               m_fFreqBandLow;         /**< The lower frequency band to average the connectivity weights from. */
    float               m_fFreqBandHigh;        /**< The higher frequency band to average the connectivity weights to. */
    qint32              m_iBlockSize;           /**< The block size of teh last received data block. In frequency bins. */

    QString             m_sAvrType;             /**< The average type. */
    QStringList         m_sConnectivityMethods; /**< The connectivity metric to use. */

    QMutex              m_mutex;                /**< The mutex to guarantee thread safety. */

    QElapsedTimer       m_timer;                /**< The timer to evaluate performance. */

    CONNECTIVITYLIB::ConnectivitySettings                                           m_connectivitySettings;         /**< The connectivity settings.*/

    QSharedPointer<UTILSLIB::CircularBuffer<CONNECTIVITYLIB::Network> >             m_pCircularBuffer;              /**< The circular buffer holding the connectivity estimates.*/
    QSharedPointer<RTPROCESSINGLIB::RtConnectivity>                                 m_pRtConnectivity;              /**< The real-time connectivity estimation object.*/
    QSharedPointer<FIFFLIB::FiffInfo>                                               m_pFiffInfo;                    /**< Fiff measurement info.*/
    QSharedPointer<DISPLIB::ConnectivitySettingsView>                               m_pConnectivitySettingsView;    /**< The connectivity settings widget which will be added to the Quick Control view. The QuickControlView will not take ownership. Ownership will be managed by the QSharedPointer.*/
    QAction*                                                                        m_pActionShowYourWidget;        /**< flag whether thread is running.*/

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeSourceEstimate>::SPtr           m_pRTSEInput;                   /**< The RealTimeSourceEstimate input.*/
    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr         m_pRTMSAInput;                  /**< The RealTimeMultiSampleArray input.*/
    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeEvokedSet>::SPtr                m_pRTEVSInput;                  /**< The RealTimeEvoked input.*/

    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeConnectivityEstimate>::SPtr    m_pRTCEOutput;                  /**< The RealTimeSourceEstimate output.*/

    CONNECTIVITYLIB::Network    m_connectivityEstimate;         /**< The current connectivity estimate.*/
    Eigen::MatrixX3f            m_matNodeVertLeft;              /**< Holds the left hemi vertex postions of the network nodes. Corresponding to the neuronal sources.*/
    Eigen::MatrixX3f            m_matNodeVertRight;             /**< Holds the right hemi vertex postions of the network nodes. Corresponding to the neuronal sources.*/
    Eigen::MatrixX3f            m_matNodeVertComb;              /**< Holds both hemi vertex postions of the network nodes. Corresponding to the neuronal sources.*/ 
    Eigen::RowVectorXi          m_vecPicks;                     /**< The picked data channels. */

    CONNECTIVITYLIB::Network    m_currentConnectivityResult;    /**< The current connectivity result.*/

    std::thread             m_OutputProcessingThread;
    std::atomic_bool        m_bProcessOutput;
};
} // NAMESPACE

#endif // NEURONALCONNECTIVITY_H
