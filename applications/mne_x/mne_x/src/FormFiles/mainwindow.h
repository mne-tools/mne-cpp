//=============================================================================================================
/**
* @file     mainwindow.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../preferences/info.h"
#include <xMeas/Nomenclature/nomenclature.h>


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
#include <QSharedPointer>


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

namespace DISPLIB
{
class DisplayManager;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class StartUpWidget;

class PluginManager;

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
    StartUpWidget* m_pStartUpWidget;    /**< holds the StartUpWidget.*/

    //Run
    RunWidget* m_pRunWidget;            /**< Holds the run widget.*/

    bool m_bDisplayMax;                 /**< whether full screen mode is activated.*/
    bool m_bIsRunning;                  /**< whether program/plugins is/are started.*/

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
    QMenu*                                 m_pMenuFile;     /**< Holds the file menu.*/
    QMenu*                                 m_pMenuView;     /**< Holds the view menu.*/
    QMenu*                                 m_pMenuLgLv;     /**< Holds the log level sub menu.*/
    QMenu*                                 m_pMenuHelp;     /**< Holds the help menu.*/

    QMenu*                                 m_pMenuDebug;    /**< Holds the debug menu.*/

    // Tool bar
    QToolBar*                             m_pToolBar;       /**< Holds the tool bar.*/
    /*@}*/


    QLabel*                             m_pLabel_Time;      /**< Holds the display label for the running time.*/
    QSharedPointer<QTimer>              m_pTimer;           /**< timer of the main application*/
    QSharedPointer<QTime>               m_pTime;            /**< Holds current time output, updated with timeout of timer.*/
    int                                 m_iTimeoutMSec;     /**< Holds milliseconds after which timer timeouts.*/

    void createPluginDockWindow();                          /**< Creates plugin dock widget.*/
    void createLogDockWindow();                             /**< Creates log dock widget.*/

    //Plugin
    PluginDockWidget*                   m_pPluginDockWidget;            /**< Holds the dock widget containing the plugins.*/
    PluginManager*                      m_pPluginManager;               /**< Holds log dock widget.*/

    QList<PLG_ID::Plugin_ID>            m_pListCurrentDisplayPlugins;   /**< Holds list of plugin id's which should be displayed.*/

    //Log
    QDockWidget*                        m_pDockWidget_Log;              /**< Holds the dock widget containing the log.*/
    QTextBrowser*                       m_pTextBrowser_Log;             /**< Holds the text browser for the log.*/

    LogLevel                             m_eLogLevelCurrent;            /**< Holds the current log level.*/

private slots:

    void CentralWidgetShowPlugin();     /**< Sets a widget to central widget of MainWindow class depending on the current plugin selected in m_pDockWidgetPlugins.*/

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

    void zoomStd();                     /**< Implements standard of runWidget.*/
    void zoomIn();                      /**< Implements zoom in of runWidget.*/
    void zoomOut();                     /**< Implements zoom out of runWidget.*/
    void toggleDisplayMax();            /**< Implements show full screen mode of runWidget.*/

    void updateTime();                  /**< Updates m_pTime and is called through timeout() of m_pTimer.*/

};

}// NAMESPACE

#endif // MAINWINDOW_H
