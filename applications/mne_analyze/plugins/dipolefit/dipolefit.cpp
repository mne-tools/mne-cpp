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

#include <disp/viewers/dipolefitview.h>

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
}

//=============================================================================================================

void DipoleFit::onModalityChanged(int iModality)
{
    QMutexLocker lock(&m_FitMutex);

    if (iModality == 0) {
        m_DipoleSettings.include_meg = true;
        m_DipoleSettings.include_eeg = false;
    } else if(iModality == 1) {
        m_DipoleSettings.include_meg = false;
        m_DipoleSettings.include_eeg = true;
    }
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
