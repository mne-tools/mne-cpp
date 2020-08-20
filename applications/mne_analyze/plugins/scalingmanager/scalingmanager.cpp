//=============================================================================================================
/**
 * @file     scalingmanager.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.5
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
 * @brief    Definition of the ScalingManager class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scalingmanager.h"

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <anShared/Utils/metatypes.h>

#include <disp/viewers/scalingview.h>
#include <disp/viewers/applytoview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCALINGMANAGERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ScalingManager::ScalingManager()
: m_pSelectionParameters(new ANSHAREDLIB::ScalingParameters)
{
}

//=============================================================================================================

ScalingManager::~ScalingManager()
{
    if(m_pSelectionParameters){
        delete m_pSelectionParameters;
    }
}

//=============================================================================================================

QSharedPointer<IPlugin> ScalingManager::clone() const
{
    QSharedPointer<ScalingManager> pScalingManagerClone = QSharedPointer<ScalingManager>::create();
    return pScalingManagerClone;
}

//=============================================================================================================

void ScalingManager::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void ScalingManager::unload()
{
}

//=============================================================================================================

QString ScalingManager::getName() const
{
    return "Scaling";
}

//=============================================================================================================

QMenu *ScalingManager::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *ScalingManager::getControl()
{
    QDockWidget* pControlDock = new QDockWidget(getName());
    pControlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pControlDock->setObjectName(getName());

    QScrollArea* wrappedScrollArea = new QScrollArea(pControlDock);
    QVBoxLayout* pLayout = new QVBoxLayout;

    DISPLIB::ScalingView* pScalingWidget = new DISPLIB::ScalingView("MNEANALYZE", wrappedScrollArea);
    m_pApplyToView = new DISPLIB::ApplyToView();

    pLayout->addWidget(pScalingWidget);
    pLayout->addWidget(m_pApplyToView);
    wrappedScrollArea->setLayout(pLayout);
    pControlDock->setWidget(wrappedScrollArea);

    connect(pScalingWidget, &DISPLIB::ScalingView::scalingChanged,
            this, &ScalingManager::onScalingChanged, Qt::UniqueConnection);

    m_pSelectionParameters->m_mScalingMap = pScalingWidget->getScaleMap();
    m_pSelectionParameters->m_mScalingMap.detach();

    return pControlDock;
}

//=============================================================================================================

QWidget *ScalingManager::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void ScalingManager::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    case SELECTED_MODEL_CHANGED:
        onScalingChanged(m_pSelectionParameters->m_mScalingMap);
        break;
    default:
        qWarning() << "[ScalingManager::handleEvent] received an Event that is not handled by switch-cases";
        break;
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> ScalingManager::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    temp.push_back(SELECTED_MODEL_CHANGED);

    return temp;
}

//=============================================================================================================

void ScalingManager::onScalingChanged(const QMap<qint32, float> &scalingMap)
{
    qDebug() << "[ScalingManager::onScalingChanged]";

    m_pSelectionParameters->m_sViewsToApply = m_pApplyToView->getSelectedViews();

    m_pSelectionParameters->m_mScalingMap = scalingMap;
    m_pSelectionParameters->m_mScalingMap.detach();

    m_pCommu->publishEvent(EVENT_TYPE::SCALING_MAP_CHANGED, QVariant::fromValue(m_pSelectionParameters));
}

//=============================================================================================================

