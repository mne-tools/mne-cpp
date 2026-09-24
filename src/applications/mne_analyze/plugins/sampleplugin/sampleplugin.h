//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     sampleplugin.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.5
 * @date     August, 2020
 * @brief    Contains the declaration of the SamplePlugin class.
 */

#ifndef SAMPLEPLUGIN_H
#define SAMPLEPLUGIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sampleplugin_global.h"
#include <anShared/Plugins/abstractplugin.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
}

//=============================================================================================================
// DEFINE NAMESPACE SURFERPLUGIN
//=============================================================================================================

namespace SAMPLEPLUGINPLUGIN
{

//=============================================================================================================
/**
 * SamplePlugin Plugin
 *
 * @brief The SamplePlugin class provides a view with all currently loaded models.
 */
class SAMPLEPLUGINSHARED_EXPORT SamplePlugin : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "sampleplugin.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs a SamplePlugin.
     */
    SamplePlugin();

    //=========================================================================================================
    /**
     * Destroys the SamplePlugin.
     */
    virtual ~SamplePlugin() override;

    // IPlugin functions
    virtual QSharedPointer<AbstractPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;
    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;
    virtual QString getBuildInfo() override;
    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

private:
    QPointer<ANSHAREDLIB::Communicator>     m_pCommu;

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // SAMPLEPLUGIN_H
