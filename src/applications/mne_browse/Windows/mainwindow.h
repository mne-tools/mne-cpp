//=============================================================================================================
/**
 * @file     mainwindow.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     January, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    mne_browse is the QT equivalent of the already existing C-version of mne_browse_raw. It is pursued
 *           to reimplement the full feature set of mne_browse_raw and even extend these.
 *
 *           An excerpt of what mne_browse_raw does:
 *           "The raw data processor mne_browse_raw is designed for simple raw data viewing and processing operations.
 *           In addition, the program is capable of off-line averaging and estimation of covariance matrices.
 *           mne_browse_raw can be also used to view averaged data in the topographical layout. Finally, mne_browse_raw
 *           can communicate with mne_analyze described in Interactive analysis to calculate current estimates from raw data interactively."
 *           (from [1])
 *
 *           Contributing and extending mne_browse is strongly appreciated!
 *           Here are some infos how mne_browse is structured. The program is based on the model/view framework of QT. [2]
 *           Hence, the base is divided into the three main compenents and the corresponding classes:
 *           - View (included in MainWindow.cpp): The base of mne_browse, everything is instantiated from this class.
 *                                               The QTableView is connected to the Model and the Delegate.
 *           - Model (RawModel.cpp): The models task is to feed the View with data, the data structure is totally up to the Model.
 *                                   In our case, it is derived from QAbstractTableModel, so we are using a table-based data structure.
 *           - Delegate (RawDelegate.cpp): The QTableView "delegates" its connected delegate to paint each table cell. The delegate does in turn
 *                                         request the data with respect to the underlying QModelIndex (with a certain column and row index).
 *
 *           Furthermore, the RawSettings class restores the mne_browse settings that were stored after the last session to the corresponding OS environment.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Utils/info.h"
#include "../Utils/types.h"
#include "../Utils/rawsettings.h"

#include "filterwindow.h"
#include "eventwindow.h"
#include "datawindow.h"
#include "aboutwindow.h"
#include "informationwindow.h"
#include "averagewindow.h"
#include "scalewindow.h"
#include "chinfowindow.h"
#include "noisereductionwindow.h"

#include <disp/viewers/channelselectionview.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QScrollBar>
#include <QToolBar>
#include <QScroller>
#include <QTextBrowser>
#include <QMessageBox>
#include <QLabel>
#include <QPixmap>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QMessageBox>

#include <memory>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <mne/mne.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

class FilterWindow;
class EventWindow;
class DataWindow;


//=============================================================================================================
/**
 * DECLARE CLASS MainWindow
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //=========================================================================================================
    /**
     * Writes to MainWindow log.
     *
     * @param [in] logMsg message
     * @param [in] lgknd message kind; Message is formated depending on its kind.
     * @param [in] lglvl message level; Message is displayed depending on its level.
     */
    void writeToLog(const QString& logMsg, LogKind lgknd, LogLevel lglvl);

private:
    //=========================================================================================================
    /**
     * openFile opens a file dialog that picks the fiff data file to analyze and invokes the setup methods.
     */
    void openFile();

    //=========================================================================================================
    /**
     * openFile opens a file dialog that lets choose the location and the file name of the fiff data file to write.
     */
    void writeFile();

    //=========================================================================================================
    /**
     * loadEvents opens a file dialog that picks the event data file.
     */
    void loadEvents();

    //=========================================================================================================
    /**
     * saveEvents saves the event data to file.
     */
    void saveEvents();

public:
    //=========================================================================================================
    /**
     * Sub-window and file accessors — used by FilterWindow, EventWindow, DataWindow.
     * These replace the former friend class declarations.
     */
    DataWindow*             dataWindow()            const { return m_pDataWindow; }
    EventWindow*            eventWindow()           const { return m_pEventWindow; }
    ChannelSelectionView*   channelSelectionView()  const { return m_pChannelSelectionView; }
    ChInfoWindow*           chInfoWindow()          const { return m_pChInfoWindow; }
    QFile&                  rawFile()                     { return m_qFileRaw; }
    QFile&                  eventFile()                   { return m_qEventFile; }

    /** Convenience accessor — collapses the dataWindow()->getDataModel() chain. */
    RawModel*               rawModel()              const;

    //=========================================================================================================
    /**
     * applyCommandLineOptions applies options parsed from the command line after the window is shown.
     *
     * @param [in] rawFile     Path to raw FIFF file to open, or empty.
     * @param [in] eventsFile  Path to event FIFF file to load, or empty.
     * @param [in] highpass    High-pass corner frequency in Hz, or -1 to skip.
     * @param [in] lowpass     Low-pass corner frequency in Hz, or -1 to skip.
     */
    void applyCommandLineOptions(const QString& rawFile,
                                 const QString& eventsFile,
                                 double highpass,
                                 double lowpass);

private:
    //=========================================================================================================
    /**
     * loadRawFile loads a raw FIFF file without a dialog.
     *
     * @param [in] filename  Absolute path to the raw FIFF file.
     * @return true on success.
     */
    bool loadRawFile(const QString& filename);

    //=========================================================================================================
    /**
     * loadEventsFile loads an event FIFF file without a dialog.
     *
     * @param [in] filename  Absolute path to the event file.
     * @return true on success.
     */
    bool loadEventsFile(const QString& filename);

    //=========================================================================================================
    /**
     * loadEvoked load the evoked data from file.
     */
    void loadEvoked();

    //=========================================================================================================
    /**
     * showFilterWindow shows the filtering window
     */
    void showWindow(QWidget *window);

private:
    //=========================================================================================================
    /**
     * setupWindowWidgets sets up the windows which can be shown during runtime (i.e. filter window, event list window, etc.).
     */
    void setupWindowWidgets();

    //=========================================================================================================
    /**
     * createToolBar sets up the applications toolbar
     */
    void createToolBar();

    //=========================================================================================================
    /**
     * connectMenus sets up the filemenu
     */
    void connectMenus();

    //=========================================================================================================
    /**
     * Sets the log level
     *
     * @param [in] lvl message level; Message is displayed depending on its level.
     */
    void setLogLevel(LogLevel lvl);

    //=========================================================================================================
    /**
     * setupMainWindow makes settings that are related to the MainWindow
     */
    void setupMainWindow();

    //=========================================================================================================
    /**
     * setWindowStatus sets the window status depending on the loaded file state
     */
    void setWindowStatus();

    QFile                   m_qFileRaw;                     /**< Fiff data file to read (set for convenience). */
    QFile                   m_qEventFile;                   /**< Fiff event data file to read (set for convenience). */
    QFile                   m_qEvokedFile;                  /**< Fiff event data file to read (set for convenience). */

    //Window widgets
    EventWindow*            m_pEventWindow;                 /**< Event widget which display the event view. */
    FilterWindow*           m_pFilterWindow;                /**< Filter widget which display the filter options for the user. */
    DataWindow*             m_pDataWindow;                  /**< Data widget which display the data for the user. */
    AboutWindow*            m_pAboutWindow;                 /**< About widget which displays information about this application.*/
    InformationWindow*      m_pInformationWindow;           /**< Information widget which displays information about this application (log, etc.).*/
    ChannelSelectionView* m_pChannelSelectionView;      /**< Selection manager window which can be used to select channels.*/
    AverageWindow*          m_pAverageWindow;               /**< Average window can be used to plot calculated averages in a 2D layout scene.*/
    ScaleWindow*            m_pScaleWindow;                 /**< Scale widget can be used to set the scaling of the different channels types. */
    ChInfoWindow*           m_pChInfoWindow;                /**< Dock window which shows the information about the curretly loaded data channels. */
    NoiseReductionWindow*   m_pNoiseReductionWindow;        /**< Dock widget to hold he projection manager. */

    QDockWidget*            m_pChannelSelectionViewDock;

    //application settings
    QSettings               m_qSettings;                    /**< QSettings variable used to write or read from independent application sessions. */
    RawSettings             m_rawSettings;                  /**< The software specific mne brose raw qt settings. */

    std::unique_ptr<Ui::MainWindowWidget> ui;               /**< Pointer to the qt designer generated ui class.*/

    QLabel*                 m_pStatusLabel;                 /**< Persistent status bar label, updated in place to avoid repeated allocation. */
    QAction*                m_pRemoveDCAction;              /**< The action which is used to control DC removal. */
    QAction*                m_pHideBadAction;               /**< The action which is used to control hide bad channel functionality. */
};

} //NAMESPACE

#endif // MAINWINDOW_H
