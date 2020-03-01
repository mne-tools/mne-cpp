//=============================================================================================================
/**
 * @file     mainwindow.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the MainWindow class.
 *
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "info.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QMap>
#include <QVector>
#include <QString>
#include <QVBoxLayout>
#include <QContextMenuEvent>
#include <QSharedPointer>
#include <QShortcut>
#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;//switch between log levels
class QMenu;
class QToolBar;
class QLabel;
class QTimer;
class QTime;
class QDockWidget;
class QTextBrowser;
QT_END_NAMESPACE

namespace SCSHAREDLIB
{
    class IPlugin;
    class PluginManager;
    class PluginSceneManager;
    class PluginConnectorConnection;
    class DisplayManager;
}

namespace DISPLIB
{
    class MultiView;
    class QuickControlView;
}

//=============================================================================================================
// DEFINE NAMESPACE MNESCAN
//=============================================================================================================

namespace MNESCAN
{

//=============================================================================================================
// MNESCAN FORWARD DECLARATIONS
//=============================================================================================================

class StartUpWidget;
class PluginGui;
class RunWidget;
class PluginDockWidget;

//=============================================================================================================
/**
 * DECLARE CLASS MainWindow
 *
 * @brief The MainWindow class provides the main application user interface.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    typedef QSharedPointer<MainWindow> SPtr;               /**< Shared pointer type for MainWindow. */
    typedef QSharedPointer<const MainWindow> ConstSPtr;    /**< Const shared pointer type for MainWindow. */

    //=========================================================================================================
    /**
     * Constructs a MainWindow which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is Q_NULLPTR, the new MainWindow becomes a window. If parent is another widget, MainWindow becomes a child window inside parent. MainWindow is deleted when its parent is deleted.
     */
    MainWindow(QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destroys the MainWindow.
     * All MainWindow's children are deleted first. The application exits if MainWindow is the main widget.
     */
    virtual ~MainWindow();

    //=========================================================================================================
    /**
     * Garbage collection
     */
    void clear();

    //=========================================================================================================
    /**
     * This event handler is called when Qt receives a window close request from the window system.
     *
     * @param [in] event     close event
     */
    void closeEvent(QCloseEvent *event);

    //=========================================================================================================
    /**
     * Starts or restarts the main application timer with a timeout interval of msec milliseconds.
     *
     * @param [in] msec timeout interval in milliseconds
     */
    void startTimer(int msec);

    //=========================================================================================================
    /**
     * Stops the main application timer.
     */
    void stopTimer();

    //=========================================================================================================
    /**
     * Writes to MainWindow log.
     *
     * @param [in] logMsg message
     * @param [in] lgknd message kind; Message is formated depending on its kind.
     * @param [in] lglvl message level; Message is displayed depending on its level.
     */
    void writeToLog(const QString& logMsg, LogKind lgknd = _LogKndMessage, LogLevel lglvl = _LogLvNormal);

private:
    //=========================================================================================================
    /**
     * Setup the user interface for running mode.
     *
     * @param [in] state whether program is running.
     */
    void uiSetupRunningState(bool state);

    void createActions();       /**< Creates all actions for user interface of MainWindow class. */
    void createMenus();         /**< Creates all menus for user interface of MainWindow class. */
    void createToolBars();      /**< Creates all tool bars for user interface of MainWindow class. */
    void initStatusBar();       /**< Creates QToolBar for user interface of MainWindow class. */
    void createPluginDockWindow();                          /**< Creates plugin dock widget.*/
    void createLogDockWindow();                             /**< Creates log dock widget.*/
    void updatePluginSetupWidget(QSharedPointer<SCSHAREDLIB::IPlugin> pPlugin);       /**< Sets the plugin setup widget to central widget of MainWindow class depending on the current plugin selected in m_pDockWidgetPlugins.*/
    void initMultiViewWidget(QList<QSharedPointer<SCSHAREDLIB::IPlugin> > lPlugins);         /**< Adds the plugin visualization widget to central widget of MainWindow class if the pipeline was started.*/
    void onControlWidgetsChanged(QList<QWidget*>& lControlWidgets,
                                 const QString& sPluginName);
    void updateConnectionWidget(QSharedPointer<SCSHAREDLIB::PluginConnectorConnection> pConnection); /**< Sets the connection widget to central widget of MainWindow class depending on the current arrow selected in m_pDockWidgetPlugins.*/
    void newConfiguration();            /**< Implements new configuration tasks.*/
    void openConfiguration();           /**< Implements open configuration tasks.*/
    void saveConfiguration();           /**< Implements save configuration tasks.*/
    void helpContents();                /**< Implements help contents action.*/
    void about();                       /**< Implements about action.*/
    void setMinLogLevel();              /**< Sets minimal log level as current log level.*/
    void setNormalLogLevel();           /**< Sets normal log level as current log level.*/
    void setMaxLogLevel();              /**< Sets maximal log level as current log level.*/
    void startMeasurement();            /**< Runs application.*/
    void stopMeasurement();             /**< Stops application.*/
    void updateTime();                  /**< Updates m_pTime and is called through timeout() of m_pTimer.*/

    StartUpWidget* m_pStartUpWidget;    /**< holds the StartUpWidget.*/

    bool m_bIsRunning;                  /**< whether program/plugins is/are started.*/

    DISPLIB::MultiView* m_pRunWidget;                            /**< The run widget */
    QShortcut* m_pRunWidgetClose;                       /**< Run widget close shortcut */
    QSharedPointer<SCSHAREDLIB::DisplayManager> m_pDisplayManager;   /**< display manager */

    QAction*                            m_pActionNewConfig;         /**< new configuration */
    QAction*                            m_pActionOpenConfig;        /**< open configuration */
    QAction*                            m_pActionSaveConfig;        /**< save configuration */
    QAction*                            m_pActionExit;              /**< exit application */

    QActionGroup*                       m_pActionGroupLgLv;         /**< group log level */
    QAction*                            m_pActionMinLgLv;           /**< set minimal log level */
    QAction*                            m_pActionNormLgLv;          /**< set normal log level */
    QAction*                            m_pActionMaxLgLv;           /**< set maximal log level */

    QAction*                            m_pActionHelpContents;      /**< open help contents */
    QAction*                            m_pActionAbout;             /**< show about dialog */
    QPointer<QAction>                   m_pActionQuickControl;      /**< Show quick control widget. */

    QAction*                            m_pActionRun;               /**< run application */
    QAction*                            m_pActionStop;              /**< stop application */

    QList< QAction* >                   m_qListDynamicPluginActions;    /**< dynamic plugin actions */
    QList< QAction* >                   m_qListDynamicDisplayActions;   /**< dynamic display actions */
    QList< QWidget* >                   m_qListDynamicControlWidgets;   /**< dynamic display widgets */

    QMenu*                              m_pMenuFile;    /**< Holds the file menu.*/
    QMenu*                              m_pMenuView;    /**< Holds the view menu.*/
    QMenu*                              m_pMenuLgLv;    /**< Holds the log level sub menu.*/
    QMenu*                              m_pMenuHelp;    /**< Holds the help menu.*/

    QToolBar*                           m_pToolBar;                 /**< Holds the tool bar.*/
    QToolBar*                           m_pDynamicPluginToolBar;    /**< Holds the plugin tool bar.*/

    QLabel*                             m_pLabelTime;      /**< Holds the display label for the running time.*/
    QSharedPointer<QTimer>              m_pTimer;           /**< timer of the main application*/
    QSharedPointer<QTime>               m_pTime;            /**< Holds current time output, updated with timeout of timer.*/
    int                                 m_iTimeoutMSec;     /**< Holds milliseconds after which timer timeouts.*/

    QDockWidget*                        m_pPluginGuiDockWidget;         /**< Dock widget which holds the plugin gui. */
    PluginGui*                          m_pPluginGui;
    QSharedPointer<SCSHAREDLIB::PluginManager>       m_pPluginManager;       /**< Holds log dock widget.*/
    QSharedPointer<SCSHAREDLIB::PluginSceneManager>  m_pPluginSceneManager;  /**< Plugin scene manager which manages the plugin graph */

    QDockWidget*                        m_pDockWidget_Log;              /**< Holds the dock widget containing the log.*/
    QTextBrowser*                       m_pTextBrowser_Log;             /**< Holds the text browser for the log.*/

    LogLevel                            m_eLogLevelCurrent;             /**< Holds the current log level.*/

    QSharedPointer<QWidget>             m_pAboutWindow;                 /**< Holds the widget containing the about information.*/
    QSharedPointer<DISPLIB::QuickControlView> m_pQuickControlView;            /**< quick control widget. */

};
}// NAMESPACE

#endif // MAINWINDOW_H
