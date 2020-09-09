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
    m_digFidHead = FiffDigPointSet(fileDig);

    QVariant data = QVariant::fromValue(m_digFidHead);
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
    bool bScale = true;

    // Initial Fiducial Alignment
    // Declare variables
    FiffDigPointSet digSetFidHead = m_digFidHead.pickTypes({FIFFV_POINT_CARDINAL});
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

        // set standart weights
        if(digSetFidHead[i].ident == FIFFV_POINT_NASION) {
            vecWeights(i) = 10.0;
        } else {
            vecWeights(i) = 1.0;
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
    return;
}

