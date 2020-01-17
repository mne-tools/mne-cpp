//=============================================================================================================
/**
* @file     rtcmne.cpp
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
* @brief    Definition of the RtcMne class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtcmne.h"

#include "FormFiles/rtcmnesetupwidget.h"

#include <disp/viewers/minimumnormsettingsview.h>

#include <fs/annotationset.h>
#include <fs/surfaceset.h>

#include <fiff/fiff_info.h>

#include <mne/mne_forwardsolution.h>
#include <mne/mne_sourceestimate.h>
#include <mne/mne_epoch_data_list.h>

#include <inverse/minimumNorm/minimumnorm.h>

#include <rtprocessing/rtinvop.h>

#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimecov.h>
#include <scMeas/realtimeevokedset.h>

#include <utils/ioutils.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtConcurrent>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTCMNEPLUGIN;
using namespace FIFFLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace INVERSELIB;
using namespace RTPROCESSINGLIB;
using namespace SCSHAREDLIB;
using namespace IOBUFFER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtcMne::RtcMne()
: m_bIsRunning(false)
, m_bReceiveData(false)
, m_bProcessData(false)
, m_bFinishedClustering(false)
, m_qFileFwdSolution(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif")
, m_sAtlasDir(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/label")
, m_sSurfaceDir(QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/surf")
, m_iNumAverages(1)
, m_iDownSample(1)
, m_sAvrType("3")
, m_pMinimumNormSettingsView(MinimumNormSettingsView::SPtr::create())
, m_sMethod("dSPM")
, m_iTimePointSps(-1)
{

}


//*************************************************************************************************************

RtcMne::~RtcMne()
{
    m_future.waitForFinished();

    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> RtcMne::clone() const
{
    QSharedPointer<RtcMne> pRtcMneClone(new RtcMne());
    return pRtcMneClone;
}


//*************************************************************************************************************

void RtcMne::init()
{
    // Inits
    m_pFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(m_qFileFwdSolution, false, true));
    m_pAnnotationSet = AnnotationSet::SPtr(new AnnotationSet(m_sAtlasDir+"/lh.aparc.a2009s.annot", m_sAtlasDir+"/rh.aparc.a2009s.annot"));
    m_pSurfaceSet = SurfaceSet::SPtr(new SurfaceSet(m_sSurfaceDir+"/lh.pial", m_sSurfaceDir+"/rh.pial"));

    // Input
    m_pRTMSAInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "MNE RTMSA In", "MNE real-time multi sample array input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify,
            this, &RtcMne::updateRTMSA, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    m_pRTESInput = PluginInputData<RealTimeEvokedSet>::create(this, "MNE RTE In", "MNE real-time evoked input data");
    connect(m_pRTESInput.data(), &PluginInputConnector::notify,
            this, &RtcMne::updateRTE, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTESInput);

    m_pRTCInput = PluginInputData<RealTimeCov>::create(this, "MNE RTC In", "MNE real-time covariance input data");
    connect(m_pRTCInput.data(), &PluginInputConnector::notify,
            this, &RtcMne::updateRTC, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTCInput);

    // Output
    m_pRTSEOutput = PluginOutputData<RealTimeSourceEstimate>::create(this, "MNE Out", "MNE output data");
    m_outputConnectors.append(m_pRTSEOutput);
    m_pRTSEOutput->data()->setName(this->getName());//Provide name to auto store widget settings

    //Add control widgets to output data (will be used by QuickControlView in RealTimeSourceEstimateWidget)
    connect(m_pMinimumNormSettingsView.data(), &MinimumNormSettingsView::methodChanged,
            this, &RtcMne::onMethodChanged);
    connect(m_pMinimumNormSettingsView.data(), &MinimumNormSettingsView::triggerTypeChanged,
            this, &RtcMne::onTriggerTypeChanged);
    connect(m_pMinimumNormSettingsView.data(), &MinimumNormSettingsView::timePointChanged,
            this, &RtcMne::onTimePointValueChanged);

    m_pRTSEOutput->data()->addControlWidget(m_pMinimumNormSettingsView);

    // start clustering
    QFuture<void> m_future = QtConcurrent::run(this, &RtcMne::doClustering);

    // Set the fwd, annotation and surface data
    m_pRTSEOutput->data()->setAnnotSet(m_pAnnotationSet);
    m_pRTSEOutput->data()->setSurfSet(m_pSurfaceSet);
    m_pRTSEOutput->data()->setFwdSolution(m_pClusteredFwd);
}


//*************************************************************************************************************

void RtcMne::unload()
{

}


//*************************************************************************************************************

void RtcMne::calcFiffInfo()
{
    QMutexLocker locker(&m_qMutex);

    if(m_qListCovChNames.size() > 0 && m_pFiffInfoInput && m_pFiffInfoForward)
    {
        qDebug() << "RtcMne::calcFiffInfoFiff - Infos available";

//        qDebug() << "RtcMne::calcFiffInfo - m_qListCovChNames" << m_qListCovChNames;
//        qDebug() << "RtcMne::calcFiffInfo - m_pFiffInfoForward->ch_names" << m_pFiffInfoForward->ch_names;
//        qDebug() << "RtcMne::calcFiffInfo - m_pFiffInfoInput->ch_names" << m_pFiffInfoInput->ch_names;

        // Align channel names of the forward solution to the incoming averaged (currently acquired) data
        // Find out whether the forward solution depends on only MEG, EEG or both MEG and EEG channels
        QStringList forwardChannelsTypes;
        m_pFiffInfoForward->ch_names.clear();
        int counter = 0;

        for(qint32 x = 0; x < m_pFiffInfoForward->chs.size(); ++x) {
            if(forwardChannelsTypes.contains("MEG") && forwardChannelsTypes.contains("EEG"))
                break;

            if(m_pFiffInfoForward->chs[x].kind == FIFFV_MEG_CH && !forwardChannelsTypes.contains("MEG"))
                forwardChannelsTypes<<"MEG";

            if(m_pFiffInfoForward->chs[x].kind == FIFFV_EEG_CH && !forwardChannelsTypes.contains("EEG"))
                forwardChannelsTypes<<"EEG";
        }

        //If only MEG channels are used
        if(forwardChannelsTypes.contains("MEG") && !forwardChannelsTypes.contains("EEG")) {
            for(qint32 x = 0; x < m_pFiffInfoInput->chs.size(); ++x)
            {
                if(m_pFiffInfoInput->chs[x].kind == FIFFV_MEG_CH) {
                    m_pFiffInfoForward->chs[counter].ch_name = m_pFiffInfoInput->chs[x].ch_name;
                    m_pFiffInfoForward->ch_names << m_pFiffInfoInput->chs[x].ch_name;
                    counter++;
                }
            }
        }

        //If only EEG channels are used
        if(!forwardChannelsTypes.contains("MEG") && forwardChannelsTypes.contains("EEG")) {
            for(qint32 x = 0; x < m_pFiffInfoInput->chs.size(); ++x)
            {
                if(m_pFiffInfoInput->chs[x].kind == FIFFV_EEG_CH) {
                    m_pFiffInfoForward->chs[counter].ch_name = m_pFiffInfoInput->chs[x].ch_name;
                    m_pFiffInfoForward->ch_names << m_pFiffInfoInput->chs[x].ch_name;
                    counter++;
                }
            }
        }

        //If both MEG and EEG channels are used
        if(forwardChannelsTypes.contains("MEG") && forwardChannelsTypes.contains("EEG")) {
            //qDebug()<<"RtcMne::calcFiffInfo - MEG EEG fwd solution";
            for(qint32 x = 0; x < m_pFiffInfoInput->chs.size(); ++x)
            {
                if(m_pFiffInfoInput->chs[x].kind == FIFFV_MEG_CH || m_pFiffInfoInput->chs[x].kind == FIFFV_EEG_CH) {
                    m_pFiffInfoForward->chs[counter].ch_name = m_pFiffInfoInput->chs[x].ch_name;
                    m_pFiffInfoForward->ch_names << m_pFiffInfoInput->chs[x].ch_name;
                    counter++;
                }
            }
        }

        //Pick only channels which are present in all data structures (covariance, evoked and forward)
        QStringList tmp_pick_ch_names;
        foreach (const QString &ch, m_pFiffInfoForward->ch_names)
        {
            if(m_pFiffInfoInput->ch_names.contains(ch))
                tmp_pick_ch_names << ch;
        }
        m_qListPickChannels.clear();

        foreach (const QString &ch, tmp_pick_ch_names)
        {
            if(m_qListCovChNames.contains(ch))
                m_qListPickChannels << ch;
        }
        RowVectorXi sel = m_pFiffInfoInput->pick_channels(m_qListPickChannels);

        //qDebug() << "RtcMne::calcFiffInfo - m_qListPickChannels.size()" << m_qListPickChannels.size();
        //qDebug() << "RtcMne::calcFiffInfo - m_qListPickChannels" << m_qListPickChannels;

        m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(m_pFiffInfoInput->pick_info(sel)));

        m_pRTSEOutput->data()->setFiffInfo(m_pFiffInfo);

        qDebug() << "RtcMne::calcFiffInfo - m_pFiffInfo" << m_pFiffInfo->ch_names;
    }
}


//*************************************************************************************************************

void RtcMne::doClustering()
{
    emit clusteringStarted();

    m_qMutex.lock();
    m_bFinishedClustering = false;
    m_pClusteredFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(m_pFwd->cluster_forward_solution(*m_pAnnotationSet.data(), 200)));
    //m_pClusteredFwd = m_pFwd;
    m_pRTSEOutput->data()->setFwdSolution(m_pClusteredFwd);

    m_qMutex.unlock();

    finishedClustering();
}


//*************************************************************************************************************

void RtcMne::finishedClustering()
{
    m_qMutex.lock();
    m_bFinishedClustering = true;
    m_pFiffInfoForward = QSharedPointer<FiffInfoBase>(new FiffInfoBase(m_pClusteredFwd->info));
    m_qMutex.unlock();

    emit clusteringFinished();
}


//*************************************************************************************************************

bool RtcMne::start()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//        QThread::wait();

    if(m_bFinishedClustering) {
        m_bIsRunning = true;
        QThread::start();
        return true;
    } else {
        return false;
    }
}


//*************************************************************************************************************

bool RtcMne::stop()
{
    m_bIsRunning = false;

    // Only clear if buffers have been initialised
    if(m_bProcessData) {
        m_qVecFiffEvoked.clear();
    }

    m_qListCovChNames.clear();

    // Stop filling buffers with data from the inputs
    m_bProcessData = false;

    m_bReceiveData = false;

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType RtcMne::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString RtcMne::getName() const
{
    return "RTC-MNE";
}


//*************************************************************************************************************

QWidget* RtcMne::setupWidget()
{
    RtcMneSetupWidget* setupWidget = new RtcMneSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    if(!m_bFinishedClustering)
        setupWidget->setClusteringState();

    connect(this, &RtcMne::clusteringStarted,
            setupWidget, &RtcMneSetupWidget::setClusteringState);
    connect(this, &RtcMne::clusteringFinished,
            setupWidget, &RtcMneSetupWidget::setSetupState);

    return setupWidget;
}


//*************************************************************************************************************

void RtcMne::updateRTMSA(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA) {
        if(!m_bReceiveData) {
            return;
        }

        //Check if buffer initialized
        if(!m_pMatrixDataBuffer) {
            m_pMatrixDataBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));
        }

        //Fiff Information of the RTMSA
        if(!m_pFiffInfoInput) {
            m_pFiffInfoInput = pRTMSA->info();
            m_iNumAverages = 1;
        }

        if(m_bProcessData) {
            for(qint32 i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                // Check for artifacts
                QMap<QString,double> mapReject;
                mapReject.insert("eog", 150e-06);

                bool bArtifactDetected = MNEEpochDataList::checkForArtifact(pRTMSA->getMultiSampleArray()[i],
                                                                            *m_pFiffInfoInput,
                                                                            mapReject);

                if(!bArtifactDetected) {
                    m_pMatrixDataBuffer->push(&pRTMSA->getMultiSampleArray()[i]);
                } else {
                    qDebug() << "RtcMne::updateRTMSA - Reject data block";
                }

            }
        }
    }
}


//*************************************************************************************************************

void RtcMne::updateRTC(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeCov> pRTC = pMeasurement.dynamicCast<RealTimeCov>();

    //MEG
    if(pRTC && m_bReceiveData) {
        // Init Real-Time inverse estimator
        if(!m_pRtInvOp && m_pFiffInfo && m_pClusteredFwd) {
            m_pRtInvOp = RtInvOp::SPtr(new RtInvOp(m_pFiffInfo, m_pClusteredFwd));
            connect(m_pRtInvOp.data(), &RtInvOp::invOperatorCalculated,
                    this, &RtcMne::updateInvOp);
        }

        //Fiff Information of the covariance
        if(m_qListCovChNames.size() != pRTC->getValue()->names.size()) {
            m_qListCovChNames = pRTC->getValue()->names;
        }

        if(m_bProcessData && m_pRtInvOp){
            m_pRtInvOp->append(*pRTC->getValue());
        }
    }
}


//*************************************************************************************************************

void RtcMne::updateRTE(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeEvokedSet> pRTES = pMeasurement.dynamicCast<RealTimeEvokedSet>();

    if(!pRTES) {
        return;
    }

    QMutexLocker locker(&m_qMutex);

    QStringList lResponsibleTriggerTypes = pRTES->getResponsibleTriggerTypes();

    if(m_pMinimumNormSettingsView) {
        m_pMinimumNormSettingsView->setTriggerTypes(lResponsibleTriggerTypes);
    }

    if(!m_bReceiveData || !lResponsibleTriggerTypes.contains(m_sAvrType)) {
        return;
    }

    FiffEvokedSet::SPtr pFiffEvokedSet = pRTES->getValue();

    //Fiff Information of the evoked
    if(!m_pFiffInfoInput && pFiffEvokedSet->evoked.size() > 0) {
        for(int i = 0; i < pFiffEvokedSet->evoked.size(); ++i) {
            if(pFiffEvokedSet->evoked.at(i).comment == m_sAvrType) {
                m_pFiffInfoInput = QSharedPointer<FiffInfo>(new FiffInfo(pFiffEvokedSet->evoked.at(i).info));
                break;
            }
        }
    }

    if(m_bProcessData) {
        for(int i = 0; i < pFiffEvokedSet->evoked.size(); ++i) {
            if(pFiffEvokedSet->evoked.at(i).comment == m_sAvrType) {
                //qDebug()<<"RtcMne::updateRTE - average found type" << m_sAvrType;
                m_qVecFiffEvoked.push_back(pFiffEvokedSet->evoked.at(i).pick_channels(m_qListPickChannels));
                break;
            }
        }
    }
}


//*************************************************************************************************************

void RtcMne::updateInvOp(const MNEInverseOperator& invOp)
{
    QMutexLocker locker(&m_qMutex);

    m_invOp = invOp;

    double snr = 1.0;
    double lambda2 = 1.0 / pow(snr, 2); //ToDo estimate lambda using covariance

    m_pMinimumNorm = MinimumNorm::SPtr(new MinimumNorm(m_invOp, lambda2, m_sMethod));

    //Set up the inverse according to the parameters
    // Use 1 nave here because in case of evoked data as input the minimum norm will always be updated when the source estimate is calculated (see run method).
    m_pMinimumNorm->doInverseSetup(1,true);
}


//*************************************************************************************************************

void RtcMne::onMethodChanged(const QString& method)
{
    m_sMethod = method;

    QMutexLocker locker(&m_qMutex);

    if(m_pMinimumNorm) {
        double snr = 1.0;
        double lambda2 = 1.0 / pow(snr, 2); //ToDo estimate lambda using covariance
        m_pMinimumNorm = MinimumNorm::SPtr(new MinimumNorm(m_invOp, lambda2, m_sMethod));

        // Set up the inverse according to the parameters.
        // Use 1 nave here because in case of evoked data as input the minimum norm will always be updated when the source estimate is calculated (see run method).
        m_pMinimumNorm->doInverseSetup(1,true);
    }
}


//*************************************************************************************************************

void RtcMne::onTriggerTypeChanged(const QString& triggerType)
{
    m_sAvrType = triggerType;
}


//*************************************************************************************************************

void RtcMne::onTimePointValueChanged(int iTimePointMs)
{
    QMutexLocker locker(&m_qMutex);

    if(m_pFiffInfoInput) {
        m_iTimePointSps = m_pFiffInfoInput->sfreq * (float)iTimePointMs * 0.001;

        if(m_bProcessData) {
            m_qVecFiffEvoked.push_back(m_currentEvoked);
        }
    }
}


//*************************************************************************************************************

void RtcMne::run()
{
    // Start receiving data
    m_qMutex.lock();
    m_bReceiveData = true;
    m_qMutex.unlock();

    // Mode 1: Use covariance and inverse operator calcualted by incoming stream
    while(true) {
        {
            QMutexLocker locker(&m_qMutex);
            if(m_pFiffInfo)
                break;
        }

        calcFiffInfo();
        msleep(10);// Wait for fiff Info
    }

//    qDebug() << "RtcMne::run - m_pClusteredFwd->info.ch_names" << m_pClusteredFwd->info.ch_names;
//    qDebug() << "RtcMne::run - m_pFiffInfo->ch_names" << m_pFiffInfo->ch_names;

    // Mode 1: End

//    // Mode 2: Use covariance and inverse operator loaded from pre calculated files
//    QFile t_fileCov(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
//    FiffCov noise_cov(t_fileCov);
//    m_qListCovChNames = noise_cov.names;

//    while(true) {
//        {
//            QMutexLocker locker(&m_qMutex);
//            if(m_pFiffInfoInput)
//                break;
//        }

//        msleep(10);// Wait for fiff Info
//    }

//    // regularize noise covariance
//    noise_cov = noise_cov.regularize(*m_pFiffInfoInput,
//                                     0.05,
//                                     0.05,
//                                     0.1,
//                                     true);

//    // make an inverse operator
//    m_invOp = MNEInverseOperator(*m_pFiffInfoInput,
//                                 *m_pClusteredFwd,
//                                 noise_cov,
//                                 0.2f,
//                                 0.8f);

//    double snr = 3.0;
//    double lambda2 = 1.0 / pow(snr, 2); //ToDo estimate lambda using covariance
//    QString method("dSPM"); //"MNE" | "dSPM" | "sLORETA"
//    m_pMinimumNorm = MinimumNorm::SPtr(new MinimumNorm(m_invOp,
//                                                       lambda2,
//                                                       method));
//    m_pMinimumNorm->doInverseSetup(1,true);

//    m_pRTSEOutput->data()->setFiffInfo(m_pFiffInfoInput);

////    qDebug() << "RtcMne::run - m_pClusteredFwd->info.ch_names" << m_pClusteredFwd->info.ch_names;
////    qDebug() << "RtcMne::run - m_pFiffInfoInput->ch_names" << m_pFiffInfoInput->ch_names;

//    // Mode 2: End

    // Init parameters
    m_bProcessData = true;

    qint32 skip_count = 0;
    qint32 t_evokedSize;
    MatrixXd rawSegment;
    MatrixXd data;
    qint32 j;
    float tmin, tstep;
    MNESourceEstimate sourceEstimate;

    // Start processing data
    while(m_bIsRunning) {
        //Process raw data from a RTMSA input
        if(m_pMatrixDataBuffer) {
            //qDebug()<<"RtcMne::run - Processing RTMSA data";

            if(m_pMinimumNorm && ((skip_count % m_iDownSample) == 0)) {
                rawSegment = m_pMatrixDataBuffer->pop();

                //Pick the same channels as in the inverse operator
                m_qMutex.lock();
                data.resize(m_invOp.noise_cov->names.size(), rawSegment.cols());

                for(j = 0; j < m_invOp.noise_cov->names.size(); ++j) {
                    data.row(j) = rawSegment.row(m_pFiffInfoInput->ch_names.indexOf(m_invOp.noise_cov->names.at(j)));
                }

                tmin = 0.0f;
                tstep = 1.0f / m_pFiffInfoInput->sfreq;

                //TODO: Add picking here. See evoked part as input.
                sourceEstimate = m_pMinimumNorm->calculateInverse(data,
                                                                  tmin,
                                                                  tstep,
                                                                  true);

                m_qMutex.unlock();

                if(!sourceEstimate.isEmpty()) {
                    //qInfo() << QDateTime::currentDateTime().toString("hh:mm:ss.z") << m_iBlockNumberProcessed++ << "MNE Processed";
                    m_pRTSEOutput->data()->setValue(sourceEstimate);
                }
            } else {
                m_pMatrixDataBuffer->pop();
            }

            ++skip_count;
        }

        //Process data from averaging input
        m_qMutex.lock();
        t_evokedSize = m_qVecFiffEvoked.size();
        m_qMutex.unlock();

        if(t_evokedSize > 0) {
            //qDebug() << "RtcMne::run - Processing RTE data - t_evokedSize" << t_evokedSize;
            if(m_pMinimumNorm && ((skip_count % m_iDownSample) == 0)) {

                m_qMutex.lock();
                m_currentEvoked = m_qVecFiffEvoked.takeFirst();
                QElapsedTimer time;
                time.start();
                //qDebug()<<"RtcMne::run - t_fiffEvoked.data.rows()"<<t_fiffEvoked.data.rows();

                sourceEstimate = m_pMinimumNorm->calculateInverse(m_currentEvoked);

//                else {
//                    m_currentEvoked = m_currentEvoked.pick_channels(m_invOp.noise_cov->names);
//                    tmin = 0.0f;
//                    tstep = 1.0f / m_pFiffInfoInput->sfreq;
//                    m_pMinimumNorm->doInverseSetup(m_currentEvoked.nave);
//                    sourceEstimate = m_pMinimumNorm->calculateInverse(m_currentEvoked.data.block(0,m_iTimePointSps,m_currentEvoked.data.rows(),1),
//                                                                      tmin,
//                                                                      tstep);
//                }

                m_qMutex.unlock();

                if(!sourceEstimate.isEmpty()) {
                    //qInfo() << time.elapsed() << m_iBlockNumberProcessed << "MNE Time";
                    //qInfo() << QDateTime::currentDateTime().toString("hh:mm:ss.z") << m_iBlockNumberProcessed++ << "MNE Processed";

//                    if(m_iTimePointSps < m_currentEvoked.data.cols()) {
//                        m_pRTSEOutput->data()->setValue(sourceEstimate.reduce(m_iTimePointSps,1));
//                    } else {
                        m_pRTSEOutput->data()->setValue(sourceEstimate);
//                    }
                }

            } else {
                m_qMutex.lock();
                m_qVecFiffEvoked.pop_front();
                m_qMutex.unlock();
            }

            ++skip_count;
        }
    }
}
