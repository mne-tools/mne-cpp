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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

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
    return "Source Localization";
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
    //QDockWidget* pControl = new QDockWidget(getName());

    return Q_NULLPTR;
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
