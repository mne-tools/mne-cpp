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

#include <mne/base/log.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SAMPLEPLUGINPLUGIN;
using namespace mne::analyze;

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

QMenu* SamplePlugin::getMenu()
{
    return nullptr;
}

//=============================================================================================================

QDockWidget* SamplePlugin::getControl()
{
    //If plugin has dock controls:
    auto pControlDock = std::make_unique<QDockWidget>(getName());
    pControlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    pControlDock->setObjectName(getName());

    QWidget* pWidget = new QWidget(pControlDock.get());
    new QVBoxLayout(pWidget);

    pControlDock->setWidget(pWidget);

    return pControlDock.release();

    //If plugin does not have dock controls:
    return nullptr;
}

//=============================================================================================================

QWidget* SamplePlugin::getView()
{
    //If the plugin has a view:
    auto pPluginView = std::make_unique<QWidget>();
    new QVBoxLayout(pPluginView.get());

    return pPluginView.release();

    //If the plugin does not have a view:
    return nullptr;
}

//=============================================================================================================

void SamplePlugin::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        default:
            mne::Log::warning("[SamplePlugin::handleEvent] received an Event that is not handled by switch-cases");
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
