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
#include <generics/circularmatrixbuffer.h>
#include <rtProcessing/rtave.h>


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
    class NewRealTimeMultiSampleArray;
    class RealTimeEvoked;
}

namespace RTPROCESSINGLIB{
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
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "averaging.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
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
    void update(SCMEASLIB::NewMeasurement::SPtr pMeasurement);

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

    void changeStimChannel(qint32 index);

    void changePreStim(qint32 mseconds);

    void changePostStim(qint32 mseconds);

    void changeArtifactThreshold(double thresholdFirst, int thresholdSecond);

    void changeArtifactReductionActive(bool state);

    void changeBaselineFrom(qint32 fromMSeconds);

    void changeBaselineTo(qint32 toMSeconds);

    void changeBaselineActive(bool state);

    void appendEvoked(FIFFLIB::FiffEvokedSet::SPtr p_pEvokedSet);

    void showAveragingWidget();

    void resetAverage(bool state);

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialises the output connector.
    */
    void initConnector();

    SCSHAREDLIB::PluginInputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr  m_pAveragingInput;      /**< The RealTimeSampleArray of the Averaging input.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeEvoked>::SPtr              m_pAveragingOutput;     /**< The RealTimeEvoked of the Averaging output.*/

    IOBUFFER::CircularMatrixBuffer<double>::SPtr    m_pAveragingBuffer;             /**< Holds incoming data.*/

    QSharedPointer<AveragingSettingsWidget>         m_pAveragingWidget;

    QVector<FIFFLIB::FiffEvokedSet::SPtr>           m_qVecEvokedData;               /**< Evoked data set */

    QMutex                                          m_qMutex;                       /**< Provides access serialization between threads*/

    FIFFLIB::FiffInfo::SPtr                         m_pFiffInfo;                    /**< Fiff measurement info.*/
    QList<qint32>                                   m_qListStimChs;                 /**< Stimulus channels.*/

    RTPROCESSINGLIB::RtAve::SPtr                    m_pRtAve;                       /**< Real-time average. */

    bool                                            m_bIsRunning;                   /**< If source lab is running */
    bool                                            m_bProcessData;                 /**< If data should be received for processing */
    bool                                            m_bDoArtifactReduction;
    bool                                            m_bDoBaselineCorrection;

    qint32                                          m_iPreStimSamples;
    qint32                                          m_iPostStimSamples;
    qint32                                          m_iPreStimSeconds;
    qint32                                          m_iPostStimSeconds;
    qint32                                          m_iBaselineFromSeconds;
    qint32                                          m_iBaselineFromSamples;
    qint32                                          m_iBaselineToSeconds;
    qint32                                          m_iBaselineToSamples;
    qint32                                          m_iStimChanIdx;
    qint32                                          m_iAverageMode;
    qint32                                          m_iNumAverages;
    qint32                                          m_iStimChan;
    qint32                                          m_iArtifactThresholdSecond;
    double                                          m_dArtifactThresholdFirst;

    QAction*                                        m_pActionShowAdjustment;

#ifdef DEBUG_AVERAGING
    //
    // TEST
    //
    qint32 m_iTestStimCh;
    qint32 m_iTestCount;
    qint32 m_iTestCount2;
#endif

signals:
    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();
};

} // NAMESPACE

#endif // AVERAGING_H
