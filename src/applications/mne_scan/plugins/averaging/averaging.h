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
#include <utils/generics/circularbuffer.h>

#include <fiff/fiff_evoked_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
    class RealTimeEvokedSet;
}

namespace RTPROCESSINGLIB{
    class RtAveraging;
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
class AVERAGINGSHARED_EXPORT Averaging : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "averaging.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

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
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;
    virtual bool start();
    virtual bool stop();
    virtual SCSHAREDLIB::AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);
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
    void onChangeStimChannel(const QString &sStimCh);

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
    void onChangeArtifactThreshold(const QMap<QString, double> &mapThresholds);

    //=========================================================================================================
    /**
     * Change the state of the artifact rejection based on variance
     *
     * @param[in] state     the new state.
     */
    void onChangeArtifactVarianceReductionActive(bool state);

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
     * Append new FiffEvokedSet to the buffer
     *
     * @param[in] evokedSet                  The new FiffEvokedSet.
     * @param[in] lResponsibleTriggerTypes   List of all trigger types which lead to the recent emit of a new evoked set.
     */
    void onNewEvokedSet(const FIFFLIB::FiffEvokedSet& evokedSet,
                        const QStringList &lResponsibleTriggerTypes);

    //=========================================================================================================
    /**
     * Reset the averaging plugin and delete all currently stored data
     *
     * @param[in] state     the new state.
     */
    void onResetAverage(bool state);

private:
    virtual void run();

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr     m_pAveragingInput;      /**< The RealTimeSampleArray of the Averaging input.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeEvokedSet>::SPtr           m_pAveragingOutput;     /**< The RealTimeEvoked of the Averaging output.*/

    UTILSLIB::CircularBuffer<FIFFLIB::FiffEvokedSet>::SPtr                      m_pCircularBuffer;      /**< Holds incoming fiff evoked sets. */

    QMutex                                          m_qMutex;                           /**< Provides access serialization between threads. */

    FIFFLIB::FiffInfo::SPtr                         m_pFiffInfo;                        /**< Fiff measurement info.*/

    QSharedPointer<RTPROCESSINGLIB::RtAveraging>          m_pRtAve;                     /**< Real-time average object. */

    QStringList                                     m_lResponsibleTriggerTypes;         /**< List of all trigger types which lead to the recent emit of a new evoked set. */

    QMap<QString,int>                               m_mapStimChsIndexNames;             /**< The currently available stim channels and their corresponding index in the data. */

signals:
    void stimChannelsChanged(const QMap<QString,int>& mapStimChsIndexNames);
    void fiffChInfoChanged(const QList<FIFFLIB::FiffChInfo>& fiffChInfoList);
    void evokedSetChanged(const FIFFLIB::FiffEvokedSet& evokedSet);
};
} // NAMESPACE

#endif // AVERAGING_H
