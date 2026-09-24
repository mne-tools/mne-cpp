//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     events.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     March, 2021
 * @brief    Contains the declaration of the events class.
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
