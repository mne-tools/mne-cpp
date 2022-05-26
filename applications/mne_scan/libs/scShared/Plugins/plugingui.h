#ifndef PLUGINGUI_H
#define PLUGINGUI_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QMenu>
#include <QWidget>

//=============================================================================================================
/**
 * Used as the interface for returning the GUI for a plugin.
 *
 * Can be used by either passing in items with the 'add' and 'set' functions,
 * or subclassed and reimplementing the 'get' functions.
 */
class PluginGUI
{
public:
    //=========================================================================================================
    /**
     * A list of actions for the current plugin.
     *
     * @return a list of plugin actions.
     */
    inline virtual QList< QAction* > getPluginActions() const;

    //=========================================================================================================
    /**
     * A list of menus for the current plugin.
     *
     * @return a list of plugin actions.
     */
    inline virtual QList< QMenu* > getPluginMenus() const;

    //=========================================================================================================
    /**
     * A list of widgets for the current plugin.
     *
     * @return a list of plugin actions.
     */
    inline virtual QList< QWidget* > getPluginWidgets() const;

    //=========================================================================================================
    /**
     * Adds a plugin action to the current plugin.
     *
     * @param[in] pAction  pointer to the action to be added to the plugin.
     */
    inline void addPluginAction(QAction* pAction);

    //=========================================================================================================
    /**
     * Adds a plugin menu to the current plugin.
     *
     * @param[in] pMenu  pointer to the menu to be added to the plugin.
     */
    inline void addPluginMenu(QMenu* pMenu);

    //=========================================================================================================
    /**
     * Adds a plugin widget to the current plugin.
     *
     * @param[in] pWidget  pointer to the widget to be added to the plugin.
     */
    inline void addPluginWidget(QWidget* pWidget);

    //=========================================================================================================
    /**
     * Returns the set up widget for configuration of the AbstractPlugin.
     *
     * @return the setup widget.
     */
     inline virtual QWidget* getSetupWidget() const;

     //=========================================================================================================
     /**
      * Sets the setup widget.
      */
     inline void setSetupWidget(QWidget* pWidget);


private:
    QList< QAction* >   m_qListPluginActions;  /**< List of plugin actions. */
    QList< QMenu* >     m_qListPluginMenus;    /**< List of plugin menus. */
    QList< QWidget* >   m_qListPluginWidgets;  /**< List of plugin widgets. */

    QWidget*            m_pSetupWidget;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


inline QList< QAction* > PluginGUI::getPluginActions() const
{
    return m_qListPluginActions;
}

//=============================================================================================================

inline void PluginGUI::addPluginAction(QAction* pAction)
{
    m_qListPluginActions.append(pAction);
}

//=============================================================================================================

inline QList< QMenu* > PluginGUI::getPluginMenus() const
{
    return m_qListPluginMenus;
}

//=============================================================================================================

inline void PluginGUI::addPluginMenu(QMenu* pMenu)
{
    m_qListPluginMenus.append(pMenu);
}

//=============================================================================================================

inline QList< QWidget* > PluginGUI::getPluginWidgets() const
{
    return m_qListPluginWidgets;
}

//=============================================================================================================

inline void PluginGUI::addPluginWidget(QWidget* pWidget)
{
    m_qListPluginWidgets.append(pWidget);
}

//=============================================================================================================

inline QWidget* PluginGUI::getSetupWidget() const
{
    return m_pSetupWidget;
}

//=============================================================================================================
/**
 * Sets the setup widget.
 */
inline void PluginGUI::setSetupWidget(QWidget* pWidget)
{
    m_pSetupWidget = pWidget;
}


#endif // PLUGINGUI_H
