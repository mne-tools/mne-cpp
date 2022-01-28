//=============================================================================================================
/**
 * @file     mainwindow.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel B Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch, Gabriel B Motta. All rights reserved.
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
#include "mainsplashscreen.h"
#include "mainsplashscreencloser.h"

#include <disp/viewers/abstractview.h>

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
    class AbstractPlugin;
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
     * @param[in] parent    pointer to parent widget; If parent is Q_NULLPTR, the new MainWindow becomes
     *                      a window. If parent is another widget, MainWindow becomes a child window inside
     *                      parent. MainWindow is deleted when its parent is deleted.
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
     * This event handler is called when Qt receives a window close request from the window system.
     *
     * @param[in] event     close event.
     */
    void closeEvent(QCloseEvent *event);

    //=========================================================================================================
    /**
     * Starts or restarts the main application timer with a timeout interval of msec milliseconds.
     *
     * @param[in] msec timeout interval in milliseconds.
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
     * @param[in] logMsg message.
     * @param[in] lgknd message kind; Message is formated depending on its kind.
     * @param[in] lglvl message level; Message is displayed depending on its level.
     */
    void writeToLog(const QString& logMsg,
                    LogKind lgknd = _LogKndMessage,
                    LogLevel lglvl = _LogLvNormal);

    //=========================================================================================================
    /**
     * Initialize the splash screen. By default, the slpash screen will be shown to the user.
     *
     */
    void initSplashScreen();

    //=========================================================================================================
    /**
     * Initialize the splash screen.
     *
     * @param[in] bShowSplashScreen    Whether to show the splash screen until this widget is shown. Default is true.
     */
    void initSplashScreen(bool bShowSplashScreen);

    //=========================================================================================================
    /**
     * Public function to hide Main applications splash screen.
     */
    void hideSplashScreen();

    //=========================================================================================================
    /**
     * Init an setup the plugins.
     */
    void setupPlugins(std::shared_ptr<SCSHAREDLIB::PluginManager>,
                      std::shared_ptr<SCSHAREDLIB::PluginSceneManager>);

    //=========================================================================================================
    /**
     * Setup the GUI of this widget.
     */
    void setupUI();

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

private:
    //=========================================================================================================
    /**
     * Changes the current layout style of the application.
     *
     * @param[in] sStyle   The new qss style.
     */
    void onStyleChanged(const QString& sStyle);

    //=========================================================================================================
    /**
     * Handles changes made to the application's GUI mode.
     */
    void onGuiModeChanged();

    //=========================================================================================================
    /**
     * Setup the user interface for running mode.
     *
     * @param[in] state whether program is running.
     */
    void uiSetupRunningState(bool state);

    //=========================================================================================================
    /**
     * Creates all actions for user interface of MainWindow class.
     */
    void createActions();

    //=========================================================================================================
    /**
     * Creates all menus for user interface of MainWindow class.
     */
    void createMenus();

    //=========================================================================================================
    /**
     * Creates all tool bars for user interface of MainWindow class.
     */
    void createToolBars();

    //=========================================================================================================
    /**
     * Creates QToolBar for user interface of MainWindow class.
     */
    void initStatusBar();

    //=========================================================================================================
    /**
     * Creates plugin dock widget.
     */
    void createPluginDockWindow();

    //=========================================================================================================
    /**
     * Creates log dock widget.
     */
    void createLogDockWindow();

    //=========================================================================================================
    /**
     * Sets the plugin setup widget to central widget of MainWindow class depending on the current plugin
     * selected in m_pDockWidgetPlugins.
     */
    void updatePluginSetupWidget(QSharedPointer<SCSHAREDLIB::AbstractPlugin> pPlugin);

    //=========================================================================================================
    /**
     * Adds the plugin visualization widget to central widget of MainWindow class if the pipeline was started.
     */
    void initMultiViewWidget(QList<QSharedPointer<SCSHAREDLIB::AbstractPlugin> > lPlugins);

    //=========================================================================================================
    /**
     * Process the event of a widget changing its location.
     */
    void onDockLocationChanged(QWidget* pWidget);

    //=========================================================================================================
    /**
     * Adds plugin control widgets to the QuickControlView.
     */
    void onPluginControlWidgetsChanged(QList<QWidget*>& lControlWidgets,
                                       const QString& sPluginName);

    //=========================================================================================================
    /**
     * Adds display control widgets to the QuickControlView.
     */
    void onDisplayControlWidgetsChanged(QList<QWidget*>& lControlWidgets,
                                        const QString& sPluginName);

    //=========================================================================================================
    /**
     * Sets the connection widget to central widget of MainWindow class depending on the current arrow selected
     * in m_pDockWidgetPlugins.
     */
    void updateConnectionWidget(QSharedPointer<SCSHAREDLIB::PluginConnectorConnection> pConnection);

    //=========================================================================================================
    /**
     * Implements new configuration tasks.
     */
    void newConfiguration();

    //=========================================================================================================
    /**
     * Implements open configuration tasks.
     */
    void openConfiguration();

    //=========================================================================================================
    /**
     * Implements save configuration tasks.
     */
    void saveConfiguration();

    //=========================================================================================================
    /**
     * Implements help contents action.
     */
    void helpContents();

    //=========================================================================================================
    /**
     * Implements about action.
     */
    void about();

    //=========================================================================================================
    /**
     * Sets minimal log level as current log level.
     */
    void setMinLogLevel();

    //=========================================================================================================
    /**
     * Sets normal log level as current log level.
     */
    void setNormalLogLevel();

    //=========================================================================================================
    /**
     * Sets maximal log level as current log level.
     */
    void setMaxLogLevel();

    //=========================================================================================================
    /**
     * Runs the created plugin pipeline.
     */
    void startMeasurement();

    //=========================================================================================================
    /**
     * Stops the created plugin pipeline.
     */
    void stopMeasurement();

    //=========================================================================================================
    /**
     * Updates m_pTime and is called through timeout() of m_pTimer.
     */
    void updateTime();

    bool m_bIsRunning;                  /**< whether program/plugins is/are started.*/

    int                                 m_iTimeoutMSec;                 /**< Holds milliseconds after which timer timeouts.*/

    QPointer<QActionGroup>              m_pActionGroupLgLv;             /**< group log level. */
    QPointer<QActionGroup>              m_pActionStyleGroup;            /**< group for styles. */
    QPointer<QActionGroup>              m_pActionModeGroup;             /**< group for gui modes. */

    QPointer<QAction>                   m_pActionNewConfig;             /**< new configuration. */
    QPointer<QAction>                   m_pActionOpenConfig;            /**< open configuration. */
    QPointer<QAction>                   m_pActionSaveConfig;            /**< save configuration. */
    QPointer<QAction>                   m_pActionExit;                  /**< exit application. */
    QPointer<QAction>                   m_pActionMinLgLv;               /**< set minimal log level. */
    QPointer<QAction>                   m_pActionNormLgLv;              /**< set normal log level. */
    QPointer<QAction>                   m_pActionMaxLgLv;               /**< set maximal log level. */
    QPointer<QAction>                   m_pActionDarkMode;              /**< toggle dark mode. */
    QPointer<QAction>                   m_pActionHelpContents;          /**< open help contents. */
    QPointer<QAction>                   m_pActionAbout;                 /**< show about dialog. */
    QPointer<QAction>                   m_pActionQuickControl;          /**< Show quick control widget. */
    QPointer<QAction>                   m_pActionRun;                   /**< run application. */
    QPointer<QAction>                   m_pActionStop;                  /**< stop application. */
    QPointer<QAction>                   m_pActionDefaultMode;           /**< stop application. */
    QPointer<QAction>                   m_pActionResearchMode;          /**< activate research gui mode. */
    QPointer<QAction>                   m_pActionClinicalMode;          /**< activate clinical gui mode. */

    QList<QAction*>                     m_qListDynamicPluginActions;    /**< dynamic plugin actions. */
    QList<QAction*>                     m_qListDynamicDisplayActions;   /**< dynamic display actions. */
    QList<QAction*>                     m_qListDynamicDisplayMenuActions;/**< dynamic display actions for the menu. */

    QPointer<QMenu>                     m_pMenuFile;                    /**< Holds the file menu.*/
    QPointer<QMenu>                     m_pMenuView;                    /**< Holds the view menu.*/
    QPointer<QMenu>                     m_pMenuLgLv;                    /**< Holds the log level sub menu.*/
    QPointer<QMenu>                     m_pMenuHelp;                    /**< Holds the help menu.*/
    QPointer<QMenu>                     m_pMenuAppearance;              /**< Holds the help menu.*/

    QPointer<QDockWidget>               m_pPluginGuiDockWidget;         /**< Dock widget which holds the plugin gui. */
    QPointer<QDockWidget>               m_pDockWidget_Log;              /**< Holds the dock widget containing the log.*/

    QPointer<QToolBar>                  m_pToolBar;                     /**< Holds the tool bar.*/
    QPointer<QToolBar>                  m_pDynamicPluginToolBar;        /**< Holds the plugin tool bar.*/

    QPointer<QLabel>                    m_pLabelTime;                   /**< Holds the display label for the running time.*/

    QPointer<QTextBrowser>              m_pTextBrowser_Log;             /**< Holds the text browser for the log.*/

    QPointer<StartUpWidget>             m_pStartUpWidget;               /**< holds the StartUpWidget.*/

    QPointer<DISPLIB::MultiView>        m_pMultiView;                   /**< The multi view widget, which is set as central widget as soon as the measurement starts. */

    LogLevel                            m_eLogLevelCurrent;             /**< Holds the current log level.*/

    QPointer<PluginGui>                 m_pPluginGui;                   /**< Holds the plugin GUI.*/

    QPointer<DISPLIB::QuickControlView> m_pQuickControlView;            /**< quick control widget. */

    MainSplashScreenCloser::SPtr         m_pSplashScreenHider;           /**< Holds the object responsible of hiding the splashscreen. */
    MainSplashScreen::SPtr              m_pSplashScreen;                /**< Holds the splash screen. */

    QSharedPointer<QTimer>                              m_pTimer;               /**< timer of the main application*/
    QSharedPointer<QTime>                               m_pTime;                /**< Holds current time output, updated with timeout of timer.*/
    std::shared_ptr<SCSHAREDLIB::PluginManager>         m_pPluginManager;       /**< Holds log dock widget.*/
    std::shared_ptr<SCSHAREDLIB::PluginSceneManager>    m_pPluginSceneManager;  /**< Plugin scene manager which manages the plugin graph. */
    QSharedPointer<QWidget>                             m_pAboutWindow;         /**< Holds the widget containing the about information.*/
    QSharedPointer<SCSHAREDLIB::DisplayManager>         m_pDisplayManager;      /**< display manager. */

    QString                             m_sSettingsPath;                    /**< The settings path to store the GUI settings to. */
    QString                             m_sCurrentStyle;                    /**< The currently selected style (dark mode, default mode). */

signals:
    //=========================================================================================================
    /**
     * Signal emmited whenever the gui modes changed
     *
     * @param[in] mode       the new gui mode.
     */
    void guiModeChanged(DISPLIB::AbstractView::GuiMode mode);

};
}// NAMESPACE

#endif // MAINWINDOW_H
