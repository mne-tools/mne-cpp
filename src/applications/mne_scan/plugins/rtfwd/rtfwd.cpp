//=============================================================================================================
/**
 * @file     rtfwd.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.1
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    Definition of the RtFwd class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfwd.h"

#include <disp/viewers/fwdsettingsview.h>

#include <fwd/computeFwd/compute_fwd.h>
#include <fwd/computeFwd/compute_fwd_settings.h>

#include <inverse/hpiFit/hpifit.h>

#include <fs/annotationset.h>

#include <mne/mne_forwardsolution.h>

#include <scMeas/realtimehpiresult.h>
#include <scMeas/realtimefwdsolution.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTFWDPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace FWDLIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace MNELIB;
using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtFwd::RtFwd()
: m_pFwdSettings(new ComputeFwdSettings)
, m_bBusy(false)
, m_bDoRecomputation(false)
, m_bDoClustering(true)
, m_bDoFwdComputation(false)
{
    // set init values
    m_pFwdSettings->solname = QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/your-solution-fwd.fif";
    m_pFwdSettings->mriname = QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/all-trans.fif";
    m_pFwdSettings->bemname = QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif";
    m_pFwdSettings->srcname = QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-oct-6-src.fif";
    m_pFwdSettings->measname = QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif";
    m_pFwdSettings->transname.clear();
    m_pFwdSettings->eeg_model_name = "Default";
    m_pFwdSettings->include_meg = true;
    m_pFwdSettings->include_eeg = true;
    m_pFwdSettings->accurate = true;
    m_pFwdSettings->mindist = 5.0f/1000.0f;
    m_pFwdSettings->ncluster = 200;

    m_sAtlasDir = QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/label";
}

//=============================================================================================================

RtFwd::~RtFwd()
{
    m_future.waitForFinished();

    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> RtFwd::clone() const
{
    QSharedPointer<RtFwd> pRtFwdClone(new RtFwd);
    return pRtFwdClone;
}

//=============================================================================================================

void RtFwd::init()
{
    // Inits
    m_pAnnotationSet = AnnotationSet::SPtr(new AnnotationSet(m_sAtlasDir+"/lh.aparc.a2009s.annot", m_sAtlasDir+"/rh.aparc.a2009s.annot"));

    // Input
    m_pHpiInput = PluginInputData<RealTimeHpiResult>::create(this, "rtFwd RTHR In", "rtFwd real time HPI result input data");
    connect(m_pHpiInput.data(), &PluginInputConnector::notify,
            this, &RtFwd::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pHpiInput);

    m_pRTMSAInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "rtFwd RTMSA In", "rtFwd real-time multi sample array input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify,
            this, &RtFwd::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    // Output
    m_pRTFSOutput = PluginOutputData<RealTimeFwdSolution>::create(this, "rtFwdOut", "rtFwd real-time forward solution output data");
    m_pRTFSOutput->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTFSOutput);
}

//=============================================================================================================

void RtFwd::unload()
{
    m_future.waitForFinished();
}

//=============================================================================================================

bool RtFwd::start()
{
    // Maybe we can move all of this to the run() method?
    // Read BEM
    QFile t_fBem(m_pFwdSettings->bemname);
    FiffStream::SPtr stream(new FiffStream(&t_fBem));
    if(!stream->open()) {
        QMessageBox msgBox;
        msgBox.setText("The bem model cannot be opend. Chosse another file.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
        msgBox.exec();
        stream->close();
        return false;
    }
    stream->close();

    // Read source space
    QFile t_fSource(m_pFwdSettings->srcname);
    stream = FiffStream::SPtr(new FiffStream(&t_fSource));
    if(!stream->open()) {
        QMessageBox msgBox;
        msgBox.setText("The source space cannot be opend. Chosse another file.");
        msgBox.setText(m_pFwdSettings->srcname);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
        msgBox.exec();
        stream->close();
        return false;
    }
    stream->close();

    // Read MRI transformation
    QFile t_fMri(m_pFwdSettings->mriname);
    stream = FiffStream::SPtr(new FiffStream(&t_fMri));
    if(!stream->open()) {
        QMessageBox msgBox;
        msgBox.setText("The mri - head transformation cannot be opend. Chosse another file.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
        msgBox.exec();
        stream->close();
        return false;
    }
    stream->close();

    // Read measurement
    QFile t_fMeas(m_pFwdSettings->measname);
    stream = FiffStream::SPtr(new FiffStream(&t_fMri));
    if(!stream->open()) {
        QMessageBox msgBox;
        msgBox.setText("The meaurement file cannot be opend. Chosse another file.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
        msgBox.exec();
        stream->close();
        return false;
    }
    stream->close();

    //Start thread
    QThread::start();

    return true;
}

//=============================================================================================================

bool RtFwd::stop()
{
    requestInterruption();
    wait(500);

    m_bPluginControlWidgetsInit = false;

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType RtFwd::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString RtFwd::getName() const
{
    return "Forward Solution";
}

//=============================================================================================================

QWidget* RtFwd::setupWidget()
{
    RtFwdSetupWidget* setupWidget = new RtFwdSetupWidget(this);
    return setupWidget;
}

//=============================================================================================================

void RtFwd::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Fiff information
        m_mutex.lock();
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();
        }
        m_mutex.unlock();

        if(!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }
    } else if(QSharedPointer<RealTimeHpiResult> pRTHPI = pMeasurement.dynamicCast<RealTimeHpiResult>()) {
        //Fiff information
        m_mutex.lock();
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTHPI->getFiffInfo();
        }

        if(!m_bBusy) {
            m_pHpiFitResult = pRTHPI->getValue();
        }
        m_mutex.unlock();

        if(!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }
    }
}

//=============================================================================================================

void RtFwd::initPluginControlWidgets()
{
    bool bFiffInfo = false;
    m_mutex.lock();
    if(m_pFiffInfo) {
        bFiffInfo = true;
    }
    m_mutex.unlock();

    if(bFiffInfo) {
        QList<QWidget*> plControlWidgets;

        FwdSettingsView* pFwdSettingsView = new FwdSettingsView(QString("MNESCAN/%1/").arg(this->getName()));
        connect(this, &RtFwd::guiModeChanged,
                pFwdSettingsView, &FwdSettingsView::setGuiMode);
        pFwdSettingsView->setObjectName("widget_");

        // connect incoming signals
        connect(pFwdSettingsView, &FwdSettingsView::recompStatusChanged,
                this, &RtFwd::onRecompStatusChanged);
        connect(pFwdSettingsView, &FwdSettingsView::clusteringStatusChanged,
                this, &RtFwd::onClusteringStatusChanged);
        connect(pFwdSettingsView, &FwdSettingsView::atlasDirChanged,
                this, &RtFwd::onAtlasDirChanged);
        connect(pFwdSettingsView, &FwdSettingsView::clusterNumberChanged,
                this, &RtFwd::onClusterNumberChanged);
        connect(pFwdSettingsView, &FwdSettingsView::doForwardComputation,
                this, &RtFwd::onDoForwardComputation);

        // connect outgoing signals
        connect(this, &RtFwd::statusInformationChanged,
                pFwdSettingsView, &FwdSettingsView::setRecomputationStatus, Qt::BlockingQueuedConnection);
        connect(this, &RtFwd::fwdSolutionAvailable,
                pFwdSettingsView, &FwdSettingsView::setSolutionInformation, Qt::BlockingQueuedConnection);
        connect(this, &RtFwd::clusteringAvailable,
                pFwdSettingsView, &FwdSettingsView::setClusteredInformation, Qt::BlockingQueuedConnection);

        plControlWidgets.append(pFwdSettingsView);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());

        m_bPluginControlWidgetsInit = true;
    }
}

//=============================================================================================================

void RtFwd::onDoForwardComputation()
{
    m_mutex.lock();
    m_bDoFwdComputation = true;
    // get value for number in cluster and set m_pFwdSettings->ncluster here
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::onRecompStatusChanged(bool bDoRecomputation)
{
    m_mutex.lock();
    if(!m_pHpiInput) {
        QMessageBox msgBox;
        msgBox.setText("Please connect the Hpi plugin.");
        msgBox.exec();
        return;
    }
    m_bDoRecomputation = bDoRecomputation;
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::onClusteringStatusChanged(bool bDoClustering)
{
    if(m_pAnnotationSet->isEmpty()) {
        QMessageBox msgBox;
        msgBox.setText("Please load an annotation set befor clustering.");
        msgBox.exec();
        return;
    }
    m_mutex.lock();
    m_bDoClustering = bDoClustering;
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::onAtlasDirChanged(const QString& sDirPath, const AnnotationSet::SPtr pAnnotationSet)
{
    m_mutex.lock();
    m_sAtlasDir = sDirPath;
    m_pAnnotationSet = pAnnotationSet;
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::onClusterNumberChanged(int iNClusterNumber)
{
    m_mutex.lock();
    m_pFwdSettings->ncluster = iNClusterNumber;
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::run()
{
    // Wait for fiff the info to arrive
    while(true) {
        m_mutex.lock();
        if(m_pFiffInfo) {
            m_mutex.unlock();
            break;
        }
        m_mutex.unlock();
        msleep(200);
    }

    m_mutex.lock();
    m_pFwdSettings->pFiffInfo = m_pFiffInfo;
    m_pRTFSOutput->measurementData()->setFiffInfo(m_pFiffInfo);
    FiffCoordTransOld transMegHeadOld = m_transDevHead.toOld();
    m_mutex.unlock();

    // initialize fwd solution
    emit statusInformationChanged(0);           // initializing
    ComputeFwd::SPtr pComputeFwd = ComputeFwd::SPtr(new ComputeFwd(m_pFwdSettings));

    QFile t_fSolution(m_pFwdSettings->solname);
    MNEForwardSolution::SPtr pFwdSolution;
    MNEForwardSolution::SPtr pClusteredFwd;

    emit statusInformationChanged(4);           // not computed

    // do recomputation if requested, not busy and transformation is different
    bool bIsLargeHeadMovement = false;          // indicate if movement was large
    bool bIsDifferent = false;                  // indicate if incoming transformation matrix is different
    bool bDoRecomputation = false;              // indicate if we want to recompute
    bool bDoClustering = false;                 // indicate if we want to cluster
    bool bFwdReady = false;                     // only cluster if fwd is ready
    bool bHpiConnectected = false;              // only update/recompute if hpi is connected
    bool bDoFwdComputation = false;             // compute forward if requested
    bool bIsInit = false;                       // only recompute if initial fwd solulion is calculated

    while(!isInterruptionRequested()) {
        m_mutex.lock();
        bDoFwdComputation = m_bDoFwdComputation;
        m_mutex.unlock();

        if(bDoFwdComputation) {
            emit statusInformationChanged(1);   // computing
            m_mutex.lock();
            m_bBusy = true;
            m_mutex.unlock();

            // compute and store
            pComputeFwd->calculateFwd();
            pComputeFwd->storeFwd();

            // get Mne Forward Solution (in future this is not necessary, ComputeForward will have this as member)
            pFwdSolution = MNEForwardSolution::SPtr(new MNEForwardSolution(t_fSolution, false, true));

            // emit results to control widget
            emit fwdSolutionAvailable(pFwdSolution->source_ori,
                                      pFwdSolution->coord_frame,
                                      pFwdSolution->nsource,
                                      pFwdSolution->nchan,
                                      pFwdSolution->src.size());

            m_mutex.lock();
            if(!m_bDoClustering) {
                m_pRTFSOutput->measurementData()->setValue(pFwdSolution);
                bFwdReady = false;                  // make sure to not cluster
                emit statusInformationChanged(5);   //finished
            }
            bFwdReady = true;                       // enable cluster
            m_bDoFwdComputation = false;            // don't call this again if not requested
            bIsInit = true;                         // init computation finished -> recomputation possible
            m_mutex.unlock();
        }

        // check if hpi is connected
        m_mutex.lock();
        if(m_pHpiFitResult) {
            bHpiConnectected = true;
        }
        m_mutex.unlock();

        if(bHpiConnectected && bIsInit) {
            // only recompute if hpi is connected
            m_mutex.lock();
            bIsLargeHeadMovement = m_pHpiFitResult->bIsLargeHeadMovement;
            bIsDifferent = !(transMegHeadOld == m_pHpiFitResult->devHeadTrans.toOld());
            bDoRecomputation = m_bDoRecomputation;
            m_mutex.unlock();

            // do recomputation if requested, a large head movement occured and devHeadTrans is different
            if(bIsLargeHeadMovement && bIsDifferent && bDoRecomputation) {
                emit statusInformationChanged(2);           // recomputing
                m_mutex.lock();
                m_bBusy = true;
                transMegHeadOld = m_pHpiFitResult->devHeadTrans.toOld();
                m_mutex.unlock();

                pComputeFwd->updateHeadPos(&transMegHeadOld);
                pFwdSolution->sol = pComputeFwd->sol;
                pFwdSolution->sol_grad = pComputeFwd->sol_grad;

                m_mutex.lock();
                m_bBusy = false;
                m_mutex.unlock();
                bFwdReady = true;

                if(!bDoClustering) {
                    m_pRTFSOutput->measurementData()->setValue(pFwdSolution);
                    bFwdReady = false;
                    emit statusInformationChanged(5);       //finished
                }
            }
        }

        // do clustering if requested and fwd is ready
        m_mutex.lock();
        bDoClustering = m_bDoClustering;
        m_mutex.unlock();

        if(bDoClustering && bFwdReady) {
            emit statusInformationChanged(3);               // clustering
            pClusteredFwd = MNEForwardSolution::SPtr(new MNEForwardSolution(pFwdSolution->cluster_forward_solution(*m_pAnnotationSet.data(), m_pFwdSettings->ncluster)));
            emit clusteringAvailable(pClusteredFwd->nsource);

            m_pRTFSOutput->measurementData()->setValue(pClusteredFwd);

            bFwdReady = false;

            emit statusInformationChanged(5);               //finished
        }
    }
}

//=============================================================================================================

QString RtFwd::getBuildInfo()
{
    return QString(RTFWDPLUGIN::buildDateTime()) + QString(" - ")  + QString(RTFWDPLUGIN::buildHash());
}
