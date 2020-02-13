//=============================================================================================================
/**
 * @file     averaging.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the Averaging class.
 *
 */

#ifndef AVERAGING_H
#define AVERAGING_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging_global.h"

#include <scShared/Interfaces/IAlgorithm.h>
#include <utils/generics/circularmatrixbuffer.h>

#include <fiff/fiff_evoked_set.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
    class RealTimeEvokedSet;
}

namespace RTPROCESSINGLIB{
    class RtAve;
}

namespace DISPLIB{
    class AveragingSettingsView;
    class ArtifactSettingsView;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE AVERAGINGPLUGIN
//=============================================================================================================

namespace AVERAGINGPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
 * DECLARE CLASS Averaging
 *
 * @brief The Averaging class provides a Averaging algorithm structure.
 */
class AVERAGINGSHARED_EXPORT Averaging : public SCSHAREDLIB::IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "averaging.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

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
     * Reimplemented virtual functions
     */
    virtual void unload();
    virtual QSharedPointer<SCSHAREDLIB::IPlugin> clone() const;
    virtual bool start();
    virtual bool stop();
    virtual SCSHAREDLIB::IPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Initialise input and output connectors.
     */
    virtual void init();

    //=========================================================================================================
    /**
     * Change the number of averages
     *
     * @param[in] numAve     new number of averages
     */
    void onChangeNumAverages(qint32 numAve);

    //=========================================================================================================
    /**
     * Change the stim channel
     *
     * @param[in] sStimCh     the new stim channel name
     */
    void onChangeStimChannel(const QString &sStimCh);

    //=========================================================================================================
    /**
     * Change the pre stim stim
     *
     * @param[in] mseconds     the new pres stim in seconds
     */
    void onChangePreStim(qint32 mseconds);

    //=========================================================================================================
    /**
     * Change the post stim stim
     *
     * @param[in] mseconds     the new post stim in seconds
     */
    void onChangePostStim(qint32 mseconds);

    //=========================================================================================================
    /**
     * Change the threshold value for trial rejection
     *
     * @param[in] mapThresholds       The new map including the current thresholds for the channels
     */
    void onChangeArtifactThreshold(const QMap<QString, double> &mapThresholds);

    //=========================================================================================================
    /**
     * Change the state of the artifact rejection based on variance
     *
     * @param[in] state     the new state
     */
    void onChangeArtifactVarianceReductionActive(bool state);

    //=========================================================================================================
    /**
     * Change the baseline from value
     *
     * @param[in] fromMSeconds     the new baseline from value in seconds
     */
    void onChangeBaselineFrom(qint32 fromMSeconds);

    //=========================================================================================================
    /**
     * Change the baseline to value
     *
     * @param[in] fromMSeconds     the new baseline to value in seconds
     */
    void onChangeBaselineTo(qint32 toMSeconds);

    //=========================================================================================================
    /**
     * Change the baseline active state
     *
     * @param[in] state     the new state
     */
    void onChangeBaselineActive(bool state);

    //=========================================================================================================
    /**
     * Append new FiffEvokedSet to the buffer
     *
     * @param[in] evokedSet                  The new FiffEvokedSet
     * @param[in] lResponsibleTriggerTypes   List of all trigger types which lead to the recent emit of a new evoked set.
     */
    void appendEvoked(const FIFFLIB::FiffEvokedSet& evokedSet,
                      const QStringList &lResponsibleTriggerTypes);

    //=========================================================================================================
    /**
     * Reset the averaging plugin and delete all currently stored data
     *
     * @param[in] state     the new state
     */
    void onResetAverage(bool state);

protected:
    virtual void run();

private:
    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr     m_pAveragingInput;      /**< The RealTimeSampleArray of the Averaging input.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeEvokedSet>::SPtr           m_pAveragingOutput;     /**< The RealTimeEvoked of the Averaging output.*/

    IOBUFFER::CircularMatrixBuffer<double>::SPtr    m_pAveragingBuffer;

    QSharedPointer<DISPLIB::AveragingSettingsView>  m_pAveragingSettingsView;           /**< Holds averaging settings widget.*/
    QSharedPointer<DISPLIB::ArtifactSettingsView>   m_pArtifactSettingsView;            /**< Holds artifact settings widget.*/

    QVector<FIFFLIB::FiffEvokedSet>                 m_qVecEvokedData;                   /**< Evoked data set. */

    QMutex                                          m_qMutex;                           /**< Provides access serialization between threads. */

    FIFFLIB::FiffInfo::SPtr                         m_pFiffInfo;                        /**< Fiff measurement info.*/

    QSharedPointer<RTPROCESSINGLIB::RtAve>          m_pRtAve;                           /**< Real-time average object. */

    bool                                            m_bIsRunning;                       /**< If this thread is running. */
    bool                                            m_bProcessData;                     /**< If data should be received for processing. */

    QStringList                                     m_lResponsibleTriggerTypes;         /**< List of all trigger types which lead to the recent emit of a new evoked set. */

    QMap<QString,int>                               m_mapStimChsIndexNames;             /**< The currently available stim channels and their corresponding index in the data. */

signals:
};

} // NAMESPACE

#endif // AVERAGING_H
