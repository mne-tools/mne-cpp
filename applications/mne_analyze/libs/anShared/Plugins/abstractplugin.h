//=============================================================================================================
/**
 * @file     abstractplugin.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.6
 * @date     October, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief    Contains declaration of AbstractPlugin class.
 *
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
     * Destroys the plugin.
     */
    virtual ~AbstractPlugin() {}

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
     * @return the menu
     */
    virtual QMenu* getMenu() = 0;

    //=========================================================================================================
    /**
     * Provides the control, in case no control is provided it returns a Q_NULLPTR
     *
     * @return the control
     */
    virtual QDockWidget* getControl() = 0;

    //=========================================================================================================
    /**
     * Provides the view, in case no view is provided it returns a Q_NULLPTR
     *
     * @return the view
     */
    virtual QWidget* getView() = 0;

    //=========================================================================================================
    /**
     * Informs the EventManager about all Events that the Plugin wants to know about. Can return an empty
     * vector in case no Events need to be seen by the Plugin.
     *
     * @return The vector of relevant Events
     */
    virtual QVector<EVENT_TYPE> getEventSubscriptions(void) const = 0;

    //=========================================================================================================
    /**
     * Initializes the plugin based on cmd line inputs given by the user.
     *
     * @param[in] sArguments  the cmd line arguments
     */
    virtual inline void cmdLineStartup(const QStringList& sArguments);

    //=========================================================================================================
    /**
     * Sets the global data, which provides the central database.
     *
     * @param[in] globalData  the global data
     */
    virtual inline void setGlobalData(QSharedPointer<AnalyzeData> globalData);

    //=========================================================================================================
    /**
     * Called by the EventManager in case a subscribed-for Event has happened.
     *
     * @param e The Event that has taken place
     */
    virtual void handleEvent(QSharedPointer<Event> e) = 0;

signals:
    //=========================================================================================================
    /**
     * Signal emmited whenever the gui modes changed
     *
     * @param [in] mode       the new gui mode
     */
    void guiModeChanged(DISPLIB::AbstractView::GuiMode mode);

protected:
    QSharedPointer<AnalyzeData>     m_pAnalyzeData;         /**< Pointer to the global data base */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

void AbstractPlugin::cmdLineStartup(const QStringList& sArguments)
{
    Q_UNUSED(sArguments)
}

//=============================================================================================================

void AbstractPlugin::setGlobalData(QSharedPointer<AnalyzeData> globalData)
{
    m_pAnalyzeData = globalData;
}

} //Namespace

Q_DECLARE_INTERFACE(ANSHAREDLIB::AbstractPlugin, "ansharedlib/1.0")

#endif //ABSTRACTPLUGIN_H
