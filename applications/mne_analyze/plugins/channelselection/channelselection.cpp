//=============================================================================================================
/**
 * @file     channelselection.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu
 * @since    0.1.4
 * @date     July, 2020
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
 * @brief    Contains the definition of the ChannelSelection class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelselection.h"

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <anShared/Utils/metatypes.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CHANNELSELECTIONPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelSelection::ChannelSelection()
{
}

//=============================================================================================================

ChannelSelection::~ChannelSelection()
{
}

//=============================================================================================================

QSharedPointer<IPlugin> ChannelSelection::clone() const
{
    QSharedPointer<ChannelSelection> pChannelSelectionClone = QSharedPointer<ChannelSelection>::create();
    return pChannelSelectionClone;
}

//=============================================================================================================

void ChannelSelection::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void ChannelSelection::unload()
{
}

//=============================================================================================================

QString ChannelSelection::getName() const
{
    return "Channel Selection";
}

//=============================================================================================================

QMenu *ChannelSelection::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *ChannelSelection::getControl()
{
    QDockWidget* temp = new QDockWidget();
    return temp;

    return Q_NULLPTR;
}

//=============================================================================================================

QWidget *ChannelSelection::getView()
{
    QWidget* temp = new QWidget();
    return temp;

    //return Q_NULLPTR;
}

//=============================================================================================================

void ChannelSelection::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    default:
        qWarning() << "[ChannelSelection::handleEvent] received an Event that is not handled by switch-cases";
        break;
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> ChannelSelection::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;

    return temp;
}
