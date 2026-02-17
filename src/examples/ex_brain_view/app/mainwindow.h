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
 * @brief    MainWindow class declaration.
 *
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QMainWindow>

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
class QProgressBar;
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
     * @param[in] evokedPath     Optional evoked/average file path.
     */
    void loadInitialData(const QString &subjectPath,
                         const QString &subjectName,
                         const QString &bemPath = QString(),
                         const QString &transPath = QString(),
                         const QString &stcPath = QString(),
                         const QString &digitizerPath = QString(),
                         const QString &srcSpacePath = QString(),
                         const QString &atlasPath = QString(),
                         const QString &evokedPath = QString());

    /**
     * Enable network UI controls (called after a network has been loaded).
     */
    void enableNetworkControls();

private:
    void setupUI();
    void setupConnections();
    void loadHemisphere(const QString &subjectPath, const QString &subjectName, const QString &hemi);
    void loadBem(const QString &subjectName, const QString &bemPath);

private:
    // Core components
    BrainView *m_brainView = nullptr;
    BrainTreeModel *m_model = nullptr;

    // Control widgets - Surface
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
    QPushButton *m_loadBemBtn = nullptr;
    QCheckBox *m_headCheck = nullptr;
    QCheckBox *m_outerCheck = nullptr;
    QCheckBox *m_innerCheck = nullptr;

    // Control widgets - View
    QCheckBox *m_showInfoCheck = nullptr;
    QCheckBox *m_multiViewCheck = nullptr;
    QCheckBox *m_vpTopCheck = nullptr;
    QCheckBox *m_vpBottomCheck = nullptr;
    QCheckBox *m_vpFrontCheck = nullptr;
    QCheckBox *m_vpLeftCheck = nullptr;

    // Control widgets - Brain Surface
    QPushButton *m_loadSurfaceBtn = nullptr;
    QPushButton *m_loadAtlasBtn = nullptr;

    // Control widgets - STC
    QPushButton *m_loadStcBtn = nullptr;
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
    QCheckBox *m_realtimeCheck = nullptr;
    QCheckBox *m_loopCheck = nullptr;

    // Control widgets - Sensors
    QPushButton *m_loadDigBtn = nullptr;
    QPushButton *m_loadTransBtn = nullptr;
    QCheckBox *m_showMegCheck = nullptr;
    QCheckBox *m_showEegCheck = nullptr;
    QCheckBox *m_showDigCheck = nullptr;
    QCheckBox *m_showDigCardinalCheck = nullptr;
    QCheckBox *m_showDigHpiCheck = nullptr;
    QCheckBox *m_showDigEegCheck = nullptr;
    QCheckBox *m_showDigExtraCheck = nullptr;
    QCheckBox *m_showHelmetCheck = nullptr;
    QCheckBox *m_applyTransCheck = nullptr;

    // Control widgets - Dipoles
    QPushButton *m_loadDipoleBtn = nullptr;
    QCheckBox *m_showDipoleCheck = nullptr;

    // Control widgets - Source Space
    QPushButton *m_loadSrcSpaceBtn = nullptr;
    QCheckBox *m_showSrcSpaceCheck = nullptr;

    // Control widgets - Connectivity Network
    QCheckBox *m_showNetworkCheck = nullptr;
    QSlider *m_networkThresholdSlider = nullptr;
    QComboBox *m_networkColormapCombo = nullptr;

    // Control widgets - Evoked / Field Maps
    QPushButton *m_loadEvokedBtn = nullptr;
    QComboBox *m_evokedSetCombo = nullptr;
    QCheckBox *m_showMegFieldCheck = nullptr;
    QCheckBox *m_showEegFieldCheck = nullptr;
    QCheckBox *m_showMegContourCheck = nullptr;
    QCheckBox *m_showEegContourCheck = nullptr;
    QComboBox *m_megHelmetCombo = nullptr;
    QSlider *m_sensorFieldTimeSlider = nullptr;
    QLabel *m_sensorFieldTimeLabel = nullptr;
    QCheckBox *m_syncTimesCheck = nullptr;
    bool m_isSyncing = false;           //!< Guard against re-entrant slider sync

    // Control widgets - Sensor Streaming
    QPushButton *m_sensorStreamBtn = nullptr;
    QComboBox *m_sensorStreamModalityCombo = nullptr;
    QCheckBox *m_sensorStreamLoopCheck = nullptr;
    QComboBox *m_sensorStreamColormapCombo = nullptr;
    QSpinBox *m_sensorStreamAvgSpin = nullptr;
};

#endif // MAINWINDOW_H
