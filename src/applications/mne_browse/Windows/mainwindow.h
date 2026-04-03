//=============================================================================================================
/**
 * @file     mainwindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
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
#include "annotationwindow.h"
#include "covariancewindow.h"
#include "epochwindow.h"
#include "virtualchannelwindow.h"
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
#include <QtConcurrent>

#include <memory>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <mne/mne.h>
#include <mne/mne_inverse_operator.h>


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
class RawModel;


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

    //=========================================================================================================
    /**
     * loadAnnotations opens a file dialog that picks the annotation sidecar file.
     */
    void loadAnnotations();

    //=========================================================================================================
    /**
     * saveAnnotations saves the annotation sidecar file.
     */
    void saveAnnotations();

    //=========================================================================================================
    /**
     * loadVirtualChannels opens a file dialog that picks the virtual-channel sidecar file.
     */
    void loadVirtualChannels();

    //=========================================================================================================
    /**
     * saveVirtualChannels saves the virtual-channel sidecar file.
     */
    void saveVirtualChannels();

public:
    //=========================================================================================================
    /**
     * Sub-window and file accessors — used by FilterWindow, EventWindow, DataWindow.
     * These replace the former friend class declarations.
     */
    void toggleFullscreen();
    void toggleZenMode();
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
     * Ensure the legacy RawModel is loaded for workflows that still require it
     * (for example write/filter/projection operations).
     *
     * @param [in] featureName  User-facing feature name used in warning dialogs.
     * @return true when the legacy model is available.
     */
    bool ensureLegacyRawModelLoaded(const QString& featureName = QString());

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
     * loadAnnotationsFile loads a browser annotation JSON file without a dialog.
     *
     * @param [in] filename  Absolute path to the annotation file.
     * @param [in] showWindow  True when the annotation manager should be shown on success.
     */
    bool loadAnnotationsFile(const QString& filename, bool showWindow = true);

    //=========================================================================================================
    /**
     * loadVirtualChannelsFile loads a browser virtual-channel JSON file without a dialog.
     *
     * @param [in] filename  Absolute path to the virtual-channel file.
     * @param [in] showWindow  True when the manager should be shown on success.
     */
    bool loadVirtualChannelsFile(const QString& filename, bool showWindow = true);

    //=========================================================================================================
    /**
     * loadInverseOperator loads an inverse-operator FIF file without a dialog.
     *
     * @param [in] filename  Absolute path to the inverse-operator file.
     * @return true on success.
     */
    bool loadInverseOperatorFile(const QString& filename);

    //=========================================================================================================
    /**
     * loadEvoked load the evoked data from file.
     */
    void loadEvoked();

    //=========================================================================================================
    /**
     * Load a precomputed inverse operator from a FIF file.
     */
    void loadInverseOperator();

    //=========================================================================================================
    /**
     * Compute evoked responses from the currently loaded raw file and available events.
     */
    void computeEvoked();

    //=========================================================================================================
    /**
     * Recompute evoked responses using the last saved evoked settings.
     */
    void recomputeEvoked();

    //=========================================================================================================
    /**
     * Save the currently displayed evoked set to a FIF file.
     */
    void saveEvoked();

    //=========================================================================================================
    /**
     * Rebuild the in-memory evoked set from the currently reviewed epochs.
     *
     * @param [in] statusMessage  Optional status-bar text shown on success.
     * @return true on success.
     */
    bool refreshReviewedEvokedSet(const QString& statusMessage = QString());

    //=========================================================================================================
    /**
     * Compute a covariance matrix from the currently loaded raw file and available events.
     */
    void computeCovariance();

    //=========================================================================================================
    /**
     * Load a covariance matrix from a FIF file.
     */
    void loadCovariance();

    //=========================================================================================================
    /**
     * Compute source estimates from the currently selected evoked response(s).
     */
    void computeSourceEstimate();

    //=========================================================================================================
    /**
     * Save the currently computed covariance matrix to a FIF file.
     */
    void saveCovariance();

    //=========================================================================================================
    /**
     * Returns the current covariance-whitening settings.
     */
    WhiteningSettings covarianceWhiteningSettings() const;

    //=========================================================================================================
    /**
     * Apply covariance-whitening settings to the browser UI and optionally persist them.
     *
     * @param [in] settings      Whitening settings to apply.
     * @param [in] saveSettings  True to store the settings in QSettings.
     */
    void setCovarianceWhiteningSettings(const WhiteningSettings& settings, bool saveSettings = true);

    //=========================================================================================================
    /**
     * Prompt for an annotation label and create an annotation from the selected raw-view span.
     */
    void handleAnnotationRangeSelected(int startSample, int endSample);

    //=========================================================================================================
    /**
     * Clear the current epoch-review session.
     */
    void clearEpochReviewSession();

    //=========================================================================================================
    /**
     * showFilterWindow shows the filtering window
     */
    void showWindow(QWidget *window);

    //=========================================================================================================
    /**
     * Shared evoked-computation implementation used for both prompted and quick-recompute flows.
     *
     * @param [in] promptForSettings  True to show the settings dialog, false to reuse the last saved setup.
     * @return true on success.
     */
    bool runEvokedComputation(bool promptForSettings);

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

    void applyViewSettingsToDataView();

protected:
    //=========================================================================================================
    /**
     * Saves persistent settings (window geometry, view toggles) on close.
     */
    void closeEvent(QCloseEvent *event) override;

    //=========================================================================================================
    /**
     * setWindowStatus sets the window status depending on the loaded file state
     */
    void setWindowStatus();

    //=========================================================================================================
    /**
     * Seed auxiliary windows from the currently available FIFF header information.
     * This keeps selection/layout-driven UI responsive directly from the fast
     * FiffBlockReader session state.
     */
    void syncAuxWindowsToFiffInfo(FIFFLIB::FiffInfo::SPtr fiffInfo,
                                  int firstSample,
                                  int lastSample);

    QFile                   m_qFileRaw;                     /**< Fiff data file to read (set for convenience). */
    QFile                   m_qEventFile;                   /**< Fiff event data file to read (set for convenience). */
    QFile                   m_qAnnotationFile;              /**< Browser annotation sidecar file. */
    QFile                   m_qVirtualChannelFile;          /**< Browser virtual-channel sidecar file. */
    QFile                   m_qEvokedFile;                  /**< Fiff event data file to read (set for convenience). */
    QFile                   m_qCovFile;                     /**< Fiff covariance file to write (set for convenience). */
    QFile                   m_qInverseOperatorFile;         /**< Inverse-operator fif file used for source-estimate export. */
    FIFFLIB::FiffCov        m_covariance;                   /**< Last computed covariance matrix. */
    MNELIB::MNEInverseOperator m_inverseOperator;           /**< Last loaded inverse operator. */

    //Window widgets
    EventWindow*            m_pEventWindow;                 /**< Event widget which display the event view. */
    AnnotationWindow*       m_pAnnotationWindow;            /**< Annotation widget which displays browser annotations. */
    CovarianceWindow*       m_pCovarianceWindow;            /**< Dock widget which inspects covariance data and drives whitening. */
    EpochWindow*            m_pEpochWindow;                 /**< Dock widget which reviews epochs before averaging. */
    VirtualChannelWindow*   m_pVirtualChannelWindow;        /**< Dock widget which manages browser-level derived channels. */
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
    QAction*                m_pCrosshairAction;             /**< Toggle crosshair cursor overlay. */
    QAction*                m_pButterflyAction;             /**< Toggle butterfly mode. */
    QAction*                m_pScalebarsAction;             /**< Toggle scalebars. */
    QAction*                m_pEventsVisibleAction;         /**< Toggle event marker visibility (E). */
    QAction*                m_pAnnotationsVisibleAction;    /**< Toggle annotation span visibility (Shift+A). */
    QAction*                m_pOverviewBarAction;           /**< Toggle overview bar visibility (O). */
    QAction*                m_pEpochMarkersAction;          /**< Toggle epoch grid lines (G). */
    QAction*                m_pClippingAction;              /**< Toggle clipping detection (C). */
    QAction*                m_pZScoreAction;                /**< Toggle z-score normalization (Z). */
    QAction*                m_pWhitenButterflyAction;       /**< Toggle whitening in the average butterfly plot. */
    QAction*                m_pAnnotationModeAction;        /**< Toggle Shift-drag annotation selection in the raw browser. */
    QList<MNELIB::MNEEpochDataList> m_epochReviewLists;     /**< Reviewed epochs grouped by event code. */
    QList<int>              m_epochReviewEventCodes;        /**< Event codes for the current review session. */
    QStringList             m_epochReviewComments;          /**< Display comments for the current review session. */
    FIFFLIB::FiffInfo::SPtr m_pEpochReviewInfo;             /**< Raw info used to rebuild evoked responses from reviewed epochs. */
    QPair<float,float>      m_epochReviewBaseline;          /**< Baseline metadata of the current review session. */
};

} //NAMESPACE

#endif // MAINWINDOW_H
