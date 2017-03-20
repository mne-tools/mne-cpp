//=============================================================================================================
/**
* @file     mainwindow.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mdiview.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QString>

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QDockWidget;
QT_END_NAMESPACE

namespace ANSHAREDLIB
{
class IExtension;
class ExtensionManager;
class AnalyzeData;
class AnalyzeSettings;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEANALYZE
//=============================================================================================================

namespace MNEANALYZE
{


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    typedef QSharedPointer<MainWindow> SPtr;               /**< Shared pointer type for MainWindow. */
    typedef QSharedPointer<const MainWindow> ConstSPtr;    /**< Const shared pointer type for MainWindow. */

    MainWindow(QWidget *parent = 0);

    ~MainWindow();


    void initGlobalSettings();
    void initGlobalData();



private:

    void createActions();       /**< Creates all actions for user interface of MainWindow class. */
    void createMenus();         /**< Creates all menus for user interface of MainWindow class. */
    void createDockWindows();
    void createMdiView();

    QAction*                            m_pActionOpenDataFile;      /**< open data file action */
    QAction*                            m_pActionExit;              /**< exit application action */

    QAction*                            m_pActionCascade;           /**< view cascade action */
    QAction*                            m_pActionTile;              /**< view tile action */

    QAction*                            m_pActionAbout;             /**< show about dialog action */

    //Main Window Menu
    QMenu*                              m_pMenuFile;        /**< Holds the file menu.*/
    QMenu*                              m_pMenuView;        /**< Holds the view menu.*/
    QMenu*                              m_pMenuHelp;        /**< Holds the help menu.*/

    QSharedPointer<QWidget>             m_pAboutWindow;     /**< Holds the widget containing the about information.*/

    QSharedPointer<ANSHAREDLIB::ExtensionManager>   m_pExtensionManager;    /**< Holds extension manager.*/


private:

    //mdi area subwindows
    void mdiViewCascade();
    void mdiViewTile();

    //Open a FIFF file
    void openFiffFile();            /**< Implements open fiff action.*/
    void about();                   /**< Implements about action.*/


private:

    //MDI Central View
    MdiView            *m_mdiView;

    //FIFF File management
    QString                 m_fiffFileName;

    QSharedPointer<ANSHAREDLIB::AnalyzeSettings> m_analyzeSettings;
    QSharedPointer<ANSHAREDLIB::AnalyzeData> m_analyzeData;

};

}// NAMESPACE

#endif // ANMAINWINDOW_H
