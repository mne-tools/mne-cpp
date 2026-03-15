//=============================================================================================================
/**
 * @file     dipolefit.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.7
 * @date     October, 2020
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
 * @brief    Definition of the InvDipoleFit class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipolefit.h"

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>

#include <anShared/Utils/metatypes.h>

#include <anShared/Model/abstractmodel.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/bemdatamodel.h>
#include <anShared/Model/mricoordmodel.h>
#include <anShared/Model/covariancemodel.h>
#include <anShared/Model/averagingdatamodel.h>
#include <anShared/Model/dipolefitmodel.h>

#include <disp/viewers/dipolefitview.h>

#include <inverse/dipole_fit/inv_dipole_fit_data.h>
#include <inverse/dipole_fit/inv_dipole_fit.h>
#include <inverse/dipole_fit/inv_ecd_set.h>

#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtConcurrent/QtConcurrent>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DIPOLEFITPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvDipoleFit::InvDipoleFit()
{
    m_DipoleSettings.dipname = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/dip-5120-bem_fit.dat";
}

//=============================================================================================================

InvDipoleFit::~InvDipoleFit()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> InvDipoleFit::clone() const
{
    QSharedPointer<InvDipoleFit> pDipoleFitClone = QSharedPointer<InvDipoleFit>::create();
    return pDipoleFitClone;
}

//=============================================================================================================

void InvDipoleFit::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void InvDipoleFit::unload()
{
}

//=============================================================================================================

QString InvDipoleFit::getName() const
{
    return "InvDipole Fit";
}

//=============================================================================================================

QMenu *InvDipoleFit::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *InvDipoleFit::getControl()
{
    DISPLIB::DipoleFitView* pDipoleView = new DISPLIB::DipoleFitView();

    //Send Gui updates
    connect(this, &InvDipoleFit::newBemModel,
            pDipoleView, &DISPLIB::DipoleFitView::addBem, Qt::UniqueConnection);
    connect(this, &InvDipoleFit::newCovarianceModel,
            pDipoleView, &DISPLIB::DipoleFitView::addNoise, Qt::UniqueConnection);
    connect(this, &InvDipoleFit::newMriModel,
            pDipoleView, &DISPLIB::DipoleFitView::addMri, Qt::UniqueConnection);
    connect(this, &InvDipoleFit::newMeasurment,
            pDipoleView, &DISPLIB::DipoleFitView::addMeas, Qt::UniqueConnection);
    connect(this, &InvDipoleFit::getUpdate,
            pDipoleView, &DISPLIB::DipoleFitView::requestParams, Qt::UniqueConnection);
    connect(this, &InvDipoleFit::removeModel,
            pDipoleView, &DISPLIB::DipoleFitView::removeModel, Qt::UniqueConnection);

    //Receive Gui updates
    connect(pDipoleView, &DISPLIB::DipoleFitView::modalityChanged,
            this, &InvDipoleFit::onModalityChanged, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::timeChanged,
            this, &InvDipoleFit::onTimeChanged, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::fittingChanged,
            this, &InvDipoleFit::onFittingChanged, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::baselineChanged,
            this, &InvDipoleFit::onBaselineChanged, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::noiseChanged,
            this, &InvDipoleFit::onNoiseChanged, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::regChanged,
            this, &InvDipoleFit::onRegChanged, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::setChanged,
            this, &InvDipoleFit::onSetChanged, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::sphereChanged,
            this, &InvDipoleFit::onSphereChanged, Qt::UniqueConnection);

    connect(pDipoleView, &DISPLIB::DipoleFitView::selectedBem,
            this, &InvDipoleFit::onNewBemSelected, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::selectedMri,
            this, &InvDipoleFit::onNewMriSelected, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::selectedNoise,
            this, &InvDipoleFit::onNewNoiseSelected, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::selectedMeas,
            this, &InvDipoleFit::onNewMeasSelected, Qt::UniqueConnection);

    //Fit
    connect(pDipoleView, &DISPLIB::DipoleFitView::performDipoleFit,
            this, &InvDipoleFit::onPerformDipoleFit, Qt::UniqueConnection);
    connect(&m_FutureWatcher, &QFutureWatcher<INVLIB::InvEcdSet>::finished,
            this, &InvDipoleFit::dipoleFitResults, Qt::UniqueConnection);

    QDockWidget* pDockWidgt = new QDockWidget(getName());
    pDockWidgt->setWidget(pDipoleView);
    pDockWidgt->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pDockWidgt->setObjectName(getName());

    emit getUpdate();

    return pDockWidgt;
}

//=============================================================================================================

QWidget *InvDipoleFit::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void InvDipoleFit::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    case EVENT_TYPE::SELECTED_MODEL_CHANGED:
        onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
        break;
    case EVENT_TYPE::MODEL_REMOVED:
        onModelRemoved(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel>>());
        break;
    default:
        qWarning() << "[InvDipoleFit::handleEvent] received an Event that is not handled by switch-cases";
        break;
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> InvDipoleFit::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);
    temp.push_back(MODEL_REMOVED);
    return temp;
}

//=============================================================================================================

void InvDipoleFit::onPerformDipoleFit(const QString& sFitName)
{
    m_sFitName = sFitName;

    triggerLoadingStart("Performing InvDipole Fit...");

    m_Future = QtConcurrent::run([this] {
        return this->dipoleFitCalculation();
    });

    m_FutureWatcher.setFuture(m_Future);
}

//=============================================================================================================

void InvDipoleFit::onModalityChanged(bool bEEG, bool bMEG)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.include_meg = bMEG;
    m_DipoleSettings.include_eeg = bEEG;
}

//=============================================================================================================

void InvDipoleFit::onTimeChanged(int iMin,
                              int iMax,
                              int iStep,
                              int iInt)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.tmin = static_cast<float>(iMin)/1000.f;
    m_DipoleSettings.tmax = static_cast<float>(iMax)/1000.f;
    m_DipoleSettings.tstep = static_cast<float>(iStep)/1000.f;
    m_DipoleSettings.integ = static_cast<float>(iInt)/1000.f;
}

//=============================================================================================================

void InvDipoleFit::onFittingChanged(float fMinDistance,
                                 float fSize)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.guess_mindist = static_cast<float>(fMinDistance)/1000.f;
    m_DipoleSettings.guess_rad = static_cast<float>(fSize)/1000.f;
}

//=============================================================================================================

void InvDipoleFit::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    QMutexLocker lock(&m_FitMutex);

    for(QSharedPointer<ANSHAREDLIB::AbstractModel> pModel : m_ModelList){
        if (pModel == pNewModel){
            return;
        }
    }

    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        emit newMeasurment(QFileInfo(pNewModel->getModelPath()).fileName());
        m_ModelList.append(pNewModel);
    } else if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_BEMDATA_MODEL) {
        emit newBemModel(QFileInfo(pNewModel->getModelPath()).fileName());
        m_ModelList.append(pNewModel);
    } else if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_NOISE_MODEL) {
        emit newCovarianceModel(QFileInfo(pNewModel->getModelPath()).fileName());
        m_ModelList.append(pNewModel);
    } else if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_MRICOORD_MODEL) {
        emit newMriModel(QFileInfo(pNewModel->getModelPath()).fileName());
        m_ModelList.append(pNewModel);
    } else if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
        QSharedPointer<ANSHAREDLIB::AveragingDataModel> pAverageModel = qSharedPointerCast<AveragingDataModel>(pNewModel);
        if (pAverageModel->isFromFile()){
            emit newMeasurment(QFileInfo(pNewModel->getModelPath()).fileName());
            m_ModelList.append(pNewModel);
        }
    }
}

//=============================================================================================================

void InvDipoleFit::newDipoleFit(INVLIB::InvEcdSet set, const QString& sFitName)
{
    QSharedPointer<ANSHAREDLIB::DipoleFitModel> pModel = QSharedPointer<ANSHAREDLIB::DipoleFitModel>(new ANSHAREDLIB::DipoleFitModel(set));
    m_pAnalyzeData->addModel<ANSHAREDLIB::DipoleFitModel>(pModel,
                                                          sFitName);

}

//=============================================================================================================

void InvDipoleFit::onBaselineChanged(int iBMin,
                                  int iBMax)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.bmin = static_cast<float>(iBMin)/1000.f;
    m_DipoleSettings.bmax = static_cast<float>(iBMax)/1000.f;
}

//=============================================================================================================

void InvDipoleFit::onNoiseChanged(double dGrad,
                               double dMag,
                               double dEeg)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.grad_std = 1e-13*dGrad;
    m_DipoleSettings.mag_std = 1e-15*dMag;
    m_DipoleSettings.eeg_std = 1e-6*dEeg;

}

//=============================================================================================================

void InvDipoleFit::onRegChanged(double dRegGrad,
                             double dRegMag,
                             double dRegEeg)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.grad_reg = dRegGrad;
    m_DipoleSettings.mag_reg = dRegMag;
    m_DipoleSettings.eeg_reg = dRegEeg;
}

//=============================================================================================================

void InvDipoleFit::onSetChanged(int iSet)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.setno = iSet;

}

//=============================================================================================================

void InvDipoleFit::onSphereChanged(double dX,
                                double dY,
                                double dZ,
                                double dRadius)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.r0[0] = dX/1000.0;
    m_DipoleSettings.r0[1] = dY/1000.0;
    m_DipoleSettings.r0[2] = dZ/1000.0;

    m_DipoleSettings.eeg_sphere_rad = dRadius/1000.0;
}

//=============================================================================================================

void InvDipoleFit::onNewBemSelected(const QString &sName)
{
    if(sName == "None"){
        m_DipoleSettings.bemname = "";
        return;
    }
    for(QSharedPointer<ANSHAREDLIB::AbstractModel> pModel : m_ModelList){
        if (QFileInfo(pModel->getModelPath()).fileName() == sName){
            m_DipoleSettings.bemname = pModel->getModelPath();
            return;
        }
    }
}

//=============================================================================================================

void InvDipoleFit::onNewMriSelected(const QString &sName)
{
    if(sName == "None"){
        m_DipoleSettings.mriname = "";
        return;
    }
    for(QSharedPointer<ANSHAREDLIB::AbstractModel> pModel : m_ModelList){
        if (QFileInfo(pModel->getModelPath()).fileName() == sName){
            m_DipoleSettings.mriname = pModel->getModelPath();
            return;
        }
    }
}

//=============================================================================================================

void InvDipoleFit::onNewNoiseSelected(const QString &sName)
{
    if(sName == "None"){
        m_DipoleSettings.noisename = "";
        return;
    }
    for(QSharedPointer<ANSHAREDLIB::AbstractModel> pModel : m_ModelList){
        if (QFileInfo(pModel->getModelPath()).fileName() == sName){
            m_DipoleSettings.noisename = pModel->getModelPath();
            return;
        }
    }
}

//=============================================================================================================

void InvDipoleFit::onNewMeasSelected(const QString &sName)
{
    if(sName == "None"){
        m_DipoleSettings.measname = "";
        return;
    }

    //qDebug() << "InvDipoleFit::onNewMeasSelected";
    for(QSharedPointer<ANSHAREDLIB::AbstractModel> pModel : m_ModelList){
        if (QFileInfo(pModel->getModelPath()).fileName() == sName){
            if(pModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL){
                m_DipoleSettings.measname = pModel->getModelPath();
                m_DipoleSettings.is_raw = false;
            } else if(pModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
                m_DipoleSettings.measname = pModel->getModelPath();
                m_DipoleSettings.is_raw = true;
            }
        }
    }
}

//=============================================================================================================

INVLIB::InvEcdSet InvDipoleFit::dipoleFitCalculation()
{
    QMutexLocker lock(&m_FitMutex);

    qInfo() << "Checking integrity...";
    m_DipoleSettings.checkIntegrity();

    qInfo() << "Initializing settings...";
    INVLIB::InvDipoleFit dipFit(&m_DipoleSettings);

    qInfo() << "Calculating fit...";
    INVLIB::InvEcdSet ecdSet = dipFit.calculateFit();

    qInfo() << "Done!";
    INVLIB::InvEcdSet ecdSetTrans = ecdSet;

    QFile file(m_DipoleSettings.mriname);

    if(file.exists()) {
        FIFFLIB::FiffCoordTrans coordTrans(file);

        for(int i = 0; i < ecdSet.size() ; ++i) {
            MatrixX3f dipoles(1, 3);
            //transform location
            dipoles(0,0) = ecdSet[i].rd(0);
            dipoles(0,1) = ecdSet[i].rd(1);
            dipoles(0,2) = ecdSet[i].rd(2);

            dipoles = coordTrans.apply_trans(dipoles);

            ecdSetTrans[i].rd(0) = dipoles(0,0);
            ecdSetTrans[i].rd(1) = dipoles(0,1);
            ecdSetTrans[i].rd(2) = dipoles(0,2);

            //transform orientation
            dipoles(0,0) = ecdSet[i].Q(0);
            dipoles(0,1) = ecdSet[i].Q(1);
            dipoles(0,2) = ecdSet[i].Q(2);

            dipoles = coordTrans.apply_trans(dipoles, false);

            ecdSetTrans[i].Q(0) = dipoles(0,0);
            ecdSetTrans[i].Q(1) = dipoles(0,1);
            ecdSetTrans[i].Q(2) = dipoles(0,2);
        }
    } else {
        qWarning("[InvDipoleFit::onPerformDipoleFit] Cannot open FiffCoordTrans file");
    }

    return ecdSet;
}

//=============================================================================================================

void InvDipoleFit::dipoleFitResults()
{
    newDipoleFit(m_Future.result(), m_sFitName);

    triggerLoadingEnd("Performing InvDipole Fit...");
}

//=============================================================================================================

void InvDipoleFit::onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel)
{
    for(QSharedPointer<ANSHAREDLIB::AbstractModel> pModel : m_ModelList){
        if (pModel == pRemovedModel){
            m_ModelList.removeAt(m_ModelList.indexOf(pModel));

            if(pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
                emit removeModel(QFileInfo(pRemovedModel->getModelPath()).fileName(), 1);
            } else if(pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
                emit removeModel(QFileInfo(pRemovedModel->getModelPath()).fileName(), 1);
            } else if(pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_BEMDATA_MODEL) {
                emit removeModel(QFileInfo(pRemovedModel->getModelPath()).fileName(), 2);
            } else if(pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_NOISE_MODEL) {
                emit removeModel(QFileInfo(pRemovedModel->getModelPath()).fileName(), 4);
            } else if(pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_MRICOORD_MODEL) {
                emit removeModel(QFileInfo(pRemovedModel->getModelPath()).fileName(), 3);
            }
            return;
        }
    }
}

//=============================================================================================================

void InvDipoleFit::triggerLoadingStart(QString sMessage)
{
    m_pCommu->publishEvent(LOADING_START, QVariant::fromValue(sMessage));
}

//=============================================================================================================

void InvDipoleFit::triggerLoadingEnd(QString sMessage)
{
    m_pCommu->publishEvent(LOADING_END, QVariant::fromValue(sMessage));
}

//=============================================================================================================

QString InvDipoleFit::getBuildInfo()
{
    return QString(DIPOLEFITPLUGIN::buildDateTime()) + QString(" - ")  + QString(DIPOLEFITPLUGIN::buildHash());
}
