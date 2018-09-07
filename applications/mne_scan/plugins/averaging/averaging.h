//=============================================================================================================
/**
* @file     averaging.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
#include <realtime/rtProcessing/rtave.h>


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_evoked_set.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//#define DEBUG_AVERAGING


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
    class RealTimeEvokedSet;
}

namespace REALTIMELIB{
    class RtAve;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE AveragingPlugin
//=============================================================================================================

namespace AveragingPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class AveragingSettingsWidget;


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
    void changeNumAverages(qint32 numAve);

    //=========================================================================================================
    /**
    * Change the average mode
    *
    * @param[in] mode     average mode (0-running or 1-cumulative)
    */
    void changeAverageMode(qint32 mode);

    //=========================================================================================================
    /**
    * Change the stim channel
    *
    * @param[in] index     the new stim channel index
    */
    void changeStimChannel(qint32 index);

    //=========================================================================================================
    /**
    * Change the pre stim stim
    *
    * @param[in] mseconds     the new pres stim in seconds
    */
    void changePreStim(qint32 mseconds);

    //=========================================================================================================
    /**
    * Change the post stim stim
    *
    * @param[in] mseconds     the new post stim in seconds
    */
    void changePostStim(qint32 mseconds);

    //=========================================================================================================
    /**
    * Change the threshold value for trial rejection
    *
    * @param[in] thresholdFirst     the new first component of the the rejection threshold value
    * @param[in] thresholdSecond    the new second component (e-...) of the the rejection threshold value
    */
    void changeArtifactThreshold(double thresholdFirst, int thresholdSecond);

    //=========================================================================================================
    /**
    * Change the state of the artifact rejection based on thresholding
    *
    * @param[in] state     the new state
    */
    void changeArtifactThresholdReductionActive(bool state);

    //=========================================================================================================
    /**
    * Change the variance value for trial rejection
    *
    * @param[in] dVariance     the new value (dVariance times calculated variance is to be rejected)
    */
    void changeArtifactVariance(double dVariance);

    //=========================================================================================================
    /**
    * Change the state of the artifact rejection based on variance
    *
    * @param[in] state     the new state
    */
    void changeArtifactVarianceReductionActive(bool state);

    //=========================================================================================================
    /**
    * Change the baseline from value
    *
    * @param[in] fromMSeconds     the new baseline from value in seconds
    */
    void changeBaselineFrom(qint32 fromMSeconds);

    //=========================================================================================================
    /**
    * Change the baseline to value
    *
    * @param[in] fromMSeconds     the new baseline to value in seconds
    */
    void changeBaselineTo(qint32 toMSeconds);

    //=========================================================================================================
    /**
    * Change the baseline active state
    *
    * @param[in] state     the new state
    */
    void changeBaselineActive(bool state);

    //=========================================================================================================
    /**
    * Append new FiffEvokedSet to the buffer
    *
    * @param[in] p_pEvokedSet               the new FiffEvokedSet
    * @param[in] lResponsibleTriggerTypes   List of all trigger types which lead to the recent emit of a new evoked set.
    */
    void appendEvoked(FIFFLIB::FiffEvokedSet::SPtr p_pEvokedSet,
                      const QStringList &lResponsibleTriggerTypes);

    //=========================================================================================================
    /**
    * Show the averaging widget
    */
    void showAveragingWidget();

    //=========================================================================================================
    /**
    * Reset the averaging plugin and delete all currently stored data
    *
    * @param[in] state     the new state
    */
    void resetAverage(bool state);

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialises the output connector.
    */
    void initConnector();

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr     m_pAveragingInput;      /**< The RealTimeSampleArray of the Averaging input.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeEvokedSet>::SPtr           m_pAveragingOutput;     /**< The RealTimeEvoked of the Averaging output.*/

    IOBUFFER::CircularMatrixBuffer<double>::SPtr    m_pAveragingBuffer;                 /**< Holds incoming data.*/

    QSharedPointer<AveragingSettingsWidget>         m_pAveragingWidget;                 /**< Holds averaging settings widget.*/

    QVector<FIFFLIB::FiffEvokedSet::SPtr>           m_qVecEvokedData;                   /**< Evoked data set. */

    QMutex                                          m_qMutex;                           /**< Provides access serialization between threads. */

    FIFFLIB::FiffInfo::SPtr                         m_pFiffInfo;                        /**< Fiff measurement info.*/
    QList<qint32>                                   m_qListStimChs;                     /**< Stimulus channels.*/

    REALTIMELIB::RtAve::SPtr                    m_pRtAve;                               /**< Real-time average. */

    bool                                            m_bIsRunning;                       /**< If this thread is running. */
    bool                                            m_bProcessData;                     /**< If data should be received for processing. */
    bool                                            m_bDoArtifactThresholdReduction;    /**< If trial rejection is to be done based on threshold. */
    bool                                            m_bDoArtifactVarianceReduction;     /**< If trial rejection is to be done based on variance. */
    bool                                            m_bDoBaselineCorrection;            /**< If baseline correction is to be performed. */

    qint32                                          m_iPreStimSamples;                  /**< The number of pre stimulus samples. */
    qint32                                          m_iPostStimSamples;                 /**< The number of post stimulus samples. */
    qint32                                          m_iPreStimSeconds;                  /**< The number of pre stimulus samples in seconds. */
    qint32                                          m_iPostStimSeconds;                 /**< The number of post stimulus samples in seconds. */
    qint32                                          m_iBaselineFromSeconds;             /**< The start value for baseline correction in seconds. */
    qint32                                          m_iBaselineFromSamples;             /**< The start value for baseline correction in samples. */
    qint32                                          m_iBaselineToSeconds;               /**< The end value for baseline correction in seconds. */
    qint32                                          m_iBaselineToSamples;               /**< The end value for baseline correction in samples. */
    qint32                                          m_iStimChanIdx;                     /**< The channel index in the stim channel list. */
    qint32                                          m_iAverageMode;                     /**< The average mode (0-running or 1-cumulative). */
    qint32                                          m_iNumAverages;                     /**< The number of averages. */
    qint32                                          m_iStimChan;                        /**< The channel index in the total channel list. */
    qint32                                          m_iArtifactThresholdSecond;         /**< The second (e-..) component of the rejection threshold value. */
    double                                          m_dArtifactVariance;                /**< The rejection variance value. */
    double                                          m_dArtifactThresholdFirst;          /**< The first component of the rejection threshold value. */

    QAction*                                        m_pActionShowAdjustment;            /**< The action triggering the averaging settings window. */

    QStringList                                     m_lResponsibleTriggerTypes;         /**< List of all trigger types which lead to the recent emit of a new evoked set. */

signals:
    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();
};

} // NAMESPACE

#endif // AVERAGING_H
