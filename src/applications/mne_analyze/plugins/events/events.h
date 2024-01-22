//=============================================================================================================
/**
 * @file     events.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.9
 * @date     March, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Lorenz Esch, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Contains the declaration of the events class.
 *
 */

#ifndef ANALYZE_EVENTS_H
#define ANALYZE_EVENTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "events_global.h"
#include "eventdelegate.h"
#include "eventview.h"

#include <anShared/Plugins/abstractplugin.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
    class FiffRawViewModel;
}

//=============================================================================================================
// DEFINE NAMESPACE eventsPLUGIN
//=============================================================================================================

namespace EVENTSPLUGIN
{

//=============================================================================================================
// EVENTSPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * events Plugin
 *
 * @brief The events class provides input and output capabilities for the fiff file format.
 */
class EVENTSSHARED_EXPORT Events : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "events.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs an Events.
     */
    Events();

    //=========================================================================================================
    /**
     * Destroys the Events.
     */
    ~Events() override;

    // AbstractPlugin functions
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
    //=========================================================================================================
    /**
     * Loads new Fiff model whan current loaded model is changed
     *
     * @param[in, out] pNewModel    pointer to currently loaded FiffRawView Model.
     */
    void onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

    //=========================================================================================================
    /**
     * Handles clearing view if currently used model is being removed
     *
     * @param[in] pRemovedModel    Pointer to model being removed.
     */
    void onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel);

    //=========================================================================================================
    /**
     * Toggles whether to show events
     *
     * @param[in] iToggle  0 for not shown, 2 for shown.
     */
    void toggleDisplayEvent(int iToggle);

    //=========================================================================================================
    /**
     * Publishes event to force FiffRawView to redraw the data viewer
     */
    void onTriggerRedraw();

    //=========================================================================================================
    /**
     * Publishes event to notify that event groups have been changed
     */
    void onEventsUpdated();

    //=========================================================================================================
    /**
     * Publishes event to force FiffRawView to jump to selected annoation
     */
    void onJumpToSelected();

    //=========================================================================================================
    /**
     * Sends event to trigger loading bar to appear and sMessage to show
     *
     * @param[in] sMessage     loading bar message.
     */
    void triggerLoadingStart(const QString& sMessage);

    //=========================================================================================================
    /**
     * Sends event to hide loading bar
     */
    void triggerLoadingEnd(const QString& sMessage);

    QSharedPointer<ANSHAREDLIB::Communicator>                     m_pCommu;                   /**< To broadcst signals. */

signals:
    void newEventAvailable(int iEvent);
    void disconnectFromModel();
    void newEventModelAvailable(QSharedPointer<ANSHAREDLIB::EventModel> pAnnotModel);
    void newFiffRawViewModel(QSharedPointer<ANSHAREDLIB::FiffRawViewModel> pFiffRawModel);
    void clearView(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel);
};

} // NAMESPACE

#endif // ANALYZE_EVENTS_H
