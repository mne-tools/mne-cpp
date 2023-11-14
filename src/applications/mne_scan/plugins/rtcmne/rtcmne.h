//=============================================================================================================
/**
 * @file     rtcmne.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
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

#include <scShared/Plugins/abstractalgorithm.h>

#include <utils/generics/circularbuffer.h>

#include <fiff/fiff_evoked.h>

#include <mne/mne_inverse_operator.h>

#include <thread>
#include <atomic>

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
    class RealTimeFwdSolution;
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
class RTCMNESHARED_EXPORT RtcMne : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "rtcmne.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

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
     * Slot called when the fiff info is to be calculated.
     */
    bool calcFiffInfo();

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
     * @param[in] pMeasurement   The evoked to be appended.
     */
    void updateRTE(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Slot to update the inverse operator
     *
     * @param[in] invOp    The inverse operator to update.
     */
    void updateInvOp(const MNELIB::MNEInverseOperator& invOp);

    //=========================================================================================================
    /**
     * Slot to update the real time forward solution
     */
    void updateRTFS(SCMEASLIB::Measurement::SPtr pMeasurement);

protected:
    //=========================================================================================================
    /**
     * Slot called when the method changed.
     *
     * @param[in] method        The new method.
     */
    void onMethodChanged(const QString &method);

    //=========================================================================================================
    /**
     * Slot called when the trigger type changed.
     *
     * @param[in] triggerType        The new trigger type.
     */
    void onTriggerTypeChanged(const QString& triggerType);

    //=========================================================================================================
    /**
     * Slot called when the time point changes.
     *
     * @param[in] iTimePointMs        The new time point in ms.
     */
    void onTimePointValueChanged(int iTimePointMs);

    virtual void run();

    QSharedPointer<SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeFwdSolution> >           m_pRTFSInput;               /**< The RealTimeFwdSolution input.*/
    QSharedPointer<SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray> >      m_pRTMSAInput;              /**< The RealTimeMultiSampleArray input.*/
    QSharedPointer<SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeEvokedSet> >             m_pRTESInput;               /**< The RealTimeEvoked input.*/
    QSharedPointer<SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeCov> >                   m_pRTCInput;                /**< The RealTimeCov input.*/
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSourceEstimate> >       m_pRTSEOutput;              /**< The RealTimeSourceEstimate output.*/
    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double >                                 m_pCircularMatrixBuffer;    /**< Holds incoming RealTimeMultiSampleArray data.*/
    QSharedPointer<UTILSLIB::CircularBuffer<FIFFLIB::FiffEvoked> >                          m_pCircularEvokedBuffer;    /**< Holds incoming RealTimeMultiSampleArray data.*/
    QSharedPointer<RTPROCESSINGLIB::RtInvOp>                                                m_pRtInvOp;                 /**< Real-time inverse operator. */
    QSharedPointer<MNELIB::MNEForwardSolution>                                              m_pFwd;                     /**< Forward solution. */
    QSharedPointer<FIFFLIB::FiffCov>                                                        m_pNoiseCov;                     /**< Noise Covariance Matrix. */
    QSharedPointer<FSLIB::AnnotationSet>                                                    m_pAnnotationSet;           /**< Annotation set. */
    QSharedPointer<FSLIB::SurfaceSet>                                                       m_pSurfaceSet;              /**< Surface set. */
    QSharedPointer<FIFFLIB::FiffInfoBase>                                                   m_pFiffInfoForward;         /**< Fiff information of the forward solution. */
    QSharedPointer<FIFFLIB::FiffInfo>                                                       m_pFiffInfo;                /**< Fiff information. */
    QSharedPointer<FIFFLIB::FiffInfo>                                                       m_pFiffInfoInput;           /**< Fiff information of the evoked. */

    bool                            m_bEvokedInput;             /**< Flag whether an evoked input was received. */
    bool                            m_bRawInput;                /**< Flag whether a raw data input was received. */
    bool                            m_bUpdateMinimumNorm;       /**< Flag whether to update the miniumum norm object. */

    QMutex                          m_qMutex;                   /**< The mutex ensuring thread safety. */
    QFuture<void>                   m_future;                   /**< The future monitoring the clustering. */

    FIFFLIB::FiffEvoked             m_currentEvoked;
    FIFFLIB::FiffCoordTrans         m_mriHeadTrans;             /**< the Mri Head transformation. */

    qint32                          m_iNumAverages;             /**< The number of trials/averages to store. */
    qint32                          m_iDownSample;              /**< Down sample factor. */
    qint32                          m_iTimePointSps;            /**< The time point to pick from the data in samples. */

    QString                         m_sAtlasDir;                /**< File to Atlas. */
    QString                         m_sSurfaceDir;              /**< File to Surface. */
    QString                         m_sAvrType;                 /**< The average type. */
    QString                         m_sMethod;                  /**< The method: "MNE" | "dSPM" | "sLORETA". */
    QFile                           m_fMriHeadTrans;            /**< The Head - Mri transformation. */

    QStringList                     m_qListCovChNames;          /**< Covariance channel names. */
    QStringList                     m_qListPickChannels;        /**< Channels to pick. */

    MNELIB::MNEInverseOperator      m_invOp;                    /**< The inverse operator. */

    std::thread             m_OutputProcessingThread;
    std::atomic_bool        m_bProcessOutput;

signals:
    void responsibleTriggerTypesChanged(const QStringList& lResponsibleTriggerTypes);

};
} // NAMESPACE

#endif // RTCMNE_H
