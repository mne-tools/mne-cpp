//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     sampleplugin.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.5
 * @date     August, 2020
 * @brief    Definition of the SamplePlugin class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sampleplugin.h"

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

using namespace SAMPLEPLUGINPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SamplePlugin::SamplePlugin()
{
}

//=============================================================================================================

SamplePlugin::~SamplePlugin()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> SamplePlugin::clone() const
{
    QSharedPointer<SamplePlugin> pSamplePluginClone = QSharedPointer<SamplePlugin>::create();
    return pSamplePluginClone;
}

//=============================================================================================================

void SamplePlugin::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void SamplePlugin::unload()
{
}

//=============================================================================================================

QString SamplePlugin::getName() const
{
    return "Sample Plugin";
}

//=============================================================================================================

QMenu *SamplePlugin::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *SamplePlugin::getControl()
{
    //If plugin has dock controls:
    QDockWidget* pControlDock = new QDockWidget(getName());
    pControlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pControlDock->setObjectName(getName());

    QWidget* pWidget = new QWidget;
    QVBoxLayout* pLayout = new QVBoxLayout;

    pWidget->setLayout(pLayout);
    pControlDock->setWidget(pWidget);

    return pControlDock;

    //If plugin does not have dock controls:
    return Q_NULLPTR;
}

//=============================================================================================================

QWidget *SamplePlugin::getView()
{
    //If the plugin has a view:
    QWidget* pPluginView = new QWidget();
    QVBoxLayout* pViewLayout = new QVBoxLayout();

    pPluginView->setLayout(pViewLayout);

    return pPluginView;

    //If the plugin does not have a view:
    return Q_NULLPTR;
}

//=============================================================================================================

void SamplePlugin::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    default:
        qWarning() << "[SamplePlugin::handleEvent] received an Event that is not handled by switch-cases";
        break;
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> SamplePlugin::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp;

    return temp;
}

//=============================================================================================================

QString SamplePlugin::getBuildInfo()
{
    return QString(SAMPLEPLUGINPLUGIN::buildDateTime()) + QString(" - ") + QString(SAMPLEPLUGINPLUGIN::buildHash());

}

