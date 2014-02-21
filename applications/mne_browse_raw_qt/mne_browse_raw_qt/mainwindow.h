//=============================================================================================================
/**
* @file     mainwindow.h
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Florian Schlembach, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    mne_browse_raw_qt is the QT equivalent of the already existing C-version of mne_browse_raw. It is pursued
*           to reimplement the full feature set of mne_browse_raw and even extend these.
*
*           An excerpt of what mne_browse_raw does:
*           "The raw data processor mne_browse_raw is designed for simple raw data viewing and processing operations.
*           In addition, the program is capable of off-line averaging and estimation of covariance matrices.
*           mne_browse_raw can be also used to view averaged data in the topographical layout. Finally, mne_browse_raw
*           can communicate with mne_analyze described in Interactive analysis to calculate current estimates from raw data interactively."
*           (from [1])
*
*           Contributing and extending mne_browse_raw_qt is strongly appreciated!
*           Here are some infos how mne_browse_raw_qt is structured. The program is based on the model/view framework of QT. [2]
*           Hence, the base is divided into the three main compenents and the corresponding classes:
*           - View (included in MainWindow.cpp): The base of mne_browse_raw_qt, everything is instantiated from this class.
*                                               The QTableView is connected to the Model and the Delegate.
*           - Model (RawModel.cpp): The models task is to feed the View with data, the data structure is totally up to the Model.
*                                   In our case, it is derived from QAbstractTableModel, so we are using a table-based data structure.
*           - Delegate (RawDelegate.cpp): The QTableView "delegates" its connected delegate to paint each table cell. The delegate does in turn
*                                         request the data with respect to the underlying QModelIndex (with a certain column and row index).
*
*           Furthermore, the RawSettings class restores the mne_browse_raw_qt settings that were stored after the last session to the corresponding OS environment.
*           Thereby, it makes use of the QSettings class of QT, which stores and restores data locally in a designated place of the OS. [3]
*
*           For further information, see more detailed information in the respective classes' description.
*
*
*
*           [1] http://martinos.org/mne/stable/manual/browse.html
*           [2] http://qt-project.org/doc/qt-5/model-view-programming.html
*           [3] http://qt-project.org/doc/qt-5/QSettings.html
*
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//=============================================================================================================
// INCLUDES

//Qt
#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QMainWindow>
#include <QWidget>

#include <QFileDialog>
#include <QFile>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QSignalMapper>

#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QScroller>

#include <QDockWidget>
#include <QTextBrowser>

#include <QDebug>
#include <QPainter>

#include <QMessageBox>

//MNE
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/parksmcclellan.h>

//MNE_BROWSE_RAW_QT
#include "rawmodel.h"
#include "rawdelegate.h"

#include "info.h"
#include "types.h"
#include "rawsettings.h"

//Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

//*************************************************************************************************************
// namespaces

using namespace Eigen;

//*************************************************************************************************************

namespace MNE_BROWSE_RAW_QT {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    /**
    * Writes to MainWindow log.
    *
    * @param [in] logMsg message
    * @param [in] lgknd message kind; Message is formated depending on its kind.
    * @param [in] lglvl message level; Message is displayed depending on its level.
    */
    void writeToLog(const QString& logMsg, LogKind lgknd, LogLevel lglvl);

public slots:
    /**
     * @brief customContextMenuRequested
     * @param pos is the position, where the right-click occurred
     */
    void customContextMenuRequested(QPoint pos);

    /**
     * setScrollBarPosition sets the position of the horizontal scrollbar
     * @param pos the absolute position of the scrollbar
     */
    void setScrollBarPosition(int pos);

private slots:
    /**
     * openFile opens a file dialog that picks the fiff data file to analyze and invokes the setup methods.
     */
    void openFile();

    /**
     * openFile opens a file dialog that lets choose the location and the file name of the fiff data file to write.
     */
    void writeFile();

    /**
     * about opens the about dialog
     */
    void about();

signals:
    void testSignal();

private:
    /**
     * setupModel creates the RawModel object being part of the model/view framework of QT (derived from QAbstractTableModel)
     */
    void setupModel();

    /**
     * setupDelegate creates the RawDelegate object being part of the model/view framework of QT (derived from QAbstractItemDelegate)
     */
    void setupDelegate();

    /**
     * setupView sets up the QTableView being part of the model/view framework and connects them with previously created RawModel and RawDelegate.
     */
    void setupView();

    /**
     * setupLayout create and connects the individual elements of the layout.
     */
    void setupLayout();

    /**
     * setupViewSettings set the settings of the view such as size policies, scrolling behaviour etc.
     */
    void setupViewSettings();

    /**
     * createMenus sets up the filemenu
     */
    void createMenus();

    /**
     * createLogDockWindow creates the log window as a dock widget
     */
    void createLogDockWindow();

    /**
    * Sets the log level
    *
    * @param [in] lvl message level; Message is displayed depending on its level.
    */
    void setLogLevel(LogLevel lvl);

    /**
     * setWindow makes settings that are related to the MainWindow
     */
    void setWindow();

    /**
     * setWindowStatus sets the window status depending on m_pRawModel->m_bFileloaded
     */
    void setWindowStatus();

    QFile m_qFileRaw; /**< Fiff data file to read (set for convenience) */
    QSignalMapper* m_qSignalMapper; /**< signal mapper used for signal-slot mapping */

    //modelview framework
    RawModel *m_pRawModel; /**< the QAbstractTable model being part of the model/view framework of Qt */
    QTableView *m_pTableView; /**< the QTableView being part of the model/view framework of Qt */
    RawDelegate *m_pRawDelegate; /**< the QAbstractDelegate being part of the model/view framework of Qt */

    //application settings
    QSettings m_qSettings;
    RawSettings m_rawSettings;

    //Log
    QDockWidget* m_pDockWidget_Log; /**< a dock widget being part of the log feature */
    QTextBrowser* m_pTextBrowser_Log; /** a textbox being part of the log feature */
    LogLevel m_eLogLevelCurrent; /**< Holds the current log level.*/

};

}

#endif // MAINWINDOW_H
