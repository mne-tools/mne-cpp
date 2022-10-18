//=============================================================================================================
/**
 * @file     forwardsolution.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>;
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     June, 2022
 *
 * @section  LICENSE
 *
 * Copyright (C) 2022, Gabriel B Motta, Juan G Prieto. All rights reserved.
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
 * @brief    ForwardSolution class defintion.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "forwardsolution.h"

#include <anShared/Management/communicator.h>
#include <anShared/Management/analyzedata.h>

#include <anShared/Model/abstractmodel.h>
#include <anShared/Model/forwardsolutionmodel.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/averagingdatamodel.h>

#include <disp/viewers/fwdsettingsview.h>

#include <fiff/fiff_stream.h>

#include <fwd/computeFwd/compute_fwd.h>
#include <fwd/computeFwd/compute_fwd_settings.h>

#include <fs/annotationset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QDockWidget>
#include <QVBoxLayout>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FORWARDSOLUTIONPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ForwardSolution::ForwardSolution()
: m_pCommu(Q_NULLPTR)
, m_pFwdSettings(new FWDLIB::ComputeFwdSettings)
, m_bBusy(false)
, m_bDoRecomputation(false)
, m_bDoClustering(true)
, m_bDoFwdComputation(false)
{
    // set init values
    m_pFwdSettings->solname = QCoreApplication::applicationDirPath() + "/MNE-sample-data/your-solution-fwd.fif";
    m_pFwdSettings->mriname = QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/all-trans.fif";
    m_pFwdSettings->bemname = QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif";
    m_pFwdSettings->srcname = QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/bem/sample-oct-6-src.fif";
    m_pFwdSettings->measname = QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif";
    m_pFwdSettings->transname.clear();
    m_pFwdSettings->eeg_model_name = "Default";
    m_pFwdSettings->include_meg = true;
    m_pFwdSettings->include_eeg = true;
    m_pFwdSettings->accurate = true;
    m_pFwdSettings->mindist = 5.0f/1000.0f;

    m_sAtlasDir = QCoreApplication::applicationDirPath() + "/MNE-sample-data/subjects/sample/label";

    m_pAnnotationSet = FSLIB::AnnotationSet::SPtr(new FSLIB::AnnotationSet(m_sAtlasDir+"/lh.aparc.a2009s.annot", m_sAtlasDir+"/rh.aparc.a2009s.annot"));
}

//=============================================================================================================

ForwardSolution::~ForwardSolution()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> ForwardSolution::clone() const
{
    QSharedPointer<ForwardSolution> pForwardSolutionClone(new ForwardSolution);
    return pForwardSolutionClone;
}

//=============================================================================================================

void ForwardSolution::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void ForwardSolution::unload()
{

}

//=============================================================================================================

QString ForwardSolution::getName() const
{
    return "Forward Solution";
}

//=============================================================================================================

QMenu *ForwardSolution::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QWidget *ForwardSolution::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget* ForwardSolution::getControl()
{
    m_pFwdSettingsView = new DISPLIB::FwdSettingsView();

    QVBoxLayout* pControlLayout = new QVBoxLayout();
    pControlLayout->addWidget(m_pFwdSettingsView);

    QScrollArea* pControlScrollArea = new QScrollArea();
    pControlScrollArea->setLayout(pControlLayout);

    QDockWidget* pControlDock = new QDockWidget(this->getName());
    pControlDock->setWidget(pControlScrollArea);

    // connect incoming signals
    connect(m_pFwdSettingsView, &DISPLIB::FwdSettingsView::recompStatusChanged,
            this, &ForwardSolution::onRecompStatusChanged);
    connect(m_pFwdSettingsView, &DISPLIB::FwdSettingsView::clusteringStatusChanged,
            this, &ForwardSolution::onClusteringStatusChanged);
    connect(m_pFwdSettingsView, &DISPLIB::FwdSettingsView::atlasDirChanged,
            this, &ForwardSolution::onAtlasDirChanged);
    connect(m_pFwdSettingsView, &DISPLIB::FwdSettingsView::doForwardComputation,
            this, &ForwardSolution::onDoForwardComputation);

    // connect outgoing signals
    connect(this, &ForwardSolution::statusInformationChanged,
            m_pFwdSettingsView, &DISPLIB::FwdSettingsView::setRecomputationStatus, Qt::BlockingQueuedConnection);
    connect(this, &ForwardSolution::fwdSolutionAvailable,
            m_pFwdSettingsView, &DISPLIB::FwdSettingsView::setSolutionInformation, Qt::BlockingQueuedConnection);
    connect(this, &ForwardSolution::clusteringAvailable,
            m_pFwdSettingsView, &DISPLIB::FwdSettingsView::setClusteredInformation, Qt::BlockingQueuedConnection);


    return pControlDock;
}

//=============================================================================================================

void ForwardSolution::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case EVENT_TYPE::SELECTED_MODEL_CHANGED:
            onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
            break;
        default:
            qWarning() << "[ForwardSolution::handleEvent] Received an Event that is not handled by switch cases.";
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> ForwardSolution::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);

    return temp;
}

//=============================================================================================================

QString ForwardSolution::getBuildInfo()
{
    return QString(FORWARDSOLUTIONPLUGIN::buildDateTime()) + QString(" - ")  + QString(FORWARDSOLUTIONPLUGIN::buildHash());
}

//=============================================================================================================

void ForwardSolution::onDoForwardComputation()
{
    if(!m_pFiffInfo){
        qInfo() << "No FiffInfo source available for forward solution computation.";
        return;
    }

    qInfo() << "Performing forward solution computation.";

    QFile t_fBem(m_pFwdSettings->bemname);
    FIFFLIB::FiffStream::SPtr stream(new FIFFLIB::FiffStream(&t_fBem));
    if(!stream->open()) {
        QMessageBox msgBox;
        msgBox.setText("The bem model cannot be opend. Chosse another file.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
        msgBox.exec();
        stream->close();
        return;
    }
    stream->close();

    // Read source space
    QFile t_fSource(m_pFwdSettings->srcname);
    stream = FIFFLIB::FiffStream::SPtr(new FIFFLIB::FiffStream(&t_fSource));
    if(!stream->open()) {
        QMessageBox msgBox;
        msgBox.setText("The source space cannot be opend. Chosse another file.");
        msgBox.setText(m_pFwdSettings->srcname);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
        msgBox.exec();
        stream->close();
        return;
    }
    stream->close();

    // Read MRI transformation
    QFile t_fMri(m_pFwdSettings->mriname);
    stream = FIFFLIB::FiffStream::SPtr(new FIFFLIB::FiffStream(&t_fMri));
    if(!stream->open()) {
        QMessageBox msgBox;
        msgBox.setText("The mri - head transformation cannot be opend. Chosse another file.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
        msgBox.exec();
        stream->close();
        return;
    }
    stream->close();

    // Read measurement
    QFile t_fMeas(m_pFwdSettings->measname);
    stream = FIFFLIB::FiffStream::SPtr(new FIFFLIB::FiffStream(&t_fMri));
    if(!stream->open()) {
        QMessageBox msgBox;
        msgBox.setText("The meaurement file cannot be opend. Chosse another file.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
        msgBox.exec();
        stream->close();
        return;
    }
    stream->close();

    qInfo() << "Verified source files for forward solution computation.";

    m_bDoFwdComputation = true;

    m_pFwdSettings->pFiffInfo = m_pFiffInfo;

    emit statusInformationChanged(0);           // initializing
    FWDLIB::ComputeFwd::SPtr pComputeFwd = FWDLIB::ComputeFwd::SPtr(new FWDLIB::ComputeFwd(m_pFwdSettings));

    QFile t_fSolution(m_pFwdSettings->solname);
    MNELIB::MNEForwardSolution::SPtr pFwdSolution;
    MNELIB::MNEForwardSolution::SPtr pClusteredFwd;

    emit statusInformationChanged(4);           // not computed

    emit statusInformationChanged(1);   // computing
    m_bBusy = true;

    // compute and store
    pComputeFwd->calculateFwd();
    pComputeFwd->storeFwd();

    // get Mne Forward Solution (in future this is not necessary, ComputeForward will have this as member)
    pFwdSolution = MNELIB::MNEForwardSolution::SPtr(new MNELIB::MNEForwardSolution(t_fSolution, false, true));

    // emit results to control widget
    emit fwdSolutionAvailable(pFwdSolution->source_ori,
                              pFwdSolution->coord_frame,
                              pFwdSolution->nsource,
                              pFwdSolution->nchan,
                              pFwdSolution->src.size());

    if(m_bDoClustering) {
        emit statusInformationChanged(3);               // clustering
        pClusteredFwd = MNELIB::MNEForwardSolution::SPtr(new MNELIB::MNEForwardSolution(pFwdSolution->cluster_forward_solution(*m_pAnnotationSet.data(), 200)));
        emit clusteringAvailable(pClusteredFwd->nsource);
        emit statusInformationChanged(5);               //finished
        m_pFwdSolution = pClusteredFwd;
    } else {
        emit statusInformationChanged(5);
        m_pFwdSolution = pFwdSolution;
    }

    QSharedPointer<ANSHAREDLIB::ForwardSolutionModel> pFwdSolModel = QSharedPointer<ANSHAREDLIB::ForwardSolutionModel>(new ANSHAREDLIB::ForwardSolutionModel(m_pFwdSolution));

    m_pAnalyzeData->addModel<ANSHAREDLIB::ForwardSolutionModel>(pFwdSolModel,
                                                                "Fwd. Sol. - " + QDateTime::currentDateTime().toString());

    qInfo() << "New soultion available";
}

//=============================================================================================================

void ForwardSolution::onRecompStatusChanged(bool bDoRecomputation)
{
    m_bDoRecomputation = bDoRecomputation;
}

//=============================================================================================================

void ForwardSolution::onClusteringStatusChanged(bool bDoClustering)
{
    if(m_pAnnotationSet->isEmpty()) {
        QMessageBox msgBox;
        msgBox.setText("Please load an annotation set befor clustering.");
        msgBox.exec();
        return;
    }
    m_bDoClustering = bDoClustering;
}

//=============================================================================================================

void ForwardSolution::onAtlasDirChanged(const QString& sDirPath, const FSLIB::AnnotationSet::SPtr pAnnotationSet)
{
    m_sAtlasDir = sDirPath;
    m_pAnnotationSet = pAnnotationSet;
}

//=============================================================================================================

void ForwardSolution::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    switch(pNewModel->getType()){
         case ANSHAREDLIB::ANSHAREDLIB_FIFFRAW_MODEL:{
            auto pRawDataModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);
            if(pRawDataModel){
                m_pFiffInfo = pRawDataModel->getFiffInfo();
            }
            break;
        }
        case ANSHAREDLIB::ANSHAREDLIB_AVERAGING_MODEL:{
            auto pAverageDataModel = qSharedPointerCast<AveragingDataModel>(pNewModel);
            if(pAverageDataModel){
                m_pFiffInfo = pAverageDataModel->getFiffInfo();
            }
            break;
        }
        default:
            break;
    }
}
