//=============================================================================================================
/**
 * @file     rtcmusic.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     February, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the RtcMusic class.
 *
 */

#ifndef RTCMUSIC_H
#define RTCMUSIC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtcmusic_global.h"
#include <scShared/Interfaces/IAlgorithm.h>

#include <utils/generics/circularmatrixbuffer.h>

#include <fs/annotationset.h>
#include <fs/surfaceset.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_evoked.h>
#include <mne/mne_forwardsolution.h>
#include <mne/mne_sourceestimate.h>
#include <inverse/rapMusic/pwlrapmusic.h>

#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimeevokedset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QFile>

//=============================================================================================================
// DEFINE NAMESPACE RTCMUSICPLUGIN
//=============================================================================================================

namespace RTCMUSICPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS RtcMusic
 *
 * @brief The RtcMusic class provides a dummy algorithm structure.
 */
class RTCMUSICSHARED_EXPORT RtcMusic : public SCSHAREDLIB::IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "rtcmusic.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

    friend class RtcMusicSetupWidget;

public:

    //=========================================================================================================
    /**
     * Constructs a RtcMusic.
     */
    RtcMusic();
    //=========================================================================================================
    /**
     * Destroys the RtcMusic.
     */
    ~RtcMusic();

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<IPlugin> clone() const;

    //=========================================================================================================
    /**
     * Initialise the RtcMusic.
     */
    virtual void init();

    //=========================================================================================================
    /**
     * Is called when plugin is detached of the stage. Can be used to safe settings.
     */
    virtual void unload();

    void calcFiffInfo();

    void doClustering();

    void finishedClustering();

    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
     * Slot to update the fiff evoked
     *
     * @param[in] pMeasurement   The evoked to be appended
     */
    void updateRTE(SCMEASLIB::Measurement::SPtr pMeasurement);

signals:
    //=========================================================================================================
    /**
     * Signal when clsutering is started
     */
    void clusteringStarted();

    //=========================================================================================================
    /**
     * Signal when clsutering has finished
     */
    void clusteringFinished();

protected:
    virtual void run();

private:
    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeEvokedSet>::SPtr   m_pRTEInput;    /**< The RealTimeEvoked input.*/

    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSourceEstimate>::SPtr      m_pRTSEOutput;  /**< The RealTimeSourceEstimate output.*/

    QMutex m_qMutex;

    QVector<FIFFLIB::FiffEvokedSet> m_qVecFiffEvoked;
    qint32 m_iNumAverages;

    bool m_bIsRunning;      /**< If source lab is running */
    bool m_bReceiveData;    /**< If thread is ready to receive data */
    bool m_bProcessData;    /**< If data should be received for processing */

    QFile                               m_qFileFwdSolution; /**< File to forward solution. */
    MNELIB::MNEForwardSolution::SPtr    m_pFwd;             /**< Forward solution. */
    MNELIB::MNEForwardSolution::SPtr    m_pClusteredFwd;    /**< Clustered forward solution. */

    bool m_bFinishedClustering;                     /**< If clustered forward solution is available. */

    QString                             m_sAtlasDir;        /**< File to Atlas. */
    FSLIB::AnnotationSet::SPtr          m_pAnnotationSet;   /**< Annotation set. */
    QString                             m_sSurfaceDir;      /**< File to Surface. */
    FSLIB::SurfaceSet::SPtr             m_pSurfaceSet;      /**< Surface set. */

    FIFFLIB::FiffInfo::SPtr             m_pFiffInfo;        /**< Fiff information. */
    FIFFLIB::FiffInfo::SPtr             m_pFiffInfoEvoked;  /**< Fiff information of the evoked. */
    FIFFLIB::FiffInfoBase::SPtr         m_pFiffInfoForward; /**< Fiff information of the forward solution. */

    QStringList                         m_qListPickChannels;        /**< Channels to pick */

    INVERSELIB::PwlRapMusic::SPtr       m_pPwlRapMusic;     /**< Powell RAP MUSIC. */
    qint32                              m_iDownSample;      /**< Sampling rate */

//    RealTimeSourceEstimate::SPtr m_pRTSE_MNE; /**< Source Estimate output channel. */
};

} // NAMESPACE

#endif // RTCMUSIC_H
