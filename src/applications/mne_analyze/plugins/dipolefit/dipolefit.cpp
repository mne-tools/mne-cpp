//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     dipolefit.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.7
 * @date     October, 2020
 * @brief    Definition of the InvDipoleFit class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "dipolefit.h"

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>


#include <anShared/Model/abstractmodel.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/bemdatamodel.h>
#include <anShared/Model/mricoordmodel.h>
#include <anShared/Model/covariancemodel.h>
#include <anShared/Model/averagingdatamodel.h>
#include <anShared/Model/dipolefitmodel.h>

#include <mne/disp/viewers/dipolefitview.h>

#include <mne/inv/dipole_fit/inv_dipole_fit_data.h>
#include <mne/inv/dipole_fit/inv_dipole_fit.h>
#include <mne/inv/dipole_fit/inv_ecd_set.h>

#include <mne/base/thread_pool.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DIPOLEFITPLUGIN;
using namespace mne::analyze;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvDipoleFit::InvDipoleFit()
{
}

//=============================================================================================================

InvDipoleFit::~InvDipoleFit()
{
    if (m_Future.valid()) {
        m_Future.wait();
    }
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
    return "Dipole Fit";
}

//=============================================================================================================

QMenu* InvDipoleFit::getMenu()
{
    return nullptr;
}

//=============================================================================================================

QDockWidget* InvDipoleFit::getControl()
{
    mne::disp::DipoleFitView* pDipoleView = new mne::disp::DipoleFitView();

    //Send Gui updates
    connect(this, &InvDipoleFit::newBemModel,
            pDipoleView, &mne::disp::DipoleFitView::addBem, Qt::UniqueConnection);
    connect(this, &InvDipoleFit::newCovarianceModel,
            pDipoleView, &mne::disp::DipoleFitView::addNoise, Qt::UniqueConnection);
    connect(this, &InvDipoleFit::newMriModel,
            pDipoleView, &mne::disp::DipoleFitView::addMri, Qt::UniqueConnection);
    connect(this, &InvDipoleFit::newMeasurment,
            pDipoleView, &mne::disp::DipoleFitView::addMeas, Qt::UniqueConnection);
    connect(this, &InvDipoleFit::getUpdate,
            pDipoleView, &mne::disp::DipoleFitView::requestParams, Qt::UniqueConnection);
    connect(this, &InvDipoleFit::removeModel,
            pDipoleView, &mne::disp::DipoleFitView::removeModel, Qt::UniqueConnection);

    //Receive Gui updates
    connect(pDipoleView, &mne::disp::DipoleFitView::modalityChanged,
            this, &InvDipoleFit::onModalityChanged, Qt::UniqueConnection);
    connect(pDipoleView, &mne::disp::DipoleFitView::timeChanged,
            this, &InvDipoleFit::onTimeChanged, Qt::UniqueConnection);
    connect(pDipoleView, &mne::disp::DipoleFitView::fittingChanged,
            this, &InvDipoleFit::onFittingChanged, Qt::UniqueConnection);
    connect(pDipoleView, &mne::disp::DipoleFitView::baselineChanged,
            this, &InvDipoleFit::onBaselineChanged, Qt::UniqueConnection);
    connect(pDipoleView, &mne::disp::DipoleFitView::noiseChanged,
            this, &InvDipoleFit::onNoiseChanged, Qt::UniqueConnection);
    connect(pDipoleView, &mne::disp::DipoleFitView::regChanged,
            this, &InvDipoleFit::onRegChanged, Qt::UniqueConnection);
    connect(pDipoleView, &mne::disp::DipoleFitView::setChanged,
            this, &InvDipoleFit::onSetChanged, Qt::UniqueConnection);
    connect(pDipoleView, &mne::disp::DipoleFitView::sphereChanged,
            this, &InvDipoleFit::onSphereChanged, Qt::UniqueConnection);

    connect(pDipoleView, &mne::disp::DipoleFitView::selectedBem,
            this, &InvDipoleFit::onNewBemSelected, Qt::UniqueConnection);
    connect(pDipoleView, &mne::disp::DipoleFitView::selectedMri,
            this, &InvDipoleFit::onNewMriSelected, Qt::UniqueConnection);
    connect(pDipoleView, &mne::disp::DipoleFitView::selectedNoise,
            this, &InvDipoleFit::onNewNoiseSelected, Qt::UniqueConnection);
    connect(pDipoleView, &mne::disp::DipoleFitView::selectedMeas,
            this, &InvDipoleFit::onNewMeasSelected, Qt::UniqueConnection);

    //Fit
    connect(pDipoleView, &mne::disp::DipoleFitView::performDipoleFit,
            this, &InvDipoleFit::onPerformDipoleFit, Qt::UniqueConnection);

    QDockWidget* pDockWidgt = new QDockWidget(getName());
    pDockWidgt->setWidget(pDipoleView);
    pDockWidgt->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pDockWidgt->setObjectName(getName());

    emit getUpdate();

    return pDockWidgt;
}

//=============================================================================================================

QWidget* InvDipoleFit::getView()
{
    return nullptr;
}

//=============================================================================================================

void InvDipoleFit::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case EVENT_TYPE::SELECTED_MODEL_CHANGED:
            onModelChanged(e->getData().value<QSharedPointer<mne::analyze::AbstractModel>>());
            break;
        case EVENT_TYPE::MODEL_REMOVED:
            onModelRemoved(e->getData().value<QSharedPointer<mne::analyze::AbstractModel>>());
            break;
        default:
            mne::Log::warning("[InvDipoleFit::handleEvent] received an Event that is not handled by switch-cases");
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

    triggerLoadingStart("Performing Dipole Fit...");

    // Only the most recently started fit reports its result
    const quint64 iGeneration = ++m_iFitGeneration;
    m_Future = mne::ThreadPool::global().submit([this, pThis = QPointer<InvDipoleFit>(this), iGeneration] {
        mne::InvEcdSet ecdSet = this->dipoleFitCalculation();
        if (pThis) {
            QMetaObject::invokeMethod(pThis, [this, iGeneration] {
                if (iGeneration == m_iFitGeneration) {
                    dipoleFitResults();
                } }, Qt::QueuedConnection);
        }
        return ecdSet;
    });
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

    m_DipoleSettings.tmin = static_cast<float>(iMin) / 1000.f;
    m_DipoleSettings.tmax = static_cast<float>(iMax) / 1000.f;
    m_DipoleSettings.tstep = static_cast<float>(iStep) / 1000.f;
    m_DipoleSettings.integ = static_cast<float>(iInt) / 1000.f;
}

//=============================================================================================================

void InvDipoleFit::onFittingChanged(float fMinDistance,
                                    float fSize)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.guess_mindist = static_cast<float>(fMinDistance) / 1000.f;
    m_DipoleSettings.guess_rad = static_cast<float>(fSize) / 1000.f;
}

//=============================================================================================================

void InvDipoleFit::onModelChanged(QSharedPointer<mne::analyze::AbstractModel> pNewModel)
{
    QMutexLocker lock(&m_FitMutex);

    for (QSharedPointer<mne::analyze::AbstractModel> pModel : m_ModelList) {
        if (pModel == pNewModel) {
            return;
        }
    }

    if (pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        emit newMeasurment(QFileInfo(pNewModel->getModelPath()).fileName());
        m_ModelList.append(pNewModel);
    } else if (pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_BEMDATA_MODEL) {
        emit newBemModel(QFileInfo(pNewModel->getModelPath()).fileName());
        m_ModelList.append(pNewModel);
    } else if (pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_NOISE_MODEL) {
        emit newCovarianceModel(QFileInfo(pNewModel->getModelPath()).fileName());
        m_ModelList.append(pNewModel);
    } else if (pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_MRICOORD_MODEL) {
        emit newMriModel(QFileInfo(pNewModel->getModelPath()).fileName());
        m_ModelList.append(pNewModel);
    } else if (pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
        QSharedPointer<mne::analyze::AveragingDataModel> pAverageModel = qSharedPointerCast<AveragingDataModel>(pNewModel);
        if (pAverageModel->isFromFile()) {
            emit newMeasurment(QFileInfo(pNewModel->getModelPath()).fileName());
            m_ModelList.append(pNewModel);
        }
    }
}

//=============================================================================================================

void InvDipoleFit::newDipoleFit(mne::InvEcdSet set, const QString& sFitName)
{
    QSharedPointer<mne::analyze::DipoleFitModel> pModel = QSharedPointer<mne::analyze::DipoleFitModel>(new mne::analyze::DipoleFitModel(set));
    m_pAnalyzeData->addModel<mne::analyze::DipoleFitModel>(pModel,
                                                           sFitName);
}

//=============================================================================================================

void InvDipoleFit::onBaselineChanged(int iBMin,
                                     int iBMax)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.bmin = static_cast<float>(iBMin) / 1000.f;
    m_DipoleSettings.bmax = static_cast<float>(iBMax) / 1000.f;
}

//=============================================================================================================

void InvDipoleFit::onNoiseChanged(double dGrad,
                                  double dMag,
                                  double dEeg)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.grad_std = 1e-13 * dGrad;
    m_DipoleSettings.mag_std = 1e-15 * dMag;
    m_DipoleSettings.eeg_std = 1e-6 * dEeg;
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

    m_DipoleSettings.r0[0] = dX / 1000.0;
    m_DipoleSettings.r0[1] = dY / 1000.0;
    m_DipoleSettings.r0[2] = dZ / 1000.0;

    m_DipoleSettings.eeg_sphere_rad = dRadius / 1000.0;
}

//=============================================================================================================

void InvDipoleFit::onNewBemSelected(const QString& sName)
{
    if (sName == "None") {
        m_DipoleSettings.bemname = "";
        return;
    }
    for (QSharedPointer<mne::analyze::AbstractModel> pModel : m_ModelList) {
        if (QFileInfo(pModel->getModelPath()).fileName() == sName) {
            m_DipoleSettings.bemname = pModel->getModelPath().toStdString();
            return;
        }
    }
}

//=============================================================================================================

void InvDipoleFit::onNewMriSelected(const QString& sName)
{
    if (sName == "None") {
        m_DipoleSettings.mriname = "";
        return;
    }
    for (QSharedPointer<mne::analyze::AbstractModel> pModel : m_ModelList) {
        if (QFileInfo(pModel->getModelPath()).fileName() == sName) {
            m_DipoleSettings.mriname = pModel->getModelPath().toStdString();
            return;
        }
    }
}

//=============================================================================================================

void InvDipoleFit::onNewNoiseSelected(const QString& sName)
{
    if (sName == "None") {
        m_DipoleSettings.noisename = "";
        return;
    }
    for (QSharedPointer<mne::analyze::AbstractModel> pModel : m_ModelList) {
        if (QFileInfo(pModel->getModelPath()).fileName() == sName) {
            m_DipoleSettings.noisename = pModel->getModelPath().toStdString();
            return;
        }
    }
}

//=============================================================================================================

void InvDipoleFit::onNewMeasSelected(const QString& sName)
{
    if (sName == "None") {
        m_DipoleSettings.measname = "";
        return;
    }

    //qDebug() << "InvDipoleFit::onNewMeasSelected";
    for (QSharedPointer<mne::analyze::AbstractModel> pModel : m_ModelList) {
        if (QFileInfo(pModel->getModelPath()).fileName() == sName) {
            if (pModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
                m_DipoleSettings.measname = pModel->getModelPath().toStdString();
                m_DipoleSettings.is_raw = false;
            } else if (pModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
                m_DipoleSettings.measname = pModel->getModelPath().toStdString();
                m_DipoleSettings.is_raw = true;
            }
        }
    }
}

//=============================================================================================================

mne::InvEcdSet InvDipoleFit::dipoleFitCalculation()
{
    QMutexLocker lock(&m_FitMutex);

    mne::Log::info("Checking integrity...");
    if (!m_DipoleSettings.checkIntegrity()) {
        return mne::InvEcdSet();
    }

    mne::Log::info("Initializing settings...");
    mne::InvDipoleFit dipFit(&m_DipoleSettings);

    mne::Log::info("Calculating fit...");
    mne::InvEcdSet ecdSet = dipFit.calculateFit();

    mne::Log::info("Done!");
    return ecdSet;
}

//=============================================================================================================

void InvDipoleFit::dipoleFitResults()
{
    newDipoleFit(m_Future.get(), m_sFitName);

    triggerLoadingEnd("Performing Dipole Fit...");
}

//=============================================================================================================

void InvDipoleFit::onModelRemoved(QSharedPointer<mne::analyze::AbstractModel> pRemovedModel)
{
    for (QSharedPointer<mne::analyze::AbstractModel> pModel : m_ModelList) {
        if (pModel == pRemovedModel) {
            m_ModelList.removeAt(m_ModelList.indexOf(pModel));

            if (pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
                emit removeModel(QFileInfo(pRemovedModel->getModelPath()).fileName(), 1);
            } else if (pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
                emit removeModel(QFileInfo(pRemovedModel->getModelPath()).fileName(), 1);
            } else if (pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_BEMDATA_MODEL) {
                emit removeModel(QFileInfo(pRemovedModel->getModelPath()).fileName(), 2);
            } else if (pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_NOISE_MODEL) {
                emit removeModel(QFileInfo(pRemovedModel->getModelPath()).fileName(), 4);
            } else if (pRemovedModel->getType() == MODEL_TYPE::ANSHAREDLIB_MRICOORD_MODEL) {
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
    return QString(DIPOLEFITPLUGIN::buildDateTime()) + QString(" - ") + QString(DIPOLEFITPLUGIN::buildHash());
}
