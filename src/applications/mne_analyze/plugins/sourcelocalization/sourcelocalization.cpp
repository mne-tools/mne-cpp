//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     sourcelocalization.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.6
 * @date     August, 2020
 * @brief    SourceLocalization class defintion.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourcelocalization.h"

#include <anShared/Management/communicator.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SOURCELOCALIZATIONPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceLocalization::SourceLocalization()
: m_pCommu(Q_NULLPTR)
{
}

//=============================================================================================================

SourceLocalization::~SourceLocalization()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> SourceLocalization::clone() const
{
    QSharedPointer<SourceLocalization> pSourceLocalizationClone(new SourceLocalization);
    return pSourceLocalizationClone;
}

//=============================================================================================================

void SourceLocalization::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void SourceLocalization::unload()
{

}

//=============================================================================================================

QString SourceLocalization::getName() const
{
    return "Source Localization";
}

//=============================================================================================================

QMenu *SourceLocalization::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QWidget *SourceLocalization::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget* SourceLocalization::getControl()
{
    //QDockWidget* pControl = new QDockWidget(getName());

    return Q_NULLPTR;
}

//=============================================================================================================

void SourceLocalization::handleEvent(QSharedPointer<Event> e)
{
    // See DataManager::handleEvent: a switch with only a default label is
    // MSVC C4065, and this plugin subscribes to no events anyway.
    qWarning() << "[SourceLocalization::handleEvent] Received an Event that is not handled:"
               << e->getType();
}

//=============================================================================================================

QVector<EVENT_TYPE> SourceLocalization::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;
    //temp.push_back(SELECTED_MODEL_CHANGED);

    return temp;
}

//=============================================================================================================

QString SourceLocalization::getBuildInfo()
{
    return QString(SOURCELOCALIZATIONPLUGIN::buildDateTime()) + QString(" - ")  + QString(SOURCELOCALIZATIONPLUGIN::buildHash());
}
