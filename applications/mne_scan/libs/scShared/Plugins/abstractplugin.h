//=============================================================================================================
/**
 * @file     abstractplugin.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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

#ifndef MNESCAN_ABSTRACTPLUGIN_H
#define MNESCAN_ABSTRACTPLUGIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"

#include "../Management/pluginoutputdata.h"
#include "../Management/plugininputdata.h"

#include <disp/viewers/abstractview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QCoreApplication>
#include <QSharedPointer>
#include <QAction>

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

//=============================================================================================================
// SCSHAREDLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS AbstractPlugin
 *
 * @brief The AbstractPlugin class is the base interface class of all plugins.
 */
class SCSHAREDSHARED_EXPORT AbstractPlugin : public QThread
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Plugin Type enumeration.
     */
    enum PluginType
    {
        _ISensor,       /**< Type for a sensor plugin. */
        _IAlgorithm,    /**< Type for a real-time algorithm plugin. */
        _PluginSet      /**< Type for a plugin set which holds different types of plugins. */
    };

    typedef QSharedPointer<AbstractPlugin> SPtr;               /**< Shared pointer type for AbstractPlugin. */
    typedef QSharedPointer<const AbstractPlugin> ConstSPtr;    /**< Const shared pointer type for AbstractPlugin. */

    typedef QVector< QSharedPointer< PluginInputConnector > > InputConnectorList;  /**< List of input connectors. */
    typedef QVector< QSharedPointer< PluginOutputConnector > > OutputConnectorList; /**< List of output connectors. */

    //=========================================================================================================
    /**
     * Destroys the AbstractPlugin.
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
     * Is called when plugin is detached of the stage. Can be used to safe settings.
     */
    virtual void unload() = 0;// = 0 call is not longer possible - it has to be reimplemented in child;

    //=========================================================================================================
    /**
     * Starts the AbstractPlugin.
     * Pure virtual method.
     *
     * @return true if success, false otherwise.
     */
    virtual bool start() = 0;

    //=========================================================================================================
    /**
     * Stops the AbstractPlugin.
     * Pure virtual method.
     *
     * @return true if success, false otherwise.
     */
    virtual bool stop() = 0;

    //=========================================================================================================
    /**
     * A list of actions for the current plugin.
     *
     * @return a list of plugin actions.
     */
    inline QList< QAction* > getPluginActions();

    //=========================================================================================================
    /**
     * Returns the plugin type.
     * Pure virtual method.
     *
     * @return type of the AbstractPlugin.
     */
    virtual PluginType getType() const = 0;

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
     * True if multi instantiation of plugin is allowed.
     *
     * @return true if multi instantiation of plugin is allowed.
     */
    virtual inline bool multiInstanceAllowed() const = 0;

    //=========================================================================================================
    /**
     * Returns the set up widget for configuration of the AbstractPlugin.
     * Pure virtual method.
     *
     * @return the setup widget.
     */
    virtual QWidget* setupWidget() = 0; //setup()

    //=========================================================================================================
    /**
     * Returns string with plugin build date and time.
     *
     * @return build date and time
     */
    virtual QString getBuildDateTime() = 0;

    inline InputConnectorList& getInputConnectors(){return m_inputConnectors;}
    inline OutputConnectorList& getOutputConnectors(){return m_outputConnectors;}

signals:
    //=========================================================================================================
    /**
     * Signal to notify that new plugin control widgets are available.
     *
     * @param[in] lControlWidgets      A QList with pointers to the control widgets. Note that the signal sender.
     *                                  does not have ownership of these pointers.
     * @param[in] sPluginName          The plugin name emmiting the signal.
     */
    void pluginControlWidgetsChanged(QList<QWidget*>& lControlWidgets,
                                     const QString& sPluginName);

    //=========================================================================================================
    /**
     * Signal emmited whenever the gui modes changed
     *
     * @param[in] mode       the new gui mode.
     */
    void guiModeChanged(DISPLIB::AbstractView::GuiMode mode);

protected:
    //=========================================================================================================
    /**
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread
     */
    virtual void run() = 0;

    //=========================================================================================================
    /**
     * Adds a plugin action to the current plugin.
     *
     * @param[in] pAction  pointer to the action to be added to the plugin.
     */
    inline void addPluginAction(QAction* pAction);

    InputConnectorList m_inputConnectors;       /**< Set of input connectors associated with this plug-in. */
    OutputConnectorList m_outputConnectors;     /**< Set of output connectors associated with this plug-in. */

    bool m_bPluginControlWidgetsInit = false;   /**< Flag to indicate if the plugin control widgets were initialized already. */

private:
    QList< QAction* >   m_qListPluginActions;  /**< List of plugin actions. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool AbstractPlugin::multiInstanceAllowed() const
{
    return true;
}

//=============================================================================================================

inline QList< QAction* > AbstractPlugin::getPluginActions()
{
    return m_qListPluginActions;
}

//=============================================================================================================

inline void AbstractPlugin::addPluginAction(QAction* pAction)
{
    m_qListPluginActions.append(pAction);
}

//=============================================================================================================

//inline void AbstractPlugin::addPluginWidget(QWidget* pWidget)
//{
//    m_qListPluginWidgets.append(pWidget);
//}
} //Namespace

Q_DECLARE_INTERFACE(SCSHAREDLIB::AbstractPlugin, "scsharedlib/1.0")

#endif //MNESCAN_ABSTRACTPLUGIN_H
