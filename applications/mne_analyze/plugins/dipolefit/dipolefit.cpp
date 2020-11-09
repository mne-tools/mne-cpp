//=============================================================================================================
/**
 * @file     dipolefit.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
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
 * @brief    Definition of the DipoleFit class.
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
#include <anShared/Model/noisemodel.h>
#include <anShared/Model/averagingdatamodel.h>

#include <disp/viewers/dipolefitview.h>

#include <inverse/dipoleFit/dipole_fit_data.h>
#include <inverse/dipoleFit/dipole_fit.h>
#include <inverse/dipoleFit/ecd_set.h>

#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DIPOLEFITPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFit::DipoleFit()
{
}

//=============================================================================================================

DipoleFit::~DipoleFit()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> DipoleFit::clone() const
{
    QSharedPointer<DipoleFit> pDipoleFitClone = QSharedPointer<DipoleFit>::create();
    return pDipoleFitClone;
}

//=============================================================================================================

void DipoleFit::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void DipoleFit::unload()
{
}

//=============================================================================================================

QString DipoleFit::getName() const
{
    return "Dipole Fit";
}

//=============================================================================================================

QMenu *DipoleFit::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *DipoleFit::getControl()
{
    QDockWidget* pDockWidgt = new QDockWidget(getName());
    DISPLIB::DipoleFitView* pDipoleView = new DISPLIB::DipoleFitView();
    pDockWidgt->setWidget(pDipoleView);

    connect(this, &DipoleFit::newBemModel,
            pDipoleView, &DISPLIB::DipoleFitView::setBem, Qt::UniqueConnection);
    connect(this, &DipoleFit::newNoiseModel,
            pDipoleView, &DISPLIB::DipoleFitView::setNoise, Qt::UniqueConnection);
    connect(this, &DipoleFit::newMriModel,
            pDipoleView, &DISPLIB::DipoleFitView::setMri, Qt::UniqueConnection);
    connect(this, &DipoleFit::getUpdate,
            pDipoleView, &DISPLIB::DipoleFitView::requestParams, Qt::UniqueConnection);

    connect(pDipoleView, &DISPLIB::DipoleFitView::modalityChanged,
            this, &DipoleFit::onModalityChanged, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::timeChanged,
            this, &DipoleFit::onTimeChanged, Qt::UniqueConnection);
    connect(pDipoleView, &DISPLIB::DipoleFitView::fittingChanged,
            this, &DipoleFit::onFittingChanged, Qt::UniqueConnection);

    connect(pDipoleView, &DISPLIB::DipoleFitView::clearBem, [=]{
            QMutexLocker lock(&m_FitMutex);
            m_DipoleSettings.bemname = "";
            });
    connect(pDipoleView, &DISPLIB::DipoleFitView::clearMri, [=]{
            QMutexLocker lock(&m_FitMutex);
            m_DipoleSettings.mriname = "";
            });
    connect(pDipoleView, &DISPLIB::DipoleFitView::clearBem, [=]{
            QMutexLocker lock(&m_FitMutex);
            m_DipoleSettings.noisename = "";
            });

    emit getUpdate();

    return pDockWidgt;
}

//=============================================================================================================

QWidget *DipoleFit::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void DipoleFit::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    case EVENT_TYPE::SELECTED_MODEL_CHANGED:
        onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
        break;
    default:
        qWarning() << "[DipoleFit::handleEvent] received an Event that is not handled by switch-cases";
        break;
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> DipoleFit::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);
    return temp;
}

//=============================================================================================================

void DipoleFit::onPerformDipoleFit()
{
    INVERSELIB::DipoleFit dipFit(&m_DipoleSettings);
    INVERSELIB::ECDSet ecdSet = dipFit.calculateFit();
    INVERSELIB::ECDSet ecdSetTrans = ecdSet;

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
        qWarning("[DipoleFit::onPerformDipoleFit] Cannot open FiffCoordTrans file");
    }

    newDipoleFit(ecdSetTrans);
}

//=============================================================================================================

void DipoleFit::onModalityChanged(bool bEEG, bool bMEG)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.include_meg = bMEG;
    m_DipoleSettings.include_eeg = bEEG;
}

//=============================================================================================================

void DipoleFit::onTimeChanged(int iMin,
                              int iMax,
                              int iStep)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.tmin = static_cast<float>(iMin)/1000.f;
    m_DipoleSettings.tmax = static_cast<float>(iMax)/1000.f;
    m_DipoleSettings.tstep = static_cast<float>(iStep)/1000.f;
}

//=============================================================================================================

void DipoleFit::onFittingChanged(float fMinDistance,
                                 float fGridSize)
{
    QMutexLocker lock(&m_FitMutex);

    m_DipoleSettings.guess_mindist = static_cast<float>(fMinDistance)/1000.f;
    m_DipoleSettings.guess_rad = static_cast<float>(fGridSize)/1000.f;
}

//=============================================================================================================

void DipoleFit::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        if(m_pFiffRawModel) {
            if(m_pFiffRawModel == pNewModel) {
                qInfo() << "[DipoleFit::onModelChanged] New model is the same as old model";
                return;
            }
        }
        m_pFiffRawModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);
        m_DipoleSettings.measname = pNewModel->getModelPath();
        m_DipoleSettings.is_raw = true;
        emit newMeasurment(QFileInfo(pNewModel->getModelPath()).fileName());

    } else if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_BEMDATA_MODEL) {
        if(m_pBemModel) {
            if(m_pBemModel == pNewModel) {
                qInfo() << "[DipoleFit::onModelChanged] New model is the same as old model";
                return;
            }
        }
        m_pBemModel = qSharedPointerCast<BemDataModel>(pNewModel);
        m_DipoleSettings.bemname = pNewModel->getModelPath();
        emit newBemModel(QFileInfo(pNewModel->getModelPath()).fileName());

    } else if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_NOISE_MODEL) {
        if(m_pNoiseModel) {
            if(m_pNoiseModel == pNewModel) {
                qInfo() << "[DipoleFit::onModelChanged] New model is the same as old model";
                return;
            }
        }
        m_pNoiseModel = qSharedPointerCast<NoiseModel>(pNewModel);
        m_DipoleSettings.noisename = pNewModel->getModelPath();
        emit newNoiseModel(QFileInfo(pNewModel->getModelPath()).fileName());

    } else if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_MRICOORD_MODEL) {
        if(m_pMriModel) {
            if(m_pMriModel == pNewModel) {
                qInfo() << "[DipoleFit::onModelChanged] New model is the same as old model";
                return;
            }
        }
        m_pMriModel = qSharedPointerCast<MriCoordModel>(pNewModel);
        m_DipoleSettings.mriname = pNewModel->getModelPath();
        emit newMriModel(QFileInfo(pNewModel->getModelPath()).fileName());

    } else if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
        if(m_pAverageModel) {
            if(m_pAverageModel == pNewModel) {
                qInfo() << "[DipoleFit::onModelChanged] New model is the same as old model";
                return;
            }
        }
        m_pAverageModel = qSharedPointerCast<AveragingDataModel>(pNewModel);
        m_DipoleSettings.measname = pNewModel->getModelPath();
        m_DipoleSettings.is_raw = false;
        emit newMeasurment(QFileInfo(pNewModel->getModelPath()).fileName());

    }
}

//=============================================================================================================

void DipoleFit::newDipoleFit(INVERSELIB::ECDSet set)
{
    m_pCommu->publishEvent(EVENT_TYPE::NEW_DIPOLE_FIT_DATA, QVariant::fromValue(set));
}
