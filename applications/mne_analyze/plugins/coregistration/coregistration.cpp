//=============================================================================================================
/**
 * @file     coregistration.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the CoRegistration class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coregistration.h"

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <anShared/Utils/metatypes.h>
#include <anShared/Model/bemdatamodel.h>

#include "disp/viewers/coregsettingsview.h"
#include "fiff/fiff_dig_point_set.h"
#include "mne/mne_bem.h"
#include "mne/mne_project_to_surface.h"
#include "rtprocessing/icp.h"

#include <Eigen/Core>
#include <stdio.h>
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COREGISTRATIONPLUGIN;
using namespace ANSHAREDLIB;
using namespace MNELIB;
using namespace DISPLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CoRegistration::CoRegistration()
    : m_pCoregSettingsView(Q_NULLPTR)
    , m_pBem(Q_NULLPTR)
{
}

//=============================================================================================================

CoRegistration::~CoRegistration()
{
}

//=============================================================================================================

QSharedPointer<IPlugin> CoRegistration::clone() const
{
    QSharedPointer<CoRegistration> pCoRegistrationClone = QSharedPointer<CoRegistration>::create();
    return pCoRegistrationClone;
}

//=============================================================================================================

void CoRegistration::init()
{
    m_pCommu = new Communicator(this);   
}

//=============================================================================================================

void CoRegistration::unload()
{
}

//=============================================================================================================

QString CoRegistration::getName() const
{
    return "Co-Registration";
}

//=============================================================================================================

QMenu *CoRegistration::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *CoRegistration::getControl()
{
    // Coregistration Settings
    m_pCoregSettingsView = new CoregSettingsView(QString("MNEANALYZE/%1").arg(this->getName()));

    QDockWidget* pControlDock = new QDockWidget(getName());
    pControlDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    pControlDock->setWidget(m_pCoregSettingsView);
    pControlDock->setObjectName(getName());

    // connect buttons
    connect(m_pCoregSettingsView, &CoregSettingsView::changeSelectedBem,
            this, &CoRegistration::onChangeSelectedBem, Qt::UniqueConnection);
    connect(m_pCoregSettingsView, &CoregSettingsView::digFileChanged,
            this, &CoRegistration::onDigitizersChanged);
    connect(m_pCoregSettingsView, &CoregSettingsView::fidFileChanged,
            this, &CoRegistration::onFiducialsChanged);
    connect(m_pCoregSettingsView, &CoregSettingsView::fitFiducials,
            this, &CoRegistration::onFitFiducials);
    connect(m_pCoregSettingsView, &CoregSettingsView::fitICP,
            this, &CoRegistration::onFitICP);

    onChangeSelectedBem(m_pCoregSettingsView->getCurrentSelectedBem());

    return pControlDock;
}

//=============================================================================================================

QWidget *CoRegistration::getView()
{
    //If the plugin does not have a view:
    return Q_NULLPTR;
}

//=============================================================================================================

void CoRegistration::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case SELECTED_MODEL_CHANGED:
            updateBemList(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel>>());
            break;
        default:
            qWarning() << "[CoRegistration::handleEvent] received an Event that is not handled by switch-cases";
            break;
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> CoRegistration::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);
    return temp;
}

//=============================================================================================================

void CoRegistration::updateBemList(ANSHAREDLIB::AbstractModel::SPtr pNewModel)
{
    if(pNewModel->getType() == ANSHAREDLIB_BEMDATA_MODEL) {
        m_pCoregSettingsView->clearSelectionBem();
        m_vecBemModels.append(pNewModel);

        for(int i = 0; i < m_vecBemModels.size(); i++){
            m_pCoregSettingsView->addSelectionBem(m_vecBemModels.at(i)->getModelName());
        }
    }
}

//=============================================================================================================

void CoRegistration::onChangeSelectedBem(const QString &sText)
{
    QVectorIterator<QSharedPointer<ANSHAREDLIB::AbstractModel>> i(m_vecBemModels);
    QSharedPointer<ANSHAREDLIB::BemDataModel> pBemDataModel;
    while (i.hasNext()) {
        if(i.peekNext()->getModelName() == sText) {
            pBemDataModel = qSharedPointerCast<BemDataModel>(i.next());
            m_pBem = QSharedPointer<MNEBem>(pBemDataModel->getBem());

            QVariant data = QVariant::fromValue(pBemDataModel);
            m_pCommu->publishEvent(EVENT_TYPE::SELECTED_BEM_CHANGED, data);
            return;
        }
    }
}

//=============================================================================================================

void CoRegistration::onDigitizersChanged(const QString& sFilePath)
{
    QFile fileDig(sFilePath);
    m_digSetHead = FiffDigPointSet(fileDig);

    QVariant data = QVariant::fromValue(m_digSetHead);
    m_pCommu->publishEvent(EVENT_TYPE::NEW_DIGITIZER_ADDED, data);
    return;
}

//=============================================================================================================

void CoRegistration::onFiducialsChanged(const QString& sFilePath)
{
    QFile fileDig(sFilePath);
    m_digFidMri = FiffDigPointSet(fileDig);

    QVariant data = QVariant::fromValue(m_digFidMri);
    m_pCommu->publishEvent(EVENT_TYPE::NEW_FIDUCIALS_ADDED, data);
    return;
}

//=============================================================================================================

void CoRegistration::onFitFiducials()
{
    // get values from view
    bool bScale = m_pCoregSettingsView->getAutoScale();
    float fWeightLPA = m_pCoregSettingsView->getWeightLPA();
    float fWeightNAS = m_pCoregSettingsView->getWeightNAS();
    float fWeightRPA = m_pCoregSettingsView->getWeightRPA();

    // Declare variables
    FiffDigPointSet digSetFidHead = m_digSetHead.pickTypes({FIFFV_POINT_CARDINAL});
    FiffDigPointSet digSetFidMRI = m_digFidMri.pickTypes({FIFFV_POINT_CARDINAL});

    Matrix3f matHead(digSetFidHead.size(),3);
    Matrix3f matMri(digSetFidMRI.size(),3);
    Matrix4f matTrans;
    Vector3f vecWeights; // LPA, Nasion, RPA
    float fScale;

    // get coordinates
    for(int i = 0; i< digSetFidHead.size(); ++i) {
        matHead(i,0) = digSetFidHead[i].r[0]; matHead(i,1) = digSetFidHead[i].r[1]; matHead(i,2) = digSetFidHead[i].r[2];
        matMri(i,0) = digSetFidMRI[i].r[0]; matMri(i,1) = digSetFidMRI[i].r[1]; matMri(i,2) = digSetFidMRI[i].r[2];

        // set weights
        switch (digSetFidHead[i].ident) {
        case FIFFV_POINT_NASION:
            vecWeights(i) = fWeightNAS;
            break;
        case FIFFV_POINT_LPA:
            vecWeights(i) = fWeightLPA;
            break;
        case FIFFV_POINT_RPA:
            vecWeights(i) = fWeightRPA;
            break;
        }
    }

    // align fiducials
    if(!RTPROCESSINGLIB::fitMatchedPoints(matHead,matMri,matTrans,fScale,bScale,vecWeights)) {
        qWarning() << "Point cloud registration not succesfull.";
    }

    // make transform
    fiff_int_t iFrom = FIFFV_COORD_HEAD;
    fiff_int_t iTo = FIFFV_COORD_MRI;
    m_transHeadMri = FiffCoordTrans::make(iFrom, iTo, matTrans);

    // send event
    QVariant data = QVariant::fromValue(m_transHeadMri);
    m_pCommu->publishEvent(EVENT_TYPE::NEW_TRANS_AVAILABE, data);

    return;
}

//=============================================================================================================

void CoRegistration::onFitICP()
{
    // get values from view
    float fWeightLPA = m_pCoregSettingsView->getWeightLPA();
    float fWeightNAS = m_pCoregSettingsView->getWeightNAS();
    float fWeightRPA = m_pCoregSettingsView->getWeightRPA();
    float fMaxDist = m_pCoregSettingsView->getOmmitDistance();
    float fTol = m_pCoregSettingsView->getConvergence();
    int iMaxIter = m_pCoregSettingsView->getMaxIter();

    // init surface points
    MNEBem bemHead = *m_pBem.data();
    MNEBemSurface::SPtr bemSurface = MNEBemSurface::SPtr::create(bemHead[0]);
    MNEProjectToSurface::SPtr mneSurfacePoints = MNEProjectToSurface::SPtr::create(*bemSurface);

    // init point cloud
    QList<int> lPickHSP({FIFFV_POINT_CARDINAL,FIFFV_POINT_HPI,FIFFV_POINT_EXTRA,FIFFV_POINT_EEG});
    FiffDigPointSet digSetHSP = m_digSetHead.pickTypes(lPickHSP);

    VectorXf vecWeightsICP(digSetHSP.size()); // Weigths vector
    MatrixXf matHsp(digSetHSP.size(),3);

    for(int i = 0; i < digSetHSP.size(); ++i) {
        matHsp(i,0) = digSetHSP[i].r[0]; matHsp(i,1) = digSetHSP[i].r[1]; matHsp(i,2) = digSetHSP[i].r[2];
        // set standart weights
        if(digSetHSP[i].kind == FIFFV_POINT_CARDINAL) {
            // set weights
            switch (digSetHSP[i].ident) {
            case FIFFV_POINT_NASION:
                vecWeightsICP(i) = fWeightNAS;
                break;
            case FIFFV_POINT_LPA:
                vecWeightsICP(i) = fWeightLPA;
                break;
            case FIFFV_POINT_RPA:
                vecWeightsICP(i) = fWeightRPA;
                break;
            }
        }
    }

    MatrixXf matHspClean;
    VectorXi vecTake;
    // discard outliers
    if(!RTPROCESSINGLIB::discard3DPointOutliers(mneSurfacePoints, matHsp, m_transHeadMri, vecTake, matHspClean, fMaxDist)) {
        qWarning() << "Discard outliers was not succesfull.";
    }
    int iNDiscarded = vecWeightsICP.size() - vecTake.size();
    m_pCoregSettingsView->setOmittedPoints(iNDiscarded);

    VectorXf vecWeightsICPClean(vecTake.size());

    for(int i = 0; i < vecTake.size(); ++i) {
        vecWeightsICPClean(i) = vecWeightsICP(vecTake(i));
    }

    // icp
    if(!RTPROCESSINGLIB::performIcp(mneSurfacePoints, matHspClean, m_transHeadMri, iMaxIter, fTol, vecWeightsICPClean)) {
        qWarning() << "ICP was not succesfull.";
    }

    // send event
    QVariant data = QVariant::fromValue(m_transHeadMri);
    m_pCommu->publishEvent(EVENT_TYPE::NEW_TRANS_AVAILABE, data);

    return;
}

