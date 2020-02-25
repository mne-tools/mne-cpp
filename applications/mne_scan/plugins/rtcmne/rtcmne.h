//=============================================================================================================
/**
 * @file     rtcmne.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the RtcMne class.
 *
 */

#ifndef RTCMNE_H
#define RTCMNE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtcmne_global.h"

#include <scShared/Interfaces/IAlgorithm.h>

#include <utils/generics/circularmatrixbuffer.h>

#include <fiff/fiff_evoked.h>

#include <mne/mne_inverse_operator.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFuture>
#include <QPointer>
#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISPLIB {
    class MinimumNormSettingsView;
}

namespace MNELIB {
    class MNEForwardSolution;
    class MNEInverseOperator;
}

namespace FIFFLIB {
    class FiffInfo;
    class FiffInfoBase;
}

namespace INVERSELIB {
    class MinimumNorm;
}

namespace RTPROCESSINGLIB {
    class RtInvOp;
}

namespace FSLIB {
    class AnnotationSet;
    class SurfaceSet;
}

namespace SCMEASLIB {
    class RealTimeEvokedSet;
    class RealTimeMultiSampleArray;
    class RealTimeCov;
    class RealTimeSourceEstimate;
}

//=============================================================================================================
// DEFINE NAMESPACE RTCMNEPLUGIN
//=============================================================================================================

namespace RTCMNEPLUGIN
{

//=============================================================================================================
// RTCRTCMNEPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS RtcMne
 *
 * @brief The RtcMne class provides a plugin for estimating distributed source localization in real-time.
 */
class RTCMNESHARED_EXPORT RtcMne : public SCSHAREDLIB::IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "rtcmne.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

    friend class RtcMneSetupWidget;

public:
    //=========================================================================================================
    /**
     * Constructs a RtcMne.
     */
    RtcMne();

    //=========================================================================================================
    /**
     * Destroys the RtcMne.
     */
    ~RtcMne();

    //=========================================================================================================
    /**
     * IAlgorithm functions
     */
    virtual QSharedPointer<SCSHAREDLIB::IPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual SCSHAREDLIB::IPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
     * Slot called when the fiff info is to be calculated.
     */
    void calcFiffInfo();

    //=========================================================================================================
    /**
     * Slot called to do the clustering.
     */
    void doClustering();

    //=========================================================================================================
    /**
     * Slot called when the clustering is finished.
     */
    void finishedClustering();

    //=========================================================================================================
    /**
     * Slot to update the real time multi sample array data
     */
    void updateRTMSA(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Slot to update the fiff covariance
     */
    void updateRTC(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Slot to update the fiff evoked
     *
     * @param[in] pMeasurement   The evoked to be appended
     */
    void updateRTE(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Slot to update the inverse operator
     *
     * @param[in] invOp    The inverse operator to update
     */
    void updateInvOp(const MNELIB::MNEInverseOperator& invOp);

protected:
    //=========================================================================================================
    /**
     * Slot called when the method changed.
     *
     * @param [in] method        The new method.
     */
    void onMethodChanged(const QString &method);

    //=========================================================================================================
    /**
     * Slot called when the trigger type changed.
     *
     * @param [in] triggerType        The new trigger type.
     */
    void onTriggerTypeChanged(const QString& triggerType);

    //=========================================================================================================
    /**
     * Slot called when the time point changes.
     *
     * @param [in] iTimePointMs        The new time point in ms.
     */
    void onTimePointValueChanged(int iTimePointMs);

    virtual void run();

    QSharedPointer<SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray> >      m_pRTMSAInput;              /**< The RealTimeMultiSampleArray input.*/
    QSharedPointer<SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeEvokedSet> >             m_pRTESInput;               /**< The RealTimeEvoked input.*/
    QSharedPointer<SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeCov> >                   m_pRTCInput;                /**< The RealTimeCov input.*/
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSourceEstimate> >       m_pRTSEOutput;              /**< The RealTimeSourceEstimate output.*/
    QSharedPointer<IOBUFFER::CircularMatrixBuffer<double> >                                 m_pMatrixDataBuffer;        /**< Holds incoming RealTimeMultiSampleArray data.*/
    QSharedPointer<INVERSELIB::MinimumNorm>                                                 m_pMinimumNorm;             /**< Minimum Norm Estimation. */
    QSharedPointer<RTPROCESSINGLIB::RtInvOp>                                                m_pRtInvOp;                 /**< Real-time inverse operator. */
    QSharedPointer<MNELIB::MNEForwardSolution>                                              m_pFwd;                     /**< Forward solution. */
    QSharedPointer<MNELIB::MNEForwardSolution>                                              m_pClusteredFwd;            /**< Clustered forward solution. */
    QSharedPointer<FSLIB::AnnotationSet>                                                    m_pAnnotationSet;           /**< Annotation set. */
    QSharedPointer<FSLIB::SurfaceSet>                                                       m_pSurfaceSet;              /**< Surface set. */
    QSharedPointer<FIFFLIB::FiffInfoBase>                                                   m_pFiffInfoForward;         /**< Fiff information of the forward solution. */
    QSharedPointer<FIFFLIB::FiffInfo>                                                       m_pFiffInfo;                /**< Fiff information. */
    QSharedPointer<FIFFLIB::FiffInfo>                                                       m_pFiffInfoInput;           /**< Fiff information of the evoked. */

    QSharedPointer<DISPLIB::MinimumNormSettingsView>                                        m_pMinimumNormSettingsView; /**< The minimum norm settings widget which will be added to the Quick Control view. The QuickControlView will not take ownership. Ownership will be managed by the QSharedPointer.*/

    QMutex                          m_qMutex;                   /**< The mutex ensuring thread safety. */
    QFuture<void>                   m_future;                   /**< The future monitoring the clustering. */

    QVector<FIFFLIB::FiffEvoked>    m_qVecFiffEvoked;           /**< The list of stored averages. */
    FIFFLIB::FiffEvoked             m_currentEvoked;

    qint32                          m_iNumAverages;             /**< The number of trials/averages to store. */
    qint32                          m_iDownSample;              /**< Down sample factor. */
    qint32                          m_iTimePointSps;            /**< The time point to pick from the data in samples. */

    bool                            m_bIsRunning;               /**< If source lab is running. */
    bool                            m_bReceiveData;             /**< If thread is ready to receive data. */
    bool                            m_bProcessData;             /**< If data should be received for processing. */
    bool                            m_bFinishedClustering;      /**< If clustered forward solution is available. */

    QFile                           m_qFileFwdSolution;         /**< File to forward solution. */

    QString                         m_sAtlasDir;                /**< File to Atlas. */
    QString                         m_sSurfaceDir;              /**< File to Surface. */
    QString                         m_sAvrType;                 /**< The average type */
    QString                         m_sMethod;                  /**< The method: "MNE" | "dSPM" | "sLORETA". */

    QStringList                     m_qListCovChNames;          /**< Covariance channel names. */
    QStringList                     m_qListPickChannels;        /**< Channels to pick. */

    MNELIB::MNEInverseOperator      m_invOp;                    /**< The inverse operator. */

signals:
    //=========================================================================================================
    /**
     * Signal when clustering is started
     */
    void clusteringStarted();

    //=========================================================================================================
    /**
     * Signal when clustering has finished
     */
    void clusteringFinished();
};

} // NAMESPACE

#endif // RTCMNE_H
