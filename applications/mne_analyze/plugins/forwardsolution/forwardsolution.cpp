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

#include <disp/viewers/fwdsettingsview.h>

#include <fwd/computeFwd/compute_fwd.h>
#include <fwd/computeFwd/compute_fwd_settings.h>

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
{
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
        default:
            qWarning() << "[ForwardSolution::handleEvent] Received an Event that is not handled by switch cases.";
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> ForwardSolution::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    //temp.push_back(SELECTED_MODEL_CHANGED);

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
    m_mutex.lock();
    m_bDoFwdComputation = true;
    m_mutex.unlock();

    m_pFwdSettings->pFiffInfo = m_pFiffInfo;

    emit statusInformationChanged(0);           // initializing
    FWDLIB::ComputeFwd::SPtr pComputeFwd = FWDLIB::ComputeFwd::SPtr(new FWDLIB::ComputeFwd(m_pFwdSettings));

    QFile t_fSolution(m_pFwdSettings->solname);
    MNELIB::MNEForwardSolution::SPtr pFwdSolution;
    MNELIB::MNEForwardSolution::SPtr pClusteredFwd;

    emit statusInformationChanged(4);           // not computed

    emit statusInformationChanged(1);   // computing
    m_mutex.lock();
    m_bBusy = true;
    m_mutex.unlock();

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
}

//=============================================================================================================

void ForwardSolution::onRecompStatusChanged(bool bDoRecomputation)
{
    m_mutex.lock();
    m_bDoRecomputation = bDoRecomputation;
    m_mutex.unlock();
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
    m_mutex.lock();
    m_bDoClustering = bDoClustering;
    m_mutex.unlock();
}

//=============================================================================================================

void ForwardSolution::onAtlasDirChanged(const QString& sDirPath, const FSLIB::AnnotationSet::SPtr pAnnotationSet)
{
    m_mutex.lock();
    m_sAtlasDir = sDirPath;
    m_pAnnotationSet = pAnnotationSet;
    m_mutex.unlock();
}
