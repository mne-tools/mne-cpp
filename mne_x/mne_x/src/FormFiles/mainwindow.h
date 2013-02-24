//=============================================================================================================
/**
* @file		mainwindow.h
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the declaration of the MainWindow class.
*
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../preferences/info.h"
#include "../../../comp/rtmeas/Nomenclature/nomenclature.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QMap>
#include <QVector>
#include <QString>
#include <QVBoxLayout>
#include <QContextMenuEvent>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QAction;
class QActionGroup;//switch between log levels
class QMenu;
class QToolBar;
class QLabel;
class QTimer;
class QTime;
class QDockWidget;
class QTextBrowser;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS OF NAMESPACE CSART
//=============================================================================================================

class StartUpWidget;

class ModuleManager;

class DisplayManager;

class RunWidget;
class ModuleDockWidget;


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

    //=========================================================================================================
    /**
    * Constructs a MainWindow which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new MainWindow becomes a window. If parent is another widget, MainWindow becomes a child window inside parent. MainWindow is deleted when its parent is deleted.
    */
    MainWindow(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the MainWindow.
    * All MainWindow's children are deleted first. The application exits if MainWindow is the main widget.
    */
    virtual ~MainWindow();

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

signals:
    //=========================================================================================================
    /**
    * This signal is emitted when a new log message of class MainWindow is available.
    *
    * @param [in] logMsg message
    * @param [in] lgknd message kind
    * @param [in] lglvl message level
    */
    void newLogMsg(const QString& logMsg, LogKind lgknd = _LogKndMessage, LogLevel lglvl = _LogLvNormal);

public slots:
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
    //StartUp
    StartUpWidget*    m_pStartUpWidget;		/**< holds the StartUpWidget.*/

    //Run
    RunWidget*                          m_pRunWidget;	/**< Holds the run widget.*/
    bool                                m_bDisplayMax;	/**< whether full screen mode is activated.*/
    bool                                m_bIsRunning;	/**< whether program/modules is/are started.*/

    //=========================================================================================================
    /**
    * Setup the user interface for running mode.
    *
    * @param [in] state whether program is running.
    */
    void uiSetupRunningState(bool state);

    /**
     * \defgroup Class MainWindow user interface
     */
    /*@{*/
    void createActions();		/**< Creates all actions for user interface of MainWindow class. */
    void createMenus(); 		/**< Creates all menus for user interface of MainWindow class. */
    void createToolBars();		/**< Creates all tool bars for user interface of MainWindow class. */

    void initStatusBar();		/**< Creates QToolBar for user interface of MainWindow class. */

    QAction*                             m_pActionNewConfig;		/**< new configuration */
    QAction*                             m_pActionOpenConfig;		/**< open configuration */
    QAction*                             m_pActionSaveConfig;		/**< save configuration */
    QAction*                             m_pActionExit;				/**< exit application */

    QActionGroup*                        m_pActionGroupLgLv;		/**< group log level */
    QAction*                             m_pActionMinLgLv;			/**< set minimal log level */
    QAction*                             m_pActionNormLgLv;			/**< set normal log level */
    QAction*                             m_pActionMaxLgLv;			/**< set maximal log level */

    QAction*                             m_pActionHelpContents;		/**< open help contents */
    QAction*                             m_pActionAbout;			/**< show about dialog */

    QAction*                             m_pActionRun;				/**< run application */
    QAction*                             m_pActionStop;				/**< stop application */
    QAction*                             m_pActionZoomStd;			/**< standard zoom */
    QAction*                             m_pActionZoomIn;			/**< zoom in */
    QAction*                             m_pActionZoomOut;			/**< zoom out */
    QAction*                             m_pActionDisplayMax;		/**< show full screen mode */

    QAction*                             m_pActionDebugDisconnect;	/**< debug action -> for debug purpose */

    //Main Window Menu
    QMenu*                                 m_pMenuFile;		/**< Holds the file menu.*/
    QMenu*                                 m_pMenuView;		/**< Holds the view menu.*/
    QMenu*                                 m_pMenuLgLv;		/**< Holds the log level sub menu.*/
    QMenu*                                 m_pMenuHelp;		/**< Holds the help menu.*/

    QMenu*                                 m_pMenuDebug;	/**< Holds the debug menu.*/

    // Tool bar
    QToolBar*                             m_pToolBar;		/**< Holds the tool bar.*/
    /*@}*/


    QLabel*                             m_pLabel_Time;		/**< Holds the display label for the running time.*/
    QTimer*                             m_pTimer;			/**< timer of the main application*/
    QTime*                              m_pTime;			/**< Holds current time output, updated with timeout of timer.*/
    int                                 m_iTimeoutMSec;		/**< Holds milliseconds after which timer timeouts.*/

    void createModuleDockWindow();										/**< Creates module dock widget.*/
    void createLogDockWindow();											/**< Creates log dock widget.*/

    //Module
    ModuleDockWidget*                    m_pModuleDockWidget;			/**< Holds the dock widget containing the modules.*/
    ModuleManager*                       m_pModuleManager;				/**< Holds log dock widget.*/

    QList<MDL_ID::Module_ID>           m_pListCurrentDisplayModules;	/**< Holds list of module id's which should be displayed.*/

    //Log
    QDockWidget*                         m_pDockWidget_Log;				/**< Holds the dock widget containing the log.*/
    QTextBrowser*                        m_pTextBrowser_Log;			/**< Holds the text browser for the log.*/

    LogLevel                             m_eLogLevelCurrent;			/**< Holds the current log level.*/


private slots:

    void CentralWidgetShowModule();		/**< Sets a widget to central widget of MainWindow class depending on the current module selected in m_pDockWidgetModules.*/

    void newConfiguration();			/**< Implements new configuration tasks.*/
    void openConfiguration();			/**< Implements open configuration tasks.*/
    void saveConfiguration();			/**< Implements save configuration tasks.*/

    void helpContents();				/**< Implements help contents action.*/

    void about();						/**< Implements about action.*/

    void setMinLogLevel();				/**< Sets minimal log level as current log level.*/
    void setNormalLogLevel();			/**< Sets normal log level as current log level.*/
    void setMaxLogLevel();				/**< Sets maximal log level as current log level.*/

    void startRTMeasurement();			/**< Runs application.*/
    void stopRTMeasurement();			/**< Stops application.*/

    void zoomStd(); 					/**< Implements standard of runWidget.*/
    void zoomIn();						/**< Implements zoom in of runWidget.*/
    void zoomOut();						/**< Implements zoom out of runWidget.*/
    void toggleDisplayMax();			/**< Implements show full screen mode of runWidget.*/

    void updateTime();		/**< Updates m_pTime and is called through timeout() of m_pTimer.*/

};

}// NAMESPACE

#endif // MAINWINDOW_H
