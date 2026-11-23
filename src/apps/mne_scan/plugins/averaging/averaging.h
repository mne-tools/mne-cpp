//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     averaging.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the Averaging class.
 */

#ifndef AVERAGING_H
#define AVERAGING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging_global.h"
#include <scShared/Plugins/abstractalgorithm.h>
#include <mne/base/circular_buffer.h>
#include <mne/dsp/rt/rt_averaging.h>
#include <mne/fiff/fiff_evoked_set.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::scan
{
class RealTimeMultiSampleArray;
class RealTimeEvokedSet;
}

//=============================================================================================================
// DEFINE NAMESPACE AVERAGINGPLUGIN
//=============================================================================================================

namespace AVERAGINGPLUGIN
{

//=============================================================================================================
// AVERAGINGPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS Averaging
 *
 * @brief The Averaging class provides a Averaging algorithm structure.
 */
class AVERAGINGSHARED_EXPORT Averaging : public mne::scan::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "averaging.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractAlgorithm)

    friend class AveragingSettingsWidget;

public:
    //=========================================================================================================
    /**
     * Constructs a Averaging.
     */
    Averaging();

    //=========================================================================================================
    /**
     * Destroys the Averaging.
     */
    ~Averaging();

    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    void initPluginControlWidgets();

    //=========================================================================================================
    /**
     * Reimplemented virtual functions
     */
    virtual void unload();
    virtual QSharedPointer<mne::scan::AbstractPlugin> clone() const;
    virtual bool start();
    virtual bool stop();
    virtual mne::scan::AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    void update(mne::scan::Measurement::SPtr pMeasurement);
    virtual QString getBuildInfo();

    //=========================================================================================================
    /**
     * Initialise input and output connectors.
     */
    virtual void init();

    //=========================================================================================================
    /**
     * Change the number of averages
     *
     * @param[in] numAve     new number of averages.
     */
    void onChangeNumAverages(qint32 numAve);

    //=========================================================================================================
    /**
     * Change the stim channel
     *
     * @param[in] sStimCh     the new stim channel name.
     */
    void onChangeStimChannel(const std::string& sStimCh);

    //=========================================================================================================
    /**
     * Change the pre stim stim
     *
     * @param[in] mseconds     the new pres stim in seconds.
     */
    void onChangePreStim(qint32 mseconds);

    //=========================================================================================================
    /**
     * Change the post stim stim
     *
     * @param[in] mseconds     the new post stim in seconds.
     */
    void onChangePostStim(qint32 mseconds);

    //=========================================================================================================
    /**
     * Change the threshold value for trial rejection
     *
     * @param[in] mapThresholds       The new map including the current thresholds for the channels.
     */
    void onChangeArtifactThreshold(const std::map<std::string, double>& mapThresholds);

    //=========================================================================================================
    /**
     * Change the baseline from value
     *
     * @param[in] fromMS     the new baseline from value in seconds.
     */
    void onChangeBaselineFrom(qint32 fromMS);

    //=========================================================================================================
    /**
     * Change the baseline to value
     *
     * @param[in] fromMS     the new baseline to value in seconds.
     */
    void onChangeBaselineTo(qint32 toMSeconds);

    //=========================================================================================================
    /**
     * Change the baseline active state
     *
     * @param[in] state     the new state.
     */
    void onChangeBaselineActive(bool state);

    //=========================================================================================================
    /**
     * Reset the averaging plugin and delete all currently stored data
     *
     * @param[in] state     the new state.
     */
    void onResetAverage(bool state);

private:
    virtual void run();

    mne::scan::PluginInputData<mne::scan::RealTimeMultiSampleArray>::SPtr m_pAveragingInput; /**< The RealTimeSampleArray of the Averaging input.*/
    mne::scan::PluginOutputData<mne::scan::RealTimeEvokedSet>::SPtr m_pAveragingOutput;      /**< The RealTimeEvoked of the Averaging output.*/

    void applyBaseline();

    std::shared_ptr<mne::CircularBuffer<Eigen::MatrixXd>> m_pBlockBuffer; /**< Incoming data blocks. */

    QMutex m_qMutex; /**< Guards the fiff info and the averaging settings. */

    std::shared_ptr<mne::FiffInfo> m_pFiffInfo; /**< Fiff measurement info.*/

    mne::RtAveraging::Settings m_settings; /**< Averaging settings, taken over by the processing thread. */
    bool m_bSettingsChanged = false;       /**< Whether m_settings changed since the processing thread took them. */
    bool m_bResetRequested = false;        /**< Whether the processing thread drops its epochs. */
    bool m_bBaselineActive = false;        /**< Whether the baseline interval is subtracted. */
    std::pair<float, float> m_baseline;    /**< Baseline interval in seconds. */

    std::map<std::string, int> m_mapStimChsIndexNames; /**< The currently available stim channels and their corresponding index in the data. */

signals:
    void stimChannelsChanged(const std::map<std::string, int>& mapStimChsIndexNames);
    void fiffChInfoChanged(const std::vector<mne::FiffChInfo>& fiffChInfoList);
    void evokedSetChanged(const mne::FiffEvokedSet& evokedSet);
};
} // NAMESPACE

#endif // AVERAGING_H
