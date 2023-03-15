//=============================================================================================================
/**
 * @file     mainwindow.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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

#include <disp/viewers/abstractview.h>
#include "analyzecore.h"

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QCloseEvent>
#include <QString>
#include <QPointer>
#include <QTextBrowser>
#include <QSettings>
#include <QMutex>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QDockWidget;
class QGridLayout;
QT_END_NAMESPACE

namespace DISPLIB {
    class MultiView;
}

//=============================================================================================================
// DEFINE NAMESPACE MNEANALYZE
//=============================================================================================================

namespace MNEANALYZE
{

//=============================================================================================================
// MNEANALYZE FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * MNE Analyze MainWindow
 *
 * @brief The MainWindow class provides the main mne analyze user interface.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    typedef QSharedPointer<MainWindow> SPtr;               /**< Shared pointer type for MainWindow. */
    typedef QSharedPointer<const MainWindow> ConstSPtr;    /**< Const shared pointer type for MainWindow. */

    //=========================================================================================================
    /**
     * Constructs a MainWindow which is a child of parent.If parent is another widget, MainWindow becomes a
     * child window inside parent. MainWindow is deleted when its parent is deleted.
     *
     * @param [in] pAnalyzeCore Pointer to the application controller class. It is needed to display subwindows froms plugins.
     * @param [in] parent Pointer to parent widget; If parent is Q_NULLPTR, the new MainWindow becomes a window.
     *
     */
    MainWindow(AnalyzeCore* pAnalyzeCore,
               QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destroys the MainWindow.
     * All MainWindow's children are deleted first. The application exits if MainWindow is the main widget.
     */
    ~MainWindow();

    //=========================================================================================================
    /**
     * This is called when the user presses the "close" button. It notifies the AnalyzeCore via a QtConnect.
     *
     * @param[in] event The event that has happened.
     */
    void closeEvent(QCloseEvent *event) override;

    //=============================================================================================================
    /**
     * Custom Qt message handler.
     *
     * @param[in] type      enum to identify the various message types.
     * @param[in] context   additional information about a log message.
     * @param[in] msg       the message to log.
     */
    void writeToLog(QtMsgType type,
                    const QMessageLogContext &context,
                    const QString &msg);

    //=========================================================================================================
    /**
     * Saves geometry and state of GUI dock widgets that have given a name with setObjectName()
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Restores geometry and state as saved by saveSettings()
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Redraw all the menus and actions created by the plugins.
     */
    void updatePluginsUI();

signals:
    //=========================================================================================================
    /**
     * Signal emmited when the main window is closed
     */
    void mainWindowClosed();

    //=========================================================================================================
    /**
     * Signal emmited whenever the gui modes changes
     *
     * @param[in] mode       the new gui mode.
     */
    void guiModeChanged(DISPLIB::AbstractView::GuiMode mode);

    //=========================================================================================================
    /**
     * Signal emmited whenever the gui style changes
     *
     * @param[in] style       the new gui style.
     */
    void guiStyleChanged(DISPLIB::AbstractView::StyleMode style);

    //=========================================================================================================
    /**
     * Signal emmited whenever the gui menu option: Reload Plugins is clicked.
     */
    void reloadPlugins();

private:
    //=========================================================================================================
    /**
     * Changes the current layout style of the application.
     *
     * @param[in] sStyle   The new qss style.
     */
    void changeStyle();

    //=========================================================================================================
    /**
     * Handles changes made to the application's GUI mode.
     */
    void manageGuiModeChanged();

    //=========================================================================================================
    /**
     * Creates log dock widget.
     */
    void createLogDockWindow();

    //=========================================================================================================
    /**
     * Setter method for CurrentStyle member variable.
     */
    void setCurrentStyle(const QString& style);

    //=========================================================================================================
    /**
    * Creates all actions for user interface of MainWindow class.
    */
    void createPluginMenus();

    //=========================================================================================================
    /**
     * Creates all dock windows for user interface of MainWindow class.
     */
    void createPluginControls();

    //=========================================================================================================
    /**
     *  Creates all Windows within the MultiView for user interface of MainWindow class.
     */
    void createPluginViews();

    //=========================================================================================================
    /**
     * Tabify all dock windows.
     */
    void tabifyDockWindows();

    //=========================================================================================================
    /**
     * Implements about action.
     */
    void about();

    //=========================================================================================================
    /**
     * Check the initialization state of the plugins and if correct, initialize all the menus, actions created
     * by all the plugins.
     */
    void initMenusAndPluginControls();

    //=========================================================================================================
    /**
     * Creates the menu actions.
     */
    void initMenuBar();
\
    //=========================================================================================================
    /**
     * Initialize this window's name size and some properties.
     */
    void initWindow();

    //=========================================================================================================
    /**
     * Load previously saved configuration settings for windows, tab order, and style.
     */
    void loadSavedSettingsAndState();

    //=========================================================================================================
    /**
     * Initialize statusbar for this window.
     */
    void initStatusBar();

    //=========================================================================================================
    /**
     * Set the properties of the control views, regarding how and where they can be tabbed.
     */
    void initPluginControls();

    //=========================================================================================================
    /**
     * Initialize the multiview widget that holds the views for each plugin.
     */
    void initPluginViews();

    //=========================================================================================================
    /**
     * Delete the menus created in initMenuBar.
     */
    void deleteMenus();

    QPointer<DISPLIB::MultiView>        m_pMultiView;               /**< The central View.*/
    QPointer<AnalyzeCore>               m_pAnalyzeCoreController;   /**< Pointer to the application controller Obj.*/
    QPointer<QGridLayout>               m_pGridLayout;              /**< Grid Layout is used for MainWindow, so that the MultiView can always fit the size of MainWindow */

    // MainWindow actions
    QPointer<QAction>                   m_pActionExit;              /**< Exit application action. */
    QPointer<QAction>                   m_pActionReloadPlugins;     /**< Reload plugins action. */
    QPointer<QAction>                   m_pActionAbout;             /**< Show about dialog action. */
    QPointer<QAction>                   m_pActionResearchMode;      /**< Toggle research mode action. */
    QPointer<QAction>                   m_pActionClinicalMode;      /**< Toggle clinical mode action. */
    QPointer<QAction>                   m_pActionDarkMode;          /**< Toggle dark mode. */

    // MainWindow menus
    QPointer<QMenu>                     m_pMenuFile;                /**< Holds the file menu.*/
    QPointer<QMenu>                     m_pMenuView;                /**< Holds the view menu.*/
    QPointer<QMenu>                     m_pMenuControl;             /**< Holds the control menu. */
    QPointer<QMenu>                     m_pMenuAppearance;          /**< Holds the appearance menu.*/
    QPointer<QMenu>                     m_pMenuHelp;                /**< Holds the help menu.*/

    QPointer<QTextBrowser>              m_pTextBrowser_Log;         /**< Holds the text browser for the log.*/

    QSharedPointer<QWidget>             m_pAboutWindow;             /**< Holds the widget containing the about information.*/

    QString                             m_sSettingsPath;            /**< The settings path to store the GUI settings to. */
    QString                             m_sCurrentStyle;            /**< The currently selected style (dark mode, default mode). */

    QMutex                              m_Mutex;                    /**< Thread-safing for printing messages. */
};

}// NAMESPACE

#endif // ANMAINWINDOW_H
