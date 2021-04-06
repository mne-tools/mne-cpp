//=============================================================================================================
/**
 * @file     timefrequency.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Contains the declaration of the TimeFrequency class.
 *
 */

#ifndef TIMEFREQUENCY_H
#define TIMEFREQUENCY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequency_global.h"

#include <anShared/Plugins/abstractplugin.h>
#include <anShared/Model/fiffrawviewmodel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
    class AbstractModel;
}

namespace DISPLIB {
    class ProgressView;
}

//=============================================================================================================
// DEFINE NAMESPACE TIMEFREQUENCYPLUGIN
//=============================================================================================================

namespace TIMEFREQUENCYPLUGIN
{

//=============================================================================================================
// TIMEFREQUENCYPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class TimeFrequencyControl;

//=============================================================================================================
/**
 * TimeFrequency Plugin
 *
 * @brief The TimeFrequency class provides input and output capabilities for the fiff file format.
 */
class TIMEFREQUENCYSHARED_EXPORT TimeFrequency : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "timefrequency.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)
public:
    //=========================================================================================================
    /**
     * Constructs a TimeFrequency.
     */
    TimeFrequency();

    //=========================================================================================================
    /**
     * Destroys the TimeFrequency.
     */
    ~TimeFrequency() override;

    // AbstractPlugin functions
    virtual QSharedPointer<AbstractPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;

    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;

    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

private:
    //=========================================================================================================
    /**
     * Loads settings from the system.
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Save the settings in the register/file/mechanism for lon-term storage.
     */
    void saveSettings();

    QPointer<ANSHAREDLIB::Communicator>             m_pCommu;                   /**< Used for sending events */
    QString                                         m_sSettingsPath;            /**< Variable that stores the key where to store settings for this plugin.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // TIMEFREQUENCY_H
