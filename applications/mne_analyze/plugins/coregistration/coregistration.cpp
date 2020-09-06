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

#include "disp/viewers/coregsettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COREGISTRATIONPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CoRegistration::CoRegistration()
    : m_pCoregSettingsView(Q_NULLPTR)
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
    m_pCoregSettingsView = new DISPLIB::CoregSettingsView(QString("MNEANALYZE/%1").arg(this->getName()));

    QDockWidget* pControlDock = new QDockWidget(getName());
    pControlDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    pControlDock->setWidget(m_pCoregSettingsView);
    pControlDock->setObjectName(getName());

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
        case NEW_BEM_ADDED:
            updateBemList();
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
    temp.push_back(NEW_BEM_ADDED);
    return temp;
}

//=============================================================================================================

void CoRegistration::updateBemList()
{
    m_pCoregSettingsView->clearSelectionGroup();
    QVector<QSharedPointer<AbstractModel> > vecBemList = m_pAnalyzeData->getModelsByType(ANSHAREDLIB_BEMDATA_MODEL);
    for(int i = 0; i < vecBemList.size(); i++){
        m_pCoregSettingsView->addSelectionGroup(vecBemList.at(i)->getModelName());
    }
}

