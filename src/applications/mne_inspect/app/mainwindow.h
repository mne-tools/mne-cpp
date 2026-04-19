//=============================================================================================================
/**
 * @file     mainwindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    MNE Inspect MainWindow class declaration.
 *
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QElapsedTimer>
#include <QStringList>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QComboBox;
class QCheckBox;
class QSlider;
class QLabel;
class QTimer;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QAction;
class QCloseEvent;
class QDockWidget;
class QGroupBox;
class QMenu;
class QProgressBar;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class BrainView;
class BrainTreeModel;


//=============================================================================================================
/**
 * MainWindow provides the main application window with control panels for brain visualization.
 *
 * @brief    Main application window.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor
     *
     * @param[in] parent     Parent widget.
     */
    explicit MainWindow(QWidget *parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor
     */
    ~MainWindow() override = default;

    //=========================================================================================================
    /**
     * Load initial data from command line arguments.
     *
     * @param[in] subjectPath    Path to subjects directory.
     * @param[in] subjectName    Subject name.
     * @param[in] bemPath        Optional BEM file path.
     * @param[in] transPath      Optional transformation file path.
     * @param[in] stcPath        Optional STC file path.
     * @param[in] digitizerPath  Optional digitizer/sensor file path.
     * @param[in] srcSpacePath   Optional source space file path.
     * @param[in] atlasPath      Optional atlas annotation file path.
     */
    void loadInitialData(const QString &subjectPath,
                         const QString &subjectName,
                         const QString &bemPath = QString(),
                         const QString &transPath = QString(),
                         const QStringList &stcPaths = QStringList(),
                         const QString &digitizerPath = QString(),
                         const QString &srcSpacePath = QString(),
                         const QString &atlasPath = QString(),
                         const QString &evokedPath = QString());

private:
    void setupUI();
    void setupConnections();
    void loadHemisphere(const QString &subjectPath, const QString &subjectName, const QString &hemi);
    void loadBem(const QString &subjectName, const QString &bemPath);
    void syncUIToEditTarget(int target);
    void updateViewportCheckboxes(int count);
    void enableNetworkControls();

    /**
     * Resolve an STC file path into a LH/RH pair and add an entry to the
     * STC combo box.  When @p activate is true the new entry is also selected,
     * triggering a load.
     */
    void addStcEntry(const QString &stcPath, bool activate = true);

    /**
     * Import an MNA/MNX project file — extract embedded files and load them.
     */
    void importMnaProject(const QString &path);

    /**
     * Export the currently loaded data as an MNA (JSON) or MNX (CBOR) project file.
     * When embedData is true, all referenced files are embedded in the container.
     */
    void exportMnaProject(const QString &path, bool embedData);

    /**
     * Track a loaded file path and its role for later export.
     */
    void trackLoadedFile(const QString &path, int role);

    /**
     * Handle close event — save settings before closing.
     *
     * @param[in] event     Close event.
     */
    void closeEvent(QCloseEvent *event) override;

    /**
     * Create menu bar with File, View, Tools, Help menus.
     */
    void createMenus();

    /**
     * Create the status bar.
     */
    void createStatusBar();

    /**
     * Create the Loaded Files dock widget with QTreeWidget.
     */
    void createLoadedFilesDock();

    /**
     * Add an entry to the Loaded Files tree widget.
     */
    void addLoadedFileEntry(const QString &path, int role);

    /**
     * Save window geometry, dock state, and recent files to QSettings.
     */
    void saveSettings();

    /**
     * Restore window geometry, dock state, and recent files from QSettings.
     */
    void restoreSettings();

    /**
     * Update the recent files submenu.
     */
    void updateRecentFilesMenu();

    /**
     * Add a path to the recent files list.
     */
    void addRecentFile(const QString &path);

private:
    // Core components
    BrainView *m_brainView = nullptr;
    BrainTreeModel *m_model = nullptr;

    // Group boxes (disabled until data is loaded)
    QGroupBox *m_surfGroup = nullptr;
    QGroupBox *m_bemGroup = nullptr;
    QGroupBox *m_stcGroup = nullptr;
    QGroupBox *m_dipoleGroup = nullptr;
    QGroupBox *m_srcSpaceGroup = nullptr;
    QGroupBox *m_networkGroup = nullptr;
    QGroupBox *m_evokedGroup = nullptr;
    QGroupBox *m_sensorStreamGroup = nullptr;
    QGroupBox *m_sensorGroup = nullptr;

    // Control widgets - FsSurface
    QComboBox *m_surfCombo = nullptr;
    QComboBox *m_overlayCombo = nullptr;
    QComboBox *m_shaderCombo = nullptr;
    QComboBox *m_bemShaderCombo = nullptr;
    QCheckBox *m_linkShadersCheck = nullptr;
    QCheckBox *m_bemColorCheck = nullptr;

    // Control widgets - Hemisphere
    QCheckBox *m_lhCheck = nullptr;
    QCheckBox *m_rhCheck = nullptr;

    // Control widgets - BEM
    QCheckBox *m_headCheck = nullptr;
    QCheckBox *m_outerCheck = nullptr;
    QCheckBox *m_innerCheck = nullptr;

    // Control widgets - View
    QComboBox *m_viewCountCombo = nullptr;
    QComboBox *m_editTargetCombo = nullptr;
    QComboBox *m_cameraPresetCombo = nullptr;
    QLabel *m_editTargetLabel = nullptr;
    QLabel *m_cameraPresetLabel = nullptr;

    // Control widgets - Brain FsSurface

    // Control widgets - STC
    QComboBox *m_stcCombo = nullptr;
    QLabel *m_stcStatusLabel = nullptr;
    QProgressBar *m_stcProgressBar = nullptr;
    QComboBox *m_colormapCombo = nullptr;
    QSlider *m_timeSlider = nullptr;
    QLabel *m_timeLabel = nullptr;
    QDoubleSpinBox *m_minThresh = nullptr;
    QDoubleSpinBox *m_midThresh = nullptr;
    QDoubleSpinBox *m_maxThresh = nullptr;
    QPushButton *m_playButton = nullptr;
    QComboBox *m_speedCombo = nullptr;
    QTimer *m_stcTimer = nullptr;

    // Control widgets - Sensors
    QCheckBox *m_showMegCheck = nullptr;
    QCheckBox *m_showEegCheck = nullptr;
    QCheckBox *m_showDigCheck = nullptr;
    QCheckBox *m_showDigCardinalCheck = nullptr;
    QCheckBox *m_showDigHpiCheck = nullptr;
    QCheckBox *m_showDigEegCheck = nullptr;
    QCheckBox *m_showDigExtraCheck = nullptr;
    QCheckBox *m_applyTransCheck = nullptr;

    // Control widgets - Dipoles
    QCheckBox *m_showDipoleCheck = nullptr;

    // Control widgets - Source Space
    QCheckBox *m_showSrcSpaceCheck = nullptr;

    // Control widgets - Network
    QCheckBox *m_showNetworkCheck = nullptr;
    QSlider *m_networkThresholdSlider = nullptr;
    QComboBox *m_networkColormapCombo = nullptr;

    // Control widgets - Evoked / Sensor Field
    QComboBox *m_evokedSetCombo = nullptr;
    QCheckBox *m_showMegFieldCheck = nullptr;
    QCheckBox *m_showEegFieldCheck = nullptr;
    QCheckBox *m_showMegContourCheck = nullptr;
    QCheckBox *m_showEegContourCheck = nullptr;
    QCheckBox *m_showHelmetCheck = nullptr;
    QComboBox *m_helmetShapeCombo = nullptr;
    QComboBox *m_megHelmetCombo = nullptr;
    QSlider *m_sensorFieldTimeSlider = nullptr;
    QLabel *m_sensorFieldTimeLabel = nullptr;
    QCheckBox *m_syncTimesCheck = nullptr;

    // Control widgets - Sensor Streaming
    QPushButton *m_sensorStreamBtn = nullptr;
    QComboBox *m_sensorStreamModalityCombo = nullptr;
    QCheckBox *m_sensorStreamLoopCheck = nullptr;
    QSpinBox *m_sensorStreamAvgSpin = nullptr;
    QComboBox *m_sensorStreamColormapCombo = nullptr;

    // Control widgets - STC Playback
    QCheckBox *m_realtimeCheck = nullptr;
    QCheckBox *m_loopCheck = nullptr;

    // Control widgets - View (additional)
    QCheckBox *m_showInfoCheck = nullptr;

    // Playback stepping (for real-time accurate playback)
    QElapsedTimer m_playbackClock;          //!< Wall-clock for measuring actual elapsed time
    double m_stcStepAccum = 0.0;            //!< Fractional sample accumulator

    // Loaded file tracking for MNA export (path → MnaFileRole int)
    QList<QPair<QString, int>> m_loadedFiles;

    // Sync state
    bool m_isSyncing = false;

    // Dock widgets
    QDockWidget *m_controlsDock = nullptr;
    QDockWidget *m_loadedFilesDock = nullptr;

    // Loaded files tree
    QTreeWidget *m_loadedFilesTree = nullptr;

    // Status bar labels
    QLabel *m_statusLabel = nullptr;
    QLabel *m_statusTimeLabel = nullptr;

    // Menu bar menus
    QMenu *m_fileMenu = nullptr;
    QMenu *m_viewMenu = nullptr;
    QMenu *m_toolsMenu = nullptr;
    QMenu *m_helpMenu = nullptr;
    QMenu *m_recentFilesMenu = nullptr;
    QMenu *m_cameraPresetsMenu = nullptr;
    QMenu *m_playbackMenu = nullptr;

    // Actions
    QAction *m_actOpenProject = nullptr;
    QAction *m_actExportProject = nullptr;
    QAction *m_actLoadSurface = nullptr;
    QAction *m_actLoadAtlas = nullptr;
    QAction *m_actLoadBem = nullptr;
    QAction *m_actLoadStc = nullptr;
    QAction *m_actLoadDipoles = nullptr;
    QAction *m_actLoadSrcSpace = nullptr;
    QAction *m_actLoadEvoked = nullptr;
    QAction *m_actLoadDigitizer = nullptr;
    QAction *m_actLoadTransform = nullptr;
    QAction *m_actQuit = nullptr;
    QAction *m_actShowControls = nullptr;
    QAction *m_actShowLoadedFiles = nullptr;
    QAction *m_actResetCamera = nullptr;
    QAction *m_actPlayPause = nullptr;
    QAction *m_actStepFwd = nullptr;
    QAction *m_actStepBack = nullptr;
    QAction *m_actRealtimeToggle = nullptr;
    QAction *m_actSyncLock = nullptr;

    // Recent files
    QStringList m_recentFiles;
};

#endif // MAINWINDOW_H
