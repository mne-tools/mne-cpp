//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     abstractplugin.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.6
 * @date     October, 2020
 * @brief    Contains declaration of AbstractPlugin class.
 */

#ifndef ABSTRACTPLUGIN_H
#define ABSTRACTPLUGIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"

#include <disp/viewers/abstractview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QMenu>
#include <QDockWidget>
#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class AnalyzeData;
class Event;

//=========================================================================================================
/**
 * DECLARE CLASS AbstractPlugin
 *
 * @brief The AbstractPlugin class is the base interface class for all plugins.
 */
class ANSHAREDSHARED_EXPORT AbstractPlugin : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<AbstractPlugin> SPtr;               /**< Shared pointer type for AbstractPlugin. */
    typedef QSharedPointer<const AbstractPlugin> ConstSPtr;    /**< Const shared pointer type for AbstractPlugin. */

    //=========================================================================================================
    /**
     * Contructor of an AbstractPlugin object.
     */
    AbstractPlugin();

    //=========================================================================================================
    /**
     * Destroys the plugin.
     */
    virtual ~AbstractPlugin();

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<AbstractPlugin> clone() const = 0;

    //=========================================================================================================
    /**
     * Initializes the plugin.
     */
    virtual void init() = 0;

    //=========================================================================================================
    /**
     * Is called when plugin unloaded.
     */
    virtual void unload() = 0;

    //=========================================================================================================
    /**
     * Returns the plugin name.
     * Pure virtual method.
     *
     * @return the name of plugin.
     */
    virtual QString getName() const = 0;

    //=========================================================================================================
    /**
     * Provides the menu, in case no menu is provided it returns a Q_NULLPTR
     *
     * @return the menu.
     */
    virtual QMenu* getMenu() = 0;

    //=========================================================================================================
    /**
     * Provides the control, in case no control is provided it returns a Q_NULLPTR
     *
     * @return the control.
     */
    virtual QDockWidget* getControl() = 0;

    //=========================================================================================================
    /**
     * Provides the view, in case no view is provided it returns a Q_NULLPTR
     *
     * @return the view.
     */
    virtual QWidget* getView() = 0;

    //=========================================================================================================
    /**
     * Informs the EventManager about all Events that the Plugin wants to know about. Can return an empty
     * vector in case no Events need to be seen by the Plugin.
     *
     * @return The vector of relevant Events.
     */
    virtual QVector<EVENT_TYPE> getEventSubscriptions(void) const = 0;

    //=========================================================================================================
    /**
     * Called by the EventManager in case a subscribed-for Event has happened.
     *
     * @param e The Event that has taken place
     */
    virtual void handleEvent(QSharedPointer<Event> e) = 0;

    //=========================================================================================================
    /**
     * Returns string with plugin build date and time.
     *
     * @return build date and time
     */
    virtual QString getBuildInfo() = 0;

    //=========================================================================================================
    /**
     * Initializes the plugin based on cmd line inputs given by the user.
     *
     * @param[in] sArguments  The cmd line arguments.
     */
    virtual void cmdLineStartup(const QStringList& sArguments);

    //=========================================================================================================
    /**
     * Set the order hint for the GUI of MNE Analyze to decide in which order the menu for this plugin will appear.
     * @return order hint for the menu position.
     */
    virtual int getOrder() const;

    //=========================================================================================================
    /**
     * Set the order hint for the GUI of MNE Analyze to decide in which order the menu for this plugin will appear.
     *
     * @param order hint for the menu position.
     */
    virtual void setOrder(int order);

    //=========================================================================================================
    /**
     * Sets the global data, which provides the central database.
     *
     * @param e The Event that has taken place.
     */
    virtual void setGlobalData(QSharedPointer<AnalyzeData> globalData);

    //=========================================================================================================
    /**
     * Set whether this plugin has been initialised or not.
     *
     * @param b The initialization status.
     */
    void setInitState(bool b);
    //=========================================================================================================
    /**
     * Get the initialization state of the plugin.
     *
     */
    bool hasBeenInitialized() const;

    //=========================================================================================================
    /**
     * Retrieve if the plugin Menu has already been loaded.
     * @return state of the menu loading.
     */
    bool menuAlreadyLoaded() const;

    //=========================================================================================================
    /**
     * Retrieve if the plugin View has already been loaded.
     * @return state of the view loading.
     */
    bool viewAlreadyLoaded() const;

    //=========================================================================================================
    /**
     * Retrieve if the plugin Control has already been loaded.
     * @return state of the control loading.
     */
    bool controlAlreadyLoaded() const;

    //=========================================================================================================
    /**
     * Set the loading state of the view menus for thisplugins.
     * @param b new loading state.
     */
    void setViewLoadingState(bool b);

    //=========================================================================================================
    /**
     * Set the loading state of the control menus for this plugin.
     * @param b new loading state.
     */
    void setControlLoadingState(bool b);

    //=========================================================================================================
    /**
     * Set the loading state of the menus for this plugin.
     * @param b new loading state.
     */
    void setMenuLoadingState(bool b);

signals:
    //=========================================================================================================
    /**
     * Signal emmited whenever the gui modes changes
     *
     * @param[in] mode       the new gui mode.
     */
    void guiModeChanged(DISPLIB::AbstractView::GuiMode mode);

    //=========================================================================================================
    /**
     * Signal emmited whenever the gui styles changes
     *
     * @param[in] style       the new gui style.
     */
    void guiStyleChanged(DISPLIB::AbstractView::StyleMode style);

protected:
    QSharedPointer<AnalyzeData>     m_pAnalyzeData;         /**< Pointer to the global data base */
    bool m_bInitialized;                                    /**< Store the initialization state of the plugin. */
    bool m_bMenuAlreadyLoaded;                              /**< Store if the plugin view has already been docked into the GUI. */
    bool m_bViewAlreadyLoaded;                              /**< Store if the plugin view has already been docked into the GUI. */
    bool m_bControlAlreadyLoaded;                           /**< Store if the plugin control has already been docked into the GUI. */
    int m_iOrder;                                           /**< Hint to order the control in the list of controls. */

};

} //Namespace

Q_DECLARE_INTERFACE(ANSHAREDLIB::AbstractPlugin, "ansharedlib/1.0")

#endif //ABSTRACTPLUGIN_H
