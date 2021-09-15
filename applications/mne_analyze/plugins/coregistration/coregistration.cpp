//=============================================================================================================
/**
 * @file     coregistration.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     August, 2020
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

#include <disp/viewers/coregsettingsview.h>
#include <fiff/fiff_dig_point_set.h>
#include <mne/mne_bem.h>
#include <mne/mne_project_to_surface.h>
#include <rtprocessing/icp.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>
#include <QDebug>
#include <QVector>
#include <QtConcurrent/QtConcurrent>

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
{
    m_vecBemDataModels = QVector<QSharedPointer<ANSHAREDLIB::AbstractModel>>();

    // init fiducial dig point set with three FiffDigPoints
    m_digFidMri = FiffDigPointSet();
    FiffDigPoint digLPA = FiffDigPoint();
    FiffDigPoint digNAS = FiffDigPoint();
    FiffDigPoint digRPA = FiffDigPoint();

    digLPA.kind = FIFFV_POINT_CARDINAL;
    digNAS.kind = FIFFV_POINT_CARDINAL;
    digRPA.kind = FIFFV_POINT_CARDINAL;
    digLPA.ident = FIFFV_POINT_LPA;
    digNAS.ident = FIFFV_POINT_NASION;
    digRPA.ident = FIFFV_POINT_RPA;
    digLPA.coord_frame = FIFFV_COORD_MRI;
    digNAS.coord_frame = FIFFV_COORD_MRI;
    digRPA.coord_frame = FIFFV_COORD_MRI;

    m_digFidMri << digLPA << digNAS << digRPA;
    m_iFiducial = 0;
}

//=============================================================================================================

CoRegistration::~CoRegistration()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> CoRegistration::clone() const
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
    connect(m_pCoregSettingsView, &CoregSettingsView::fidStoreFileChanged,
            this, &CoRegistration::onStoreFiducials);
    connect(m_pCoregSettingsView, &CoregSettingsView::transParamChanged,
            this, &CoRegistration::onUpdateTrans);
    connect(m_pCoregSettingsView, &CoregSettingsView::loadTrans,
            this, &CoRegistration::onLoadTrans);
    connect(m_pCoregSettingsView, &CoregSettingsView::storeTrans,
            this, &CoRegistration::onStoreTrans);

    // connect fiducial selection
    connect(m_pCoregSettingsView, &CoregSettingsView::pickFiducials,
            this, &CoRegistration::onPickFiducials);
    connect(m_pCoregSettingsView, &CoregSettingsView::fiducialChanged,
            this, &CoRegistration::onPickedFiducialChanged);

    // automaticly load Bem if available
    onChangeSelectedBem(m_pCoregSettingsView->getCurrentSelectedBem());

    // Thread handling
    connect(&m_FutureWatcher, &QFutureWatcher<QMap<double,QList<int>>>::finished,
            this, &CoRegistration::createNewTrans, Qt::UniqueConnection);

    return pControlDock;
}

//=============================================================================================================

QWidget *CoRegistration::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void CoRegistration::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case EVENT_TYPE::NEW_FIDUCIAL_PICKED:
            onSetFiducial(e->getData().value<QVector3D>());
            break;
        case EVENT_TYPE::SELECTED_MODEL_CHANGED:
            onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
            break;
        case EVENT_TYPE::MODEL_REMOVED:
            onModelRemoved(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel>>());
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
    temp.push_back(NEW_FIDUCIAL_PICKED);
    temp.push_back(SELECTED_MODEL_CHANGED);
    temp.push_back(MODEL_REMOVED);
    return temp;
}

//=============================================================================================================

void CoRegistration::updateBemList(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    // check first if model was already added and if the passed model is actually a BemDataModel
    if(!m_vecBemDataModels.contains(pNewModel) && pNewModel->getType() == ANSHAREDLIB_BEMDATA_MODEL) {
        m_pCoregSettingsView->clearSelectionBem();
        m_vecBemDataModels.append(pNewModel);
        for(auto& pBemDataModel : m_vecBemDataModels) {
            m_pCoregSettingsView->addSelectionBem(pBemDataModel->getModelName());
        }
    }
}

//=============================================================================================================

void CoRegistration::onChangeSelectedBem(const QString &sText)
{
    QSharedPointer<ANSHAREDLIB::BemDataModel> pBemDataModel;

    for (auto& bemDataModel : m_vecBemDataModels) {
        if(bemDataModel->getModelName() == sText && sText != m_sCurrentSelectedBem){
            // update current selected Bem
            pBemDataModel = qSharedPointerCast<BemDataModel>(bemDataModel);
            m_pBem = QSharedPointer<MNEBem>(pBemDataModel->getBem());
            m_sCurrentSelectedBem = sText;

            // send event to 3DView etc.
            QVariant data = QVariant::fromValue(pBemDataModel);
            m_pCommu->publishEvent(EVENT_TYPE::SELECTED_BEM_CHANGED, data);
            return;
        }
    }
}

//=============================================================================================================

void CoRegistration::onPickFiducials(const bool bActivatePicking)
{
    // rotate camera to recently selected fiducial in 3DView
    if(bActivatePicking) {
        onPickedFiducialChanged(m_pCoregSettingsView->getCurrentFiducial());
    }

    QVariant data = QVariant::fromValue(bActivatePicking);
    m_pCommu->publishEvent(EVENT_TYPE::FID_PICKING_STATUS, data);
}

//=============================================================================================================

void CoRegistration::onSetFiducial(const QVector3D vecResult)
{
    // Connect to 3DView and activate ObjectPicking
    fiff_float_t    r[3];           /**< Point location. */
    for(int i = 0; i < 3; i++) {
        r[i] = vecResult[i];
    }
    fiff_int_t iFiducial = m_pCoregSettingsView->getCurrentFiducial();
    switch(iFiducial) {
        case FIFFV_POINT_LPA:
            m_digFidMri[0].r[0] = r[0];
            m_digFidMri[0].r[1] = r[1];
            m_digFidMri[0].r[2] = r[2];
            m_pCoregSettingsView->setFiducials(vecResult);
            break;
        case FIFFV_POINT_NASION:
            m_digFidMri[1].r[0] = r[0];
            m_digFidMri[1].r[1] = r[1];
            m_digFidMri[1].r[2] = r[2];
            m_pCoregSettingsView->setFiducials(vecResult);
            break;
        case FIFFV_POINT_RPA:
            m_digFidMri[2].r[0] = r[0];
            m_digFidMri[2].r[1] = r[1];
            m_digFidMri[2].r[2] = r[2];
            m_pCoregSettingsView->setFiducials(vecResult);
            break;
    }

    QVariant data = QVariant::fromValue(m_digFidMri);
    m_pCommu->publishEvent(EVENT_TYPE::NEW_FIDUCIALS_ADDED, data);

    return;
}

//=============================================================================================================

void CoRegistration::onPickedFiducialChanged(const int iFiducial)
{
    m_iFiducial = iFiducial;
    QVariant data = QVariant::fromValue(iFiducial);
    m_pCommu->publishEvent(EVENT_TYPE::FIDUCIAL_CHANGED, data);
}

//=============================================================================================================

void CoRegistration::onDigitizersChanged(const QString& sFilePath)
{
    QFile fileDig(sFilePath);
    m_digSetHead.clear();
    m_digSetHead = FiffDigPointSet(fileDig);

    // send digitizer to 3DView
    QVariant data = QVariant::fromValue(m_digSetHead);
    m_pCommu->publishEvent(EVENT_TYPE::NEW_DIGITIZER_ADDED, data);

    // set transformation if not empty
    if(!m_transHeadMri.isEmpty()) {
        QVariant data = QVariant::fromValue(m_transHeadMri);
        m_pCommu->publishEvent(EVENT_TYPE::NEW_TRANS_AVAILABE, data);
    }

    return;
}

//=============================================================================================================

void CoRegistration::onFiducialsChanged(const QString& sFilePath)
{
    QFile fileDig(sFilePath);
    m_digFidMri.clear();
    m_digFidMri = FiffDigPointSet(fileDig);

    QVariant data = QVariant::fromValue(m_digFidMri);
    m_pCommu->publishEvent(EVENT_TYPE::NEW_FIDUCIALS_ADDED, data);
    return;
}

//=============================================================================================================

void CoRegistration::onStoreFiducials(const QString& sFilePath)
{
    QFile fileDig(sFilePath);
    m_digFidMri.write(fileDig);
    return;
}

//=============================================================================================================

void CoRegistration::onLoadTrans(const QString& sFilePath)
{
    QFile fileTrans(sFilePath);
    FiffCoordTrans transTemp = FiffCoordTrans(fileTrans);

    // Transformation parameters
    Vector3f vecRot;
    Vector3f vecScale;
    Vector3f vecTrans;

    // check for type of transformation
    if(transTemp.from == FIFFV_COORD_HEAD && transTemp.to == FIFFV_COORD_MRI) {
        m_transHeadMri.clear();
        m_transHeadMri = FiffCoordTrans(transTemp);

        // Update Widget
        getParamFromTrans(m_transHeadMri.trans,vecRot,vecTrans,vecScale);
        m_pCoregSettingsView->setTransParams(vecTrans,vecRot,vecScale);

        // Send event
        QVariant data = QVariant::fromValue(m_transHeadMri);
        m_pCommu->publishEvent(EVENT_TYPE::NEW_TRANS_AVAILABE, data);

    } else if (transTemp.from == FIFFV_COORD_MRI && transTemp.to == FIFFV_COORD_HEAD) {
        transTemp.invert_transform();
        m_transHeadMri = FiffCoordTrans(transTemp);

        // Update Widget
        getParamFromTrans(m_transHeadMri.trans,vecRot,vecTrans,vecScale);
        m_pCoregSettingsView->setTransParams(vecTrans,vecRot,vecScale);

        // send event
        QVariant data = QVariant::fromValue(m_transHeadMri);
        m_pCommu->publishEvent(EVENT_TYPE::NEW_TRANS_AVAILABE, data);
    } else {
        qDebug() << "[CoRegistration::onLoadTrans] Loaded Transformation from:" << FiffCoordTrans::frame_name(transTemp.from) << "to:" << FiffCoordTrans::frame_name(transTemp.to) << "is not suitable for co-registration.";
    }
    return;
}

//=============================================================================================================

void CoRegistration::onStoreTrans(const QString& sFilePath)
{
    QFile fileTrans(sFilePath);

    if(!m_transHeadMri.isEmpty()){
        m_transHeadMri.write(fileTrans);
    }
    return;
}

//=============================================================================================================

void CoRegistration::onFitFiducials()
{
    if(m_digSetHead.isEmpty() || m_digFidMri.isEmpty() || m_pBem->isEmpty()) {
        qWarning() << "[CoRegistration::onFitFiducials] Make sure to load all the necessary data.";
        return;
    }

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
    float fScale = 0.0;

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

    // update GUI
    Vector3f vecRot;
    Vector3f vecScale;
    Vector3f vecTrans;
    getParamFromTrans(m_transHeadMri.trans,vecRot,vecTrans,vecScale);
    m_pCoregSettingsView->setTransParams(vecTrans,vecRot,vecScale);

    // send event
    QVariant data = QVariant::fromValue(m_transHeadMri);
    m_pCommu->publishEvent(EVENT_TYPE::NEW_TRANS_AVAILABE, data);

    return;
}

//=============================================================================================================

void CoRegistration::triggerLoadingStart(QString sMessage)
{
    m_pCommu->publishEvent(LOADING_START, QVariant::fromValue(sMessage));
}

//=============================================================================================================

void CoRegistration::triggerLoadingEnd(QString sMessage)
{
    m_pCommu->publishEvent(LOADING_END, QVariant::fromValue(sMessage));
}

//=============================================================================================================

void CoRegistration::onFitICP()
{
    if(m_digSetHead.isEmpty() || m_digFidMri.isEmpty() || m_pBem->isEmpty()) {
        qWarning() << "[CoRegistration::onFitICP] Make sure to load all the necessary data.";
        return;
    }

    if (m_FutureWatcher.isRunning()){
        qWarning() << "ICP computation already taking place.";
        return;
    }

    // get icp settings from view
    setIcpProperties();

    triggerLoadingStart("Performing ICP ...");

    // start icp
    m_Future = QtConcurrent::run(this,
                                 &CoRegistration::computeICP,
                                 m_transHeadMri,
                                 m_digSetHead,
                                 *m_pBem.data());

    m_FutureWatcher.setFuture(m_Future);

    return;
}

//=============================================================================================================

void CoRegistration::setIcpProperties()
{
    // get values from view
    m_bScale = m_pCoregSettingsView->getAutoScale();
    m_fWeightLPA = m_pCoregSettingsView->getWeightLPA();
    m_fWeightNAS = m_pCoregSettingsView->getWeightNAS();
    m_fWeightRPA = m_pCoregSettingsView->getWeightRPA();
    m_fWeightHPI = m_pCoregSettingsView->getWeightHPI();
    m_fWeightHSP = m_pCoregSettingsView->getWeightHSP();
    m_fWeightEEG = m_pCoregSettingsView->getWeightEEG();
    m_fMaxDist = m_pCoregSettingsView->getOmmitDistance();
    m_fTol = m_pCoregSettingsView->getConvergence();
    m_iMaxIter = m_pCoregSettingsView->getMaxIter();
}

//=============================================================================================================

FiffCoordTrans CoRegistration::computeICP(FiffCoordTrans transInit,
                                          FiffDigPointSet digSetHead,
                                          MNEBem bemHead)
{
    // get values from members
    m_ParameterMutex.lock();
    bool bScale = m_bScale;
    float fWeightLPA = m_fWeightLPA;
    float fWeightNAS = m_fWeightNAS;
    float fWeightRPA = m_fWeightRPA;
    float fWeightHPI = m_fWeightHPI;
    float fWeightHSP = m_fWeightHSP;
    float fWeightEEG = m_fWeightEEG;
    float fMaxDist = m_fMaxDist;
    float fTol = m_fTol;
    int iMaxIter = m_iMaxIter;
    m_ParameterMutex.unlock();

    float fRMSE = 0.0;

    // init surface points
    MNEBemSurface::SPtr bemSurface = MNEBemSurface::SPtr::create(bemHead[0]);
    MNEProjectToSurface::SPtr mneSurfacePoints = MNEProjectToSurface::SPtr::create(*bemSurface);

    // get selected digitizers
    QList<int> lPickHSP = m_pCoregSettingsView->getDigitizerCheckState();
    FiffDigPointSet digSetHSP = digSetHead.pickTypes(lPickHSP);

    VectorXf vecWeightsICP(digSetHSP.size()); // Weigths vector
    MatrixXf matHsp(digSetHSP.size(),3);

    for(int i = 0; i < digSetHSP.size(); ++i) {
        matHsp(i,0) = digSetHSP[i].r[0]; matHsp(i,1) = digSetHSP[i].r[1]; matHsp(i,2) = digSetHSP[i].r[2];
        // set weights
        switch (digSetHSP[i].kind){
        case FIFFV_POINT_CARDINAL:
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
            break;
        case FIFFV_POINT_EEG:
            vecWeightsICP(i) = fWeightEEG;
            break;
        case FIFFV_POINT_HPI:
            vecWeightsICP(i) = fWeightHPI;
            break;
        case FIFFV_POINT_EXTRA:
            vecWeightsICP(i) = fWeightHSP;
            break;
        }
    }

    // discard outliers
    MatrixXf matHspClean;
    VectorXi vecTake;

    if(!RTPROCESSINGLIB::discard3DPointOutliers(mneSurfacePoints,
                                                matHsp,
                                                transInit,
                                                vecTake,
                                                matHspClean,
                                                fMaxDist)) {
        qWarning() << "Discard outliers was not succesfull.";
    }
    int iNDiscarded = vecWeightsICP.size() - vecTake.size();
    m_pCoregSettingsView->setOmittedPoints(iNDiscarded);

    VectorXf vecWeightsICPClean(vecTake.size());
    for(int i = 0; i < vecTake.size(); ++i) {
        vecWeightsICPClean(i) = vecWeightsICP(vecTake(i));
    }

    // icp
    RTPROCESSINGLIB::performIcp(mneSurfacePoints,
                                matHspClean,
                                transInit,
                                fRMSE,
                                bScale,
                                iMaxIter,
                                fTol,
                                vecWeightsICPClean);

    FiffCoordTrans transHeadMri = transInit;

    // update GUI
    Vector3f vecRot;
    Vector3f vecScale;
    Vector3f vecTrans;
    getParamFromTrans(transHeadMri.trans,vecRot,vecTrans,vecScale);
    m_pCoregSettingsView->setTransParams(vecTrans,vecRot,vecScale);
    m_pCoregSettingsView->setRMSE(fRMSE);

    return transHeadMri;
}

//=============================================================================================================

void CoRegistration::createNewTrans()
{
    m_transHeadMri = m_Future.result();

    // update GUI
    Vector3f vecRot;
    Vector3f vecScale;
    Vector3f vecTrans;
    getParamFromTrans(m_transHeadMri.trans,vecRot,vecTrans,vecScale);
    m_pCoregSettingsView->setTransParams(vecTrans,vecRot,vecScale);

    // send event
    QVariant data = QVariant::fromValue(m_transHeadMri);
    m_pCommu->publishEvent(EVENT_TYPE::NEW_TRANS_AVAILABE, data);

    triggerLoadingEnd("Performing ICP ...");
}

//=============================================================================================================

void CoRegistration::onUpdateTrans()
{
    Vector3f vecRot;
    Vector3f vecScale;
    Vector3f vecTrans;
    Matrix4f matTrans;

    // update transformation
    m_pCoregSettingsView->getTransParams(vecRot,vecTrans,vecScale);
    getTransFromParam(matTrans,vecRot,vecTrans,vecScale);

    // send event
    m_transHeadMri.trans = matTrans;
    QVariant data = QVariant::fromValue(m_transHeadMri);
    m_pCommu->publishEvent(EVENT_TYPE::NEW_TRANS_AVAILABE, data);

    return;
}

//=============================================================================================================

void CoRegistration::getParamFromTrans(const Matrix4f& matTrans,
                                       Vector3f& vecRot,
                                       Vector3f& vecTrans,
                                       Vector3f& vecScale)
{
    Matrix3f matRot = matTrans.block(0,0,3,3);

    // get scaling factor and normalize rotation
    for(int i = 0; i < 3; i++) {
        vecScale(i) = matRot.col(i).norm();
        matRot.col(i) = matRot.col(i) / vecScale(i);
    }
    // get rotation in rad - z,y,x
    vecRot = matRot.eulerAngles(2,1,0);

    // get translation vector
    vecTrans = m_transHeadMri.trans.block(0,3,3,1);
}

//=============================================================================================================

void CoRegistration::getTransFromParam(Matrix4f& matTrans,
                                       const Vector3f& vecRot,
                                       const Vector3f& vecTrans,
                                       const Vector3f& vecScale)
{
    // init trans
    matTrans.fill(0);

    // get rotation matrix
    // ZYX euler angle rotation is equivalent to XYZ fixed axis rotation
    Eigen::AngleAxisf zAngle(vecRot(0), Eigen::Vector3f::UnitZ());
    Eigen::AngleAxisf yAngle(vecRot(1), Eigen::Vector3f::UnitY());
    Eigen::AngleAxisf xAngle(vecRot(2), Eigen::Vector3f::UnitX());

    Eigen::Quaternion<float> quat = zAngle * yAngle * xAngle;
    quat.normalize();
    matTrans.block(0,0,3,3) = quat.matrix();

    // apply translation part
    matTrans.block(0,3,3,1) = vecTrans;

    // apply scaling
    Matrix4f matScale = Matrix4f::Identity();
    matScale(0,0) = vecScale(0);
    matScale(1,1) = vecScale(1);
    matScale(2,2) = vecScale(2);
    matTrans = matTrans * matScale;

    return;
}

//=============================================================================================================

void CoRegistration::deleteModels()
{
    QVector<QSharedPointer<AbstractModel>> vecModels = m_pAnalyzeData->getModelsByType(ANSHAREDLIB_BEMDATA_MODEL);
    m_pCoregSettingsView->clearSelectionBem();

    if(m_vecBemDataModels != vecModels) {
        m_vecBemDataModels = vecModels;
        if (m_vecBemDataModels.isEmpty()) {
            // empty bem and string
            m_pCoregSettingsView->addSelectionBem("Select Bem");
            m_pBem->clear();
            m_sCurrentSelectedBem = "";
        } else {
            // update new bem list
            for(auto& pBemDataModel : m_vecBemDataModels) {
                m_pCoregSettingsView->addSelectionBem(pBemDataModel->getModelName());
            }
        }
    }
}

//=============================================================================================================

void CoRegistration::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_BEMDATA_MODEL){
        updateBemList(pNewModel);
    }
}

//=============================================================================================================

void CoRegistration::onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel)
{
    if(pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_BEMDATA_MODEL){
        removeFromBemList(pRemovedModel);
    }
}

//=============================================================================================================

bool CoRegistration::removeFromBemList(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    while(m_vecBemDataModels.contains(pNewModel)){
        int iIndex = m_vecBemDataModels.indexOf(pNewModel);
        m_vecBemDataModels.remove(iIndex);
        if(m_sCurrentSelectedBem == QFileInfo(pNewModel->getModelPath()).fileName()){
            m_sCurrentSelectedBem = "";
        }

        m_pCoregSettingsView->clearSelectionBem();
        for(auto& pBemDataModel : m_vecBemDataModels) {
            m_pCoregSettingsView->addSelectionBem(pBemDataModel->getModelName());
        }
        return true;
    }
    return false;
}


//=============================================================================================================

QString CoRegistration::getBuildDateTime()
{
    return QString(BUILDINFO::dateTime());
}
