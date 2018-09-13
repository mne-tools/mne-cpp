//=============================================================================================================
/**
* @file     mne.h
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
* @brief    Contains the declaration of the MNE class.
*
*/

#ifndef MNE_H
#define MNE_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include <scShared/Interfaces/IAlgorithm.h>

#include <utils/generics/circularmatrixbuffer.h>

#include <fs/annotationset.h>
#include <fs/surfaceset.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_evoked.h>
#include <mne/mne_forwardsolution.h>
#include <mne/mne_sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>
#include <realtime/rtProcessing/rtinvop.h>

#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimecov.h>
#include <scMeas/realtimeevokedset.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISPLIB {
    class MinimumNormSettingsView;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEPLUGIN
//=============================================================================================================

namespace MNEPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVERSELIB;
using namespace REALTIMELIB;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;


//*************************************************************************************************************
//=============================================================================================================
// MNEPLUGIN FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS MNE
*
* @brief The MNE class provides a dummy algorithm structure.
*/
class MNESHARED_EXPORT MNE : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "mne.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

    friend class MNESetupWidget;

public:

    //=========================================================================================================
    /**
    * Constructs a MNE.
    */
    MNE();
    //=========================================================================================================
    /**
    * Destroys the MNE.
    */
    ~MNE();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    //=========================================================================================================
    /**
    * Initialise the MNE.
    */
    void init();

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
    * Slot to update the real time multi sample array data
    *
    */
    void updateRTMSA(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Slot to update the fiff covariance
    *
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

    virtual void run();

    PluginInputData<RealTimeMultiSampleArray>::SPtr         m_pRTMSAInput;                  /**< The RealTimeMultiSampleArray input.*/
    PluginInputData<RealTimeEvokedSet>::SPtr                m_pRTESInput;                   /**< The RealTimeEvoked input.*/
    PluginInputData<RealTimeCov>::SPtr                      m_pRTCInput;                    /**< The RealTimeCov input.*/

    PluginOutputData<RealTimeSourceEstimate>::SPtr          m_pRTSEOutput;                  /**< The RealTimeSourceEstimate output.*/

    CircularMatrixBuffer<double>::SPtr                      m_pMatrixDataBuffer;            /**< Holds incoming RealTimeMultiSampleArray data.*/

    QPointer<DISPLIB::MinimumNormSettingsView>              m_pMinimumNormSettingsView;    /**< The minimum norm settings widget which will be added to the Quick Control view.*/

    QMutex m_qMutex;
    QFuture<void> m_future;

    QVector<FiffEvoked> m_qVecFiffEvoked;
    qint32 m_iNumAverages;

    bool m_bIsRunning;      /**< If source lab is running */
    bool m_bReceiveData;    /**< If thread is ready to receive data */
    bool m_bProcessData;    /**< If data should be received for processing */

    //MNE stuff
    QFile                       m_qFileFwdSolution; /**< File to forward solution. */
    MNEForwardSolution::SPtr    m_pFwd;             /**< Forward solution. */
    MNEForwardSolution::SPtr    m_pClusteredFwd;    /**< Clustered forward solution. */

    bool m_bFinishedClustering;                     /**< If clustered forward solution is available. */

    QString                     m_sAtlasDir;        /**< File to Atlas. */
    AnnotationSet::SPtr         m_pAnnotationSet;   /**< Annotation set. */
    QString                     m_sSurfaceDir;      /**< File to Surface. */
    SurfaceSet::SPtr            m_pSurfaceSet;      /**< Surface set. */

    FiffInfo::SPtr              m_pFiffInfo;        /**< Fiff information. */
    FiffInfo::SPtr              m_pFiffInfoInput;   /**< Fiff information of the evoked. */
    QStringList                 m_qListCovChNames;  /**< Covariance channel names. */
    FiffInfoBase::SPtr          m_pFiffInfoForward; /**< Fiff information of the forward solution. */

    QStringList                 m_qListPickChannels;        /**< Channels to pick */

    RtInvOp::SPtr               m_pRtInvOp;         /**< Real-time inverse operator. */
    MNEInverseOperator          m_invOp;            /**< The inverse operator. */

    MinimumNorm::SPtr           m_pMinimumNorm;     /**< Minimum Norm Estimation. */
    qint32                      m_iDownSample;      /**< Sampling rate */

    QString                     m_sAvrType;         /**< The average type */
    QString                     m_sMethod;          /**< The method: "MNE" | "dSPM" | "sLORETA" */

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
};

} // NAMESPACE

#endif // MNE_H
