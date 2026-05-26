//=============================================================================================================
/**
 * @file     mainwindow.cpp
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
 * @brief    MNE Inspect MainWindow class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"
#include <disp3D/view/brainview.h>
#include <disp3D/model/braintreemodel.h>
#include <disp3D/core/viewstate.h>
#include <disp3D/scene/multimodalscene.h>
#include <disp3D/scene/pickresult.h>

#include <fs/fs_surface.h>
#include <fs/fs_surfaceset.h>
#include <fs/fs_annotation.h>
#include <mne/mne_bem.h>
#include <mna/mna_io.h>
#include <mna/mna_project.h>
#include <mna/mna_file_ref.h>
#include <mna/mna_subject.h>
#include <mna/mna_session.h>
#include <mna/mna_recording.h>
#include <mna/mna_types.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QToolButton>
#include <QAction>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QScrollArea>
#include <QProgressBar>
#include <QGridLayout>
#include <QTimer>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QDebug>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QAction>
#include <QSet>
#include <QSettings>
#include <QApplication>
#include <QMessageBox>
#include <QCloseEvent>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>

#include <QFormLayout>
#include <QLinearGradient>
#include <QPainter>
#include <QPaintEvent>
#include <QSignalBlocker>

#include <algorithm>
#include <cmath>

#ifdef WASMBUILD
#include <QBuffer>
#include <QCborValue>
#include <QCborMap>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace MNELIB;
using namespace MNALIB;

//=============================================================================================================
// OVERLAY COLOR BAR
//=============================================================================================================

/**
 * Lightweight colour-bar widget for the Overlay dock. Renders a horizontal
 * gradient between fmin/fmid/fmax with three tick labels. No signals.
 */
class OverlayColorBar : public QWidget
{
public:
    explicit OverlayColorBar(QWidget* parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(40);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    enum class Colormap { Sequential = 0, Divergent, HotCool };

    void setRange(float fmin, float fmid, float fmax)
    {
        m_fmin = fmin;
        m_fmid = fmid;
        m_fmax = fmax;
        update();
    }

    void setColormap(Colormap c)
    {
        m_cmap = c;
        update();
    }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        QPainter p(this);
        const QRect r = rect().adjusted(2, 2, -2, -18);
        QLinearGradient g(r.topLeft(), r.topRight());
        switch (m_cmap) {
            case Colormap::Sequential:
                g.setColorAt(0.0, QColor(20, 20, 80));
                g.setColorAt(0.5, QColor(120, 80, 180));
                g.setColorAt(1.0, QColor(255, 240, 120));
                break;
            case Colormap::Divergent:
                g.setColorAt(0.0, QColor(30, 90, 220));
                g.setColorAt(0.5, QColor(245, 245, 245));
                g.setColorAt(1.0, QColor(220, 60, 30));
                break;
            case Colormap::HotCool:
                g.setColorAt(0.0, QColor(0, 80, 200));
                g.setColorAt(0.5, QColor(0, 0, 0));
                g.setColorAt(1.0, QColor(240, 60, 0));
                break;
        }
        p.fillRect(r, g);
        p.setPen(palette().color(QPalette::Text));
        p.drawRect(r);

        const QString lo = QString::number(m_fmin, 'g', 3);
        const QString mid = QString::number(m_fmid, 'g', 3);
        const QString hi = QString::number(m_fmax, 'g', 3);
        const int y = r.bottom() + 14;
        p.drawText(r.left(), y, lo);
        p.drawText(r.center().x() - p.fontMetrics().horizontalAdvance(mid) / 2,
                   y, mid);
        p.drawText(r.right() - p.fontMetrics().horizontalAdvance(hi), y, hi);
    }

private:
    float    m_fmin = 0.0f;
    float    m_fmid = 0.5f;
    float    m_fmax = 1.0f;
    Colormap m_cmap = Colormap::Sequential;
};

//=============================================================================================================
// BIDS HELPERS
//=============================================================================================================

/**
 * Map an MnaFileRole to a BIDS-like subdirectory.
 */
static QString bidsSubdirForRole(MnaFileRole role)
{
    switch (role) {
    case MnaFileRole::Surface:
    case MnaFileRole::Annotation:
        return QStringLiteral("anat");
    case MnaFileRole::Bem:
    case MnaFileRole::SourceSpace:
        return QStringLiteral("bem");
    case MnaFileRole::Digitizer:
    case MnaFileRole::Transform:
    case MnaFileRole::Evoked:
        return QStringLiteral("meg");
    case MnaFileRole::SourceEstimate:
        return QStringLiteral("source");
    default:
        return QStringLiteral("other");
    }
}

/**
 * Build a BIDS-like relative path: sub-<subj>/ses-<sess>/<modality>/<filename>
 */
static QString bidsBuildRelPath(const QString &subjId, const QString &sessId,
                                MnaFileRole role, const QString &fileName)
{
    return QStringLiteral("sub-%1/ses-%2/%3/%4")
        .arg(subjId, sessId, bidsSubdirForRole(role), QFileInfo(fileName).fileName());
}

//=============================================================================================================
// WASM HELPERS
//=============================================================================================================

#ifdef WASMBUILD
/**
 * Write file content obtained from QFileDialog::getOpenFileContent() to a
 * temporary path in the Emscripten virtual filesystem so that existing
 * file-path-based loaders can consume it transparently.
 */
static QString wasmSaveToTemp(const QString &fileName, const QByteArray &fileContent)
{
    QString tempPath = QStringLiteral("/tmp/") + QFileInfo(fileName).fileName();
    QFile f(tempPath);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "WASM: failed to write temp file" << tempPath;
        return QString();
    }
    f.write(fileContent);
    f.close();
    return tempPath;
}
#endif

//=============================================================================================================
// HELPERS
//=============================================================================================================

/**
 * Create a flat, modern title bar widget for a QDockWidget.
 * Thin bar with uppercase label on the left and a small close button on the right.
 */
static QWidget *createFlatDockTitleBar(QDockWidget *dock, const QString &title)
{
    QWidget *bar = new QWidget(dock);
    bar->setObjectName("flatDockTitleBar");
    bar->setMinimumHeight(40);
    bar->setMaximumHeight(40);
    bar->setStyleSheet(
        "#flatDockTitleBar { background: palette(window); border-bottom: 1px solid palette(mid); padding: 0px; margin: 0px; }"
    );

    QHBoxLayout *lay = new QHBoxLayout(bar);
    lay->setContentsMargins(10, 8, 10, 8);
    lay->setSpacing(0);

    QLabel *lbl = new QLabel(title.toUpper());
    lbl->setStyleSheet(
        "font-size: 11px;"
        "font-weight: 600;"
        "letter-spacing: 0.5px;"
        "color: palette(text);"
        "background: transparent;"
        "border: none;"
    );
    lay->addWidget(lbl, 1, Qt::AlignVCenter);

    QPushButton *closeBtn = new QPushButton("\u00D7");   // multiplication sign ×
    closeBtn->setFixedSize(20, 20);
    closeBtn->setFlat(true);
    closeBtn->setCursor(Qt::ArrowCursor);
    closeBtn->setStyleSheet(
        "QPushButton { font-size: 14px; color: palette(text); background: transparent; border: none; border-radius: 3px; }"
        "QPushButton:hover { background: palette(mid); }"
    );
    QObject::connect(closeBtn, &QPushButton::clicked, dock, &QDockWidget::close);
    lay->addWidget(closeBtn, 0, Qt::AlignVCenter);

    return bar;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_scene(this)
    , m_electrodesPlugin(this)
    , m_mriSlicesPlugin(this)
    , m_pickReadout(this)
{
    setWindowTitle("MNE Inspect");

    // Wire the two inspection plugins (electrodes, MRI slices) to the shared scene before any UI
    // construction so that subsequent dock widgets / menu actions can
    // assume the scene is fully populated.
    m_electrodesPlugin.attachScene(&m_scene);
    m_mriSlicesPlugin.attachScene(&m_scene);
    m_pickReadout.attachScene(&m_scene);
    m_pickReadout.setElectrodesPlugin(&m_electrodesPlugin);
    m_pickReadout.setMriSlicesPlugin(&m_mriSlicesPlugin);

    setupUI();
    createPickDock();
    createLayersDock();
    createOverlayDock();
    createMenus();
    createStatusBar();
    setupConnections();

    restoreSettings();

    // Sync view-count combo to the value BrainView restored from its own settings.
    // This triggers currentIndexChanged which rebuilds edit-target combo, etc.
    const int restoredCount = m_brainView->viewCount();
    m_viewCountCombo->setCurrentIndex(restoredCount - 1);
}

//=============================================================================================================

void MainWindow::setupUI()
{
    // Side Panel (Controls) with Scroll Area
    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setMinimumWidth(280);
    scrollArea->setMaximumWidth(320);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *sidePanel = new QWidget;
    sidePanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sidePanel->setMaximumWidth(280);
    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(4, 4, 4, 4);
    sideLayout->setSpacing(5);
    scrollArea->setWidget(sidePanel);

    // ===== Brain Surface Group =====
    m_surfGroup = new QGroupBox("Brain Surface");
    m_surfGroup->setEnabled(false);
    QVBoxLayout *surfLayout = new QVBoxLayout(m_surfGroup);
    surfLayout->setContentsMargins(6, 12, 6, 6);
    surfLayout->setSpacing(8);

    // Surface Selector
    QLabel *surfLabel = new QLabel("Surface Type:");
    m_surfCombo = new QComboBox;
    m_surfCombo->addItems({"pial", "inflated", "white"});

    // Shader Mode
    QLabel *shaderLabel = new QLabel("Brain Shader:");
    m_shaderCombo = new QComboBox;
    m_shaderCombo->addItems({"Standard", "Holographic", "Anatomical"});

    QLabel *bemShaderLabel = new QLabel("Head Shader:");
    m_bemShaderCombo = new QComboBox;
    m_bemShaderCombo->addItems({"Standard", "Holographic", "Anatomical"});
    m_bemShaderCombo->setEnabled(false);

    m_linkShadersCheck = new QCheckBox("Link Shaders to Brain Surface");
    m_linkShadersCheck->setChecked(true);
    m_linkShadersCheck->setToolTip("When checked, Head Shader follows Brain Shader selection.");

    m_bemColorCheck = new QCheckBox("Show BEM Colors");
    m_bemColorCheck->setChecked(true);
    m_bemColorCheck->setToolTip("Toggle between standard Red/Green/Blue colors and White/Overlay colors.");

    // Overlay
    QLabel *overlayLabel = new QLabel("Overlay:");
    m_overlayCombo = new QComboBox;
    m_overlayCombo->addItems({"Surface", "Annotation", "Scientific"});

    m_lhCheck = new QCheckBox("Left Hemisphere");
    m_lhCheck->setChecked(true);

    m_rhCheck = new QCheckBox("Right Hemisphere");
    m_rhCheck->setChecked(true);

    // BEM Checks
    m_headCheck = new QCheckBox("Head Surface");
    m_headCheck->setChecked(true);
    m_outerCheck = new QCheckBox("Outer Skull");
    m_outerCheck->setChecked(true);
    m_innerCheck = new QCheckBox("Inner Skull");
    m_innerCheck->setChecked(true);

    surfLayout->addWidget(surfLabel);
    surfLayout->addWidget(m_surfCombo);
    surfLayout->addWidget(overlayLabel);
    surfLayout->addWidget(m_overlayCombo);
    surfLayout->addWidget(shaderLabel);
    surfLayout->addWidget(m_shaderCombo);
    surfLayout->addWidget(m_lhCheck);
    surfLayout->addWidget(m_rhCheck);

    // ===== BEM Surface Group =====
    m_bemGroup = new QGroupBox("BEM Surface");
    m_bemGroup->setEnabled(false);
    QVBoxLayout *bemLayout = new QVBoxLayout(m_bemGroup);
    bemLayout->setContentsMargins(6, 12, 6, 6);
    bemLayout->setSpacing(8);

    bemLayout->addWidget(m_headCheck);
    bemLayout->addWidget(m_outerCheck);
    bemLayout->addWidget(m_innerCheck);
    bemLayout->addWidget(m_bemColorCheck);
    bemLayout->addWidget(bemShaderLabel);
    bemLayout->addWidget(m_bemShaderCombo);
    bemLayout->addWidget(m_linkShadersCheck);

    // ===== Source Estimate Group =====
    m_stcGroup = new QGroupBox("Source Estimate");
    m_stcGroup->setEnabled(false);
    QVBoxLayout *stcLayout = new QVBoxLayout(m_stcGroup);
    stcLayout->setContentsMargins(6, 12, 6, 6);
    stcLayout->setSpacing(8);

    QLabel *stcSelectLabel = new QLabel("Source Estimate:");
    m_stcCombo = new QComboBox;
    m_stcCombo->setEnabled(false);
    m_stcCombo->setToolTip("Select which loaded source estimate to display");

    QLabel *colormapLabel = new QLabel("Colormap:");
    m_colormapCombo = new QComboBox;
    m_colormapCombo->addItems({"Hot", "Jet", "Viridis", "Cool", "RedBlue", "Bone"});

    m_timeLabel = new QLabel("Time: 0.000 s");
    m_timeSlider = new QSlider(Qt::Horizontal);
    m_timeSlider->setEnabled(false);

    QLabel *threshLabel = new QLabel("Thresholds:");
    m_minThresh = new QDoubleSpinBox;
    m_midThresh = new QDoubleSpinBox;
    m_maxThresh = new QDoubleSpinBox;
    m_minThresh->setDecimals(6);
    m_midThresh->setDecimals(6);
    m_maxThresh->setDecimals(6);
    m_minThresh->setRange(0, 1e12);
    m_midThresh->setRange(0, 1e12);
    m_maxThresh->setRange(0, 1e12);
    m_minThresh->setValue(0.0);
    m_midThresh->setValue(0.5);
    m_maxThresh->setValue(10.0);

    QGridLayout *threshGrid = new QGridLayout;
    threshGrid->addWidget(new QLabel("Min"), 0, 0);
    threshGrid->addWidget(m_minThresh, 0, 1);
    threshGrid->addWidget(new QLabel("Mid"), 1, 0);
    threshGrid->addWidget(m_midThresh, 1, 1);
    threshGrid->addWidget(new QLabel("Max"), 2, 0);
    threshGrid->addWidget(m_maxThresh, 2, 1);

    QLabel *playbackLabel = new QLabel("Playback:");
    QHBoxLayout *playbackLayout = new QHBoxLayout;
    m_playButton = new QPushButton("Play");
    m_speedCombo = new QComboBox;
    m_speedCombo->addItem("0.01x", 0.01);
    m_speedCombo->addItem("0.05x", 0.05);
    m_speedCombo->addItem("0.1x", 0.1);
    m_speedCombo->addItem("0.25x", 0.25);
    m_speedCombo->addItem("0.5x", 0.5);
    m_speedCombo->addItem("1.0x", 1.0);
    m_speedCombo->addItem("2.0x", 2.0);
    m_speedCombo->addItem("5.0x", 5.0);
    m_speedCombo->addItem("10.0x", 10.0);
    m_speedCombo->setCurrentIndex(2);
    playbackLayout->addWidget(m_playButton);
    playbackLayout->addWidget(m_speedCombo);

    m_realtimeCheck = new QCheckBox("Realtime Streaming");
    m_realtimeCheck->setToolTip("Use threaded real-time streaming pipeline instead of simple timer playback");
    m_loopCheck = new QCheckBox("Loop");
    m_loopCheck->setChecked(true);
    m_loopCheck->setToolTip("Loop data when the end of the stream is reached");
    QHBoxLayout *rtLayout = new QHBoxLayout;
    rtLayout->addWidget(m_realtimeCheck);
    rtLayout->addWidget(m_loopCheck);

    m_stcTimer = new QTimer(this);

    m_stcStatusLabel = new QLabel("");
    m_stcStatusLabel->setStyleSheet("color: #666; font-style: italic;");
    m_stcStatusLabel->setWordWrap(true);
    m_stcStatusLabel->hide();

    m_stcProgressBar = new QProgressBar;
    m_stcProgressBar->setRange(0, 100);
    m_stcProgressBar->setValue(0);
    m_stcProgressBar->hide();

    stcLayout->addWidget(stcSelectLabel);
    stcLayout->addWidget(m_stcCombo);
    stcLayout->addWidget(m_stcStatusLabel);
    stcLayout->addWidget(m_stcProgressBar);
    stcLayout->addWidget(colormapLabel);
    stcLayout->addWidget(m_colormapCombo);
    stcLayout->addWidget(threshLabel);
    stcLayout->addLayout(threshGrid);
    stcLayout->addWidget(playbackLabel);
    stcLayout->addLayout(playbackLayout);
    stcLayout->addLayout(rtLayout);
    stcLayout->addWidget(m_timeSlider);
    stcLayout->addWidget(m_timeLabel);
    stcLayout->addStretch();

    // ===== Dipole Group =====
    m_dipoleGroup = new QGroupBox("Dipoles");
    m_dipoleGroup->setEnabled(false);
    QVBoxLayout *dipoleLayout = new QVBoxLayout(m_dipoleGroup);
    dipoleLayout->setContentsMargins(6, 12, 6, 6);
    dipoleLayout->setSpacing(8);

    m_showDipoleCheck = new QCheckBox("Show Dipoles");
    m_showDipoleCheck->setChecked(true);
    m_showDipoleCheck->setEnabled(false);

    dipoleLayout->addWidget(m_showDipoleCheck);

    // ===== Source Space Group =====
    m_srcSpaceGroup = new QGroupBox("Source Space");
    m_srcSpaceGroup->setEnabled(false);
    QVBoxLayout *srcSpaceLayout = new QVBoxLayout(m_srcSpaceGroup);
    srcSpaceLayout->setContentsMargins(6, 12, 6, 6);
    srcSpaceLayout->setSpacing(8);

    m_showSrcSpaceCheck = new QCheckBox("Show Source Space");
    m_showSrcSpaceCheck->setChecked(false);
    m_showSrcSpaceCheck->setEnabled(false);

    srcSpaceLayout->addWidget(m_showSrcSpaceCheck);

    // ===== Connectivity Network Group =====
    m_networkGroup = new QGroupBox("Connectivity Network");
    m_networkGroup->setEnabled(false);
    QVBoxLayout *networkLayout = new QVBoxLayout(m_networkGroup);
    networkLayout->setContentsMargins(6, 12, 6, 6);
    networkLayout->setSpacing(8);

    m_showNetworkCheck = new QCheckBox("Show Network");
    m_showNetworkCheck->setChecked(false);
    m_showNetworkCheck->setEnabled(false);

    QLabel *netThreshLabel = new QLabel("Threshold:");
    m_networkThresholdSlider = new QSlider(Qt::Horizontal);
    m_networkThresholdSlider->setRange(0, 100);
    m_networkThresholdSlider->setValue(50);
    m_networkThresholdSlider->setEnabled(false);

    QLabel *netCmapLabel = new QLabel("Colormap:");
    m_networkColormapCombo = new QComboBox;
    m_networkColormapCombo->addItems({"Hot", "Jet", "Bone", "RedBlue", "Plasma"});
    m_networkColormapCombo->setEnabled(false);

    networkLayout->addWidget(m_showNetworkCheck);
    networkLayout->addWidget(netThreshLabel);
    networkLayout->addWidget(m_networkThresholdSlider);
    networkLayout->addWidget(netCmapLabel);
    networkLayout->addWidget(m_networkColormapCombo);

    // ===== Evoked Group =====
    m_evokedGroup = new QGroupBox("Evoked");
    m_evokedGroup->setEnabled(false);
    QVBoxLayout *evokedLayout = new QVBoxLayout(m_evokedGroup);
    evokedLayout->setContentsMargins(6, 12, 6, 6);
    evokedLayout->setSpacing(6);

    QLabel *evokedSetLabel = new QLabel("Evoked Set:");
    m_evokedSetCombo = new QComboBox;
    m_evokedSetCombo->setEnabled(false);
    m_evokedSetCombo->setToolTip("Select which evoked/average data set to display");

    m_showMegFieldCheck = new QCheckBox("Show MEG Field Map");
    m_showMegFieldCheck->setChecked(false);
    m_showMegFieldCheck->setEnabled(false);

    m_showEegFieldCheck = new QCheckBox("Show EEG Potential Map");
    m_showEegFieldCheck->setChecked(false);
    m_showEegFieldCheck->setEnabled(false);

    m_showMegContourCheck = new QCheckBox("Show MEG Isofield Lines");
    m_showMegContourCheck->setChecked(false);
    m_showMegContourCheck->setEnabled(false);

    m_showEegContourCheck = new QCheckBox("Show EEG Equipotential Lines");
    m_showEegContourCheck->setChecked(false);
    m_showEegContourCheck->setEnabled(false);

    QLabel *megSurfLabel = new QLabel("MEG Surface:");
    m_megHelmetCombo = new QComboBox;
    m_megHelmetCombo->addItems({"Helmet", "Head"});
    m_megHelmetCombo->setToolTip("Choose whether MEG field mapping uses the helmet or head surface.");
    m_megHelmetCombo->setEnabled(false);

    m_sensorFieldTimeSlider = new QSlider(Qt::Horizontal);
    m_sensorFieldTimeSlider->setRange(0, 0);
    m_sensorFieldTimeSlider->setValue(0);
    m_sensorFieldTimeSlider->setEnabled(false);

    m_sensorFieldTimeLabel = new QLabel("Field Time: 0.000 s");

    m_syncTimesCheck = new QCheckBox("Sync STC \u2194 Evoked");
    m_syncTimesCheck->setChecked(true);
    m_syncTimesCheck->setToolTip("Synchronize STC and sensor field time points");
    m_syncTimesCheck->setEnabled(false);

    evokedLayout->addWidget(evokedSetLabel);
    evokedLayout->addWidget(m_evokedSetCombo);
    evokedLayout->addWidget(m_showMegFieldCheck);
    evokedLayout->addWidget(m_showEegFieldCheck);
    evokedLayout->addWidget(m_showMegContourCheck);
    evokedLayout->addWidget(m_showEegContourCheck);
    evokedLayout->addWidget(megSurfLabel);
    evokedLayout->addWidget(m_megHelmetCombo);
    evokedLayout->addWidget(m_sensorFieldTimeLabel);
    evokedLayout->addWidget(m_sensorFieldTimeSlider);
    evokedLayout->addWidget(m_syncTimesCheck);

    // ===== Sensor Streaming Group =====
    m_sensorStreamGroup = new QGroupBox("Sensor Streaming");
    m_sensorStreamGroup->setEnabled(false);
    QVBoxLayout *sensorStreamLayout = new QVBoxLayout(m_sensorStreamGroup);
    sensorStreamLayout->setContentsMargins(6, 12, 6, 6);
    sensorStreamLayout->setSpacing(6);

    QLabel *ssModalityLabel = new QLabel("Modality:");
    m_sensorStreamModalityCombo = new QComboBox;
    m_sensorStreamModalityCombo->addItems({"MEG", "EEG"});
    m_sensorStreamModalityCombo->setEnabled(false);

    m_sensorStreamBtn = new QPushButton("Start Sensor Streaming");
    m_sensorStreamBtn->setEnabled(false);

    m_sensorStreamLoopCheck = new QCheckBox("Loop");
    m_sensorStreamLoopCheck->setChecked(true);
    m_sensorStreamLoopCheck->setEnabled(false);

    QLabel *ssAvgLabel = new QLabel("Averages:");
    m_sensorStreamAvgSpin = new QSpinBox;
    m_sensorStreamAvgSpin->setRange(1, 100);
    m_sensorStreamAvgSpin->setValue(1);
    m_sensorStreamAvgSpin->setEnabled(false);

    QLabel *ssCmapLabel = new QLabel("Colormap:");
    m_sensorStreamColormapCombo = new QComboBox;
    m_sensorStreamColormapCombo->addItems({"MNE", "Hot", "Jet", "Viridis", "Cool", "RedBlue"});
    m_sensorStreamColormapCombo->setEnabled(false);

    sensorStreamLayout->addWidget(ssModalityLabel);
    sensorStreamLayout->addWidget(m_sensorStreamModalityCombo);
    sensorStreamLayout->addWidget(m_sensorStreamBtn);
    sensorStreamLayout->addWidget(m_sensorStreamLoopCheck);
    sensorStreamLayout->addWidget(ssAvgLabel);
    sensorStreamLayout->addWidget(m_sensorStreamAvgSpin);
    sensorStreamLayout->addWidget(ssCmapLabel);
    sensorStreamLayout->addWidget(m_sensorStreamColormapCombo);

    // ===== Sensor Group =====
    m_sensorGroup = new QGroupBox("Sensors");
    m_sensorGroup->setEnabled(false);
    QVBoxLayout *sensorLayout = new QVBoxLayout(m_sensorGroup);
    sensorLayout->setContentsMargins(6, 12, 6, 6);
    sensorLayout->setSpacing(8);

    m_showMegCheck = new QCheckBox("Show MEG");
    m_showMegCheck->setChecked(false);
    m_showMegCheck->setEnabled(false);

    m_showEegCheck = new QCheckBox("Show EEG");
    m_showEegCheck->setChecked(false);
    m_showEegCheck->setEnabled(false);

    m_showDigCheck = new QCheckBox("Show Digitizer");
    m_showDigCheck->setChecked(false);
    m_showDigCheck->setEnabled(false);

    // Per-category digitizer visibility
    m_showDigCardinalCheck = new QCheckBox("  Cardinal (NAS/LPA/RPA)");
    m_showDigCardinalCheck->setChecked(false);
    m_showDigCardinalCheck->setEnabled(false);

    m_showDigHpiCheck = new QCheckBox("  HPI Coils");
    m_showDigHpiCheck->setChecked(false);
    m_showDigHpiCheck->setEnabled(false);

    m_showDigEegCheck = new QCheckBox("  EEG Points");
    m_showDigEegCheck->setChecked(false);
    m_showDigEegCheck->setEnabled(false);

    m_showDigExtraCheck = new QCheckBox("  Head Shape");
    m_showDigExtraCheck->setChecked(false);
    m_showDigExtraCheck->setEnabled(false);

    m_showHelmetCheck = new QCheckBox("Show MEG Helmet");
    m_showHelmetCheck->setChecked(false);
    m_showHelmetCheck->setEnabled(false);

    m_helmetShapeCombo = new QComboBox;
    m_helmetShapeCombo->blockSignals(true);   // prevent premature loadMegHelmetSurface during addItems
    m_helmetShapeCombo->addItems({"306m", "122m", "306m RT", "BabyMEG",
                                  "BabySQUID", "CTF 275", "KIT", "Magnes 2500WH", "Magnes 3600WH"});
    m_helmetShapeCombo->blockSignals(false);
    m_helmetShapeCombo->setToolTip("Select MEG helmet shape to display.");
    m_helmetShapeCombo->setEnabled(false);

    m_applyTransCheck = new QCheckBox("Apply Transformation");
    m_applyTransCheck->setChecked(true);
    m_applyTransCheck->setToolTip("Apply Head-to-MRI transformation to sensors if available.");

    sensorLayout->addWidget(m_showMegCheck);
    sensorLayout->addWidget(m_showEegCheck);
    sensorLayout->addWidget(m_showDigCheck);
    sensorLayout->addWidget(m_showDigCardinalCheck);
    sensorLayout->addWidget(m_showDigHpiCheck);
    sensorLayout->addWidget(m_showDigEegCheck);
    sensorLayout->addWidget(m_showDigExtraCheck);
    sensorLayout->addWidget(m_showHelmetCheck);
    sensorLayout->addWidget(m_helmetShapeCombo);
    sensorLayout->addWidget(m_applyTransCheck);

    // ===== View Group =====
    QGroupBox *viewGroup = new QGroupBox("View");
    QVBoxLayout *viewLayout = new QVBoxLayout(viewGroup);
    viewLayout->setContentsMargins(6, 12, 6, 6);
    viewLayout->setSpacing(8);

    // Info Panel Toggle
    m_showInfoCheck = new QCheckBox("Show Info Labels");
    m_showInfoCheck->setChecked(true);
    m_showInfoCheck->setToolTip("Show or hide overlay info labels (FPS, shader, surface info).");
    viewLayout->addWidget(m_showInfoCheck);

    // View Count
    QLabel *viewCountLabel = new QLabel("Layout:");
    m_viewCountCombo = new QComboBox();
    m_viewCountCombo->addItems({"Single View", "2 Views", "3 Views", "4 Views"});
    m_viewCountCombo->setCurrentIndex(0);
    m_viewCountCombo->setToolTip("Number of viewport panes (1 = single, 2 = side-by-side, 3 = two + one, 4 = 2×2 grid).");
    QHBoxLayout *viewCountRow = new QHBoxLayout();
    viewCountRow->addWidget(viewCountLabel);
    viewCountRow->addWidget(m_viewCountCombo);
    viewLayout->addLayout(viewCountRow);

    // Edit Target
    m_editTargetLabel = new QLabel("Edit Target:");
    m_editTargetCombo = new QComboBox();
    m_editTargetCombo->addItem("All Viewports");
    m_editTargetCombo->setToolTip("Select which viewport the surface, shader, and overlay controls apply to.");
    m_editTargetCombo->setEnabled(false);
    QHBoxLayout *editTargetRow = new QHBoxLayout();
    editTargetRow->addWidget(m_editTargetLabel);
    editTargetRow->addWidget(m_editTargetCombo);
    viewLayout->addLayout(editTargetRow);

    // Camera Preset (per-viewport)
    m_cameraPresetLabel = new QLabel("Camera:");
    m_cameraPresetCombo = new QComboBox();
    m_cameraPresetCombo->addItems({"Top", "Perspective", "Front", "Left", "Bottom", "Back", "Right"});
    m_cameraPresetCombo->setCurrentIndex(1);  // Perspective
    m_cameraPresetCombo->setToolTip("Camera orientation preset for the selected viewport.");
    m_cameraPresetCombo->setEnabled(false);
    QHBoxLayout *presetRow = new QHBoxLayout();
    presetRow->addWidget(m_cameraPresetLabel);
    presetRow->addWidget(m_cameraPresetCombo);
    viewLayout->addLayout(presetRow);

    // Assemble side panel
    sideLayout->addWidget(m_surfGroup);
    sideLayout->addWidget(m_bemGroup);
    sideLayout->addWidget(m_stcGroup);
    sideLayout->addWidget(m_dipoleGroup);
    sideLayout->addWidget(m_srcSpaceGroup);
    sideLayout->addWidget(m_networkGroup);
    sideLayout->addWidget(m_evokedGroup);
    sideLayout->addWidget(m_sensorStreamGroup);
    sideLayout->addWidget(m_sensorGroup);
    sideLayout->addWidget(viewGroup);
    sideLayout->addStretch();

    // ===== Brain View =====
    m_brainView = new BrainView;
    m_model = new BrainTreeModel(m_brainView);
    m_brainView->setModel(m_model);

    // Wrap BrainView + per-viewport time strips in a vertical layout
    QWidget *viewContainer = new QWidget;
    QVBoxLayout *viewContainerLayout = new QVBoxLayout(viewContainer);
    viewContainerLayout->setContentsMargins(0, 0, 0, 0);
    viewContainerLayout->setSpacing(0);
    viewContainerLayout->addWidget(m_brainView, 1);

    // ===== Controls Dock Widget =====
    m_controlsDock = new QDockWidget("Controls", this);
    m_controlsDock->setObjectName("controlsDock");
    m_controlsDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    m_controlsDock->setTitleBarWidget(createFlatDockTitleBar(m_controlsDock, "Controls"));
    m_controlsDock->setWidget(scrollArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_controlsDock);

    setCentralWidget(viewContainer);

    // ===== Loaded Files Dock Widget =====
    createLoadedFilesDock();
}

//=============================================================================================================

void MainWindow::setupConnections()
{
    // ── Project ────────────────────────────────────────────────────────

    connect(m_actOpenProject, &QAction::triggered, [this]() {
#ifdef WASMBUILD
        QFileDialog::getOpenFileContent("MNA Project (*.mnx)", [this](const QString &fileName, const QByteArray &fileContent) {
            if (fileName.isEmpty()) return;
            QString path = wasmSaveToTemp(fileName, fileContent);
            if (path.isEmpty()) return;
            importMnaProject(path);
        });
#else
        QString path = QFileDialog::getOpenFileName(this, "Open MNA Project", "",
            "MNA Project Files (*.mna *.mnx);;All Files (*)");
        if (path.isEmpty()) return;
        importMnaProject(path);
#endif
    });

    connect(m_actExportProject, &QAction::triggered, [this]() {
#ifdef WASMBUILD
        // WASM always exports as .mnx with embedded data
        MnaProject proj;
        proj.name = QStringLiteral("MNE Inspect Export");
        proj.description = QStringLiteral("Exported from MNE Inspect (WASM)");
        proj.modified = QDateTime::currentDateTimeUtc();

        MnaSubject subj;
        subj.id = QStringLiteral("User");
        MnaSession session;
        session.id = QStringLiteral("session-01");
        MnaRecording rec;
        rec.id = QStringLiteral("recording-01");

        for (const auto &entry : m_loadedFiles) {
            MnaFileRef ref;
            ref.role = static_cast<MnaFileRole>(entry.second);
            ref.path = bidsBuildRelPath(subj.id, session.id, ref.role, entry.first);
            ref.format = QFileInfo(entry.first).suffix();
            ref.embedded = true;

            QFile f(entry.first);
            if (f.open(QIODevice::ReadOnly)) {
                ref.data = f.readAll();
                ref.sizeBytes = ref.data.size();
                f.close();
            }
            rec.files.append(ref);
        }

        session.recordings.append(rec);
        subj.sessions.append(session);
        proj.subjects.append(subj);

        // Serialize to CBOR in memory
        const QByteArray magic = QByteArrayLiteral("MNX1");
        QCborValue cborVal(proj.toCbor());
        QByteArray cborData = cborVal.toCbor();
        QByteArray out = magic + cborData;

        QFileDialog::saveFileContent(out, "project.mnx");
#else
        QString path = QFileDialog::getSaveFileName(this, "Export MNA Project", "project.mnx",
            "MNX Binary (*.mnx);;MNA JSON (*.mna);;All Files (*)");
        if (path.isEmpty()) return;
        bool embed = path.endsWith(QLatin1String(".mnx"), Qt::CaseInsensitive);
        exportMnaProject(path, embed);
#endif
    });

    // Surface type
    connect(m_surfCombo, &QComboBox::currentTextChanged, m_brainView, &BrainView::setActiveSurface);

    // Inflated surface logic
    connect(m_surfCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        bool isInflated = (text == "inflated");
        if (isInflated) {
            m_headCheck->setChecked(false);
            m_outerCheck->setChecked(false);
            m_innerCheck->setChecked(false);
        }
        m_headCheck->setEnabled(!isInflated);
        m_outerCheck->setEnabled(!isInflated);
        m_innerCheck->setEnabled(!isInflated);
    });

    // Shaders
    connect(m_shaderCombo, &QComboBox::currentTextChanged, m_brainView, &BrainView::setShaderMode);
    connect(m_shaderCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        if (m_linkShadersCheck->isChecked()) {
            m_bemShaderCombo->setCurrentText(text);
        }
    });
    connect(m_bemShaderCombo, &QComboBox::currentTextChanged, m_brainView, &BrainView::setBemShaderMode);
    connect(m_linkShadersCheck, &QCheckBox::toggled, [this](bool checked) {
        m_bemShaderCombo->setEnabled(!checked);
        if (checked) {
            m_bemShaderCombo->setCurrentText(m_shaderCombo->currentText());
        }
    });

    // Overlay
    connect(m_overlayCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        m_brainView->setVisualizationMode(text);
        if (text != "Source Estimate" && m_stcTimer->isActive()) {
            m_stcTimer->stop();
            m_playButton->setText("Play");
        }
    });

    // Hemisphere visibility
    connect(m_lhCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setHemiVisible(0, checked); });
    connect(m_rhCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setHemiVisible(1, checked); });

    // BEM loading
    connect(m_actLoadBem, &QAction::triggered, [this]() {
#ifdef WASMBUILD
        QFileDialog::getOpenFileContent("BEM Files (*.fif)", [this](const QString &fileName, const QByteArray &fileContent) {
            if (fileName.isEmpty()) return;
            QString path = wasmSaveToTemp(fileName, fileContent);
            if (path.isEmpty()) return;
            loadBem("User", path);
        });
#else
        QString path = QFileDialog::getOpenFileName(this, "Select BEM Surface", "", "BEM Files (*-bem.fif *-bem-sol.fif);;FIF Files (*.fif);;All Files (*)");
        if (path.isEmpty()) return;
        loadBem("User", path);
#endif
    });

    // BEM visibility
    connect(m_headCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setBemVisible("head", checked); });
    connect(m_outerCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setBemVisible("outer_skull", checked); });
    connect(m_innerCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setBemVisible("inner_skull", checked); });
    connect(m_bemColorCheck, &QCheckBox::toggled, m_brainView, &BrainView::setBemHighContrast);

    // Multi-view: view count
    connect(m_viewCountCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        const int count = index + 1;
        m_brainView->setViewCount(count);
        updateViewportCheckboxes(count);

        const bool isMulti = (count > 1);
        m_editTargetCombo->setEnabled(isMulti);
        m_cameraPresetCombo->setEnabled(isMulti);

        // Rebuild the edit-target combo items
        m_editTargetCombo->blockSignals(true);
        m_editTargetCombo->clear();
        if (isMulti) {
            for (int i = 0; i < count; ++i) {
                const int preset = i;  // defaultForIndex assigns preset = index
                m_editTargetCombo->addItem(multiViewPresetName(preset), i);
            }
            m_editTargetCombo->setCurrentIndex(0);
            m_brainView->setVisualizationEditTarget(0);
            syncUIToEditTarget(0);
        } else {
            m_editTargetCombo->addItem("All Viewports");
            m_brainView->setVisualizationEditTarget(-1);
            syncUIToEditTarget(-1);
        }
        m_editTargetCombo->blockSignals(false);
    });

    // Multi-view: edit target selection
    connect(m_editTargetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        if (m_brainView->viewMode() == BrainView::SingleView) return;
        const int target = m_editTargetCombo->itemData(index).toInt();
        m_brainView->setVisualizationEditTarget(target);
        syncUIToEditTarget(target);
    });

    // Multi-view: camera preset for the selected viewport
    connect(m_cameraPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int presetIndex) {
        if (m_brainView->viewMode() == BrainView::SingleView) return;
        const int target = m_brainView->visualizationEditTarget();
        if (target >= 0) {
            m_brainView->setViewportCameraPreset(target, presetIndex);
        }
    });

    // Multi-view: bidirectional sync when user clicks viewport labels inside BrainView
    connect(m_brainView, &BrainView::visualizationEditTargetChanged, [this](int target) {
        if (target < 0) return;
        // Update combo without re-triggering the connection
        m_editTargetCombo->blockSignals(true);
        for (int i = 0; i < m_editTargetCombo->count(); ++i) {
            if (m_editTargetCombo->itemData(i).toInt() == target) {
                m_editTargetCombo->setCurrentIndex(i);
                break;
            }
        }
        m_editTargetCombo->blockSignals(false);
        syncUIToEditTarget(target);
    });

    // Brain Surface
    connect(m_actLoadSurface, &QAction::triggered, [this]() {
#ifdef WASMBUILD
        QFileDialog::getOpenFileContent("FreeSurfer Surface (*.*)", [this](const QString &fileName, const QByteArray &fileContent) {
            if (fileName.isEmpty()) return;
            QString path = wasmSaveToTemp(fileName, fileContent);
            if (path.isEmpty()) return;

            QString fn = QFileInfo(path).fileName();
            QString hemi = fn.contains("lh.") ? "lh" : (fn.contains("rh.") ? "rh" : "lh");
            QString type = "pial";
            if (fn.contains("inflated")) type = "inflated";
            else if (fn.contains("white")) type = "white";
            else if (fn.contains("orig")) type = "orig";

            FsSurface surf(path);
            if (!surf.isEmpty()) {
                m_model->addSurface("User", hemi, type, surf);
                m_surfGroup->setEnabled(true);
                trackLoadedFile(path, static_cast<int>(MnaFileRole::Surface));
                qInfo() << "Loaded surface:" << hemi << type
                         << "(load the opposite hemisphere separately in browser mode)";
            }
        });
#else
        QString path = QFileDialog::getOpenFileName(this, "Select Surface", "", "FreeSurfer Surface (*.pial *.inflated *.white *.orig);;All Files (*)");
        if (path.isEmpty()) return;

        // Guess hemi and type
        QString fileName = QFileInfo(path).fileName();
        QString hemi = fileName.contains("lh.") ? "lh" : (fileName.contains("rh.") ? "rh" : "lh");
        QString type = "pial";
        if (fileName.contains("inflated")) type = "inflated";
        else if (fileName.contains("white")) type = "white";
        else if (fileName.contains("orig")) type = "orig";

        FsSurface surf(path);
        if (!surf.isEmpty()) {
            m_model->addSurface("User", hemi, type, surf);
            m_surfGroup->setEnabled(true);
            trackLoadedFile(path, static_cast<int>(MnaFileRole::Surface));
            qInfo() << "Loaded surface:" << hemi << type;
        }

        // Auto-load opposite hemisphere if available
        QString otherHemi = (hemi == "lh") ? "rh" : "lh";
        QString otherPath = QFileInfo(path).absolutePath() + "/" + otherHemi + "." + type;
        if (QFile::exists(otherPath)) {
            FsSurface otherSurf(otherPath);
            if (!otherSurf.isEmpty()) {
                m_model->addSurface("User", otherHemi, type, otherSurf);
                trackLoadedFile(otherPath, static_cast<int>(MnaFileRole::Surface));
                qInfo() << "Auto-loaded opposite hemisphere:" << otherHemi << type;
            }
        }
#endif
    });

    connect(m_actLoadAtlas, &QAction::triggered, [this]() {
#ifdef WASMBUILD
        QFileDialog::getOpenFileContent("FreeSurfer Annotation (*.annot)", [this](const QString &fileName, const QByteArray &fileContent) {
            if (fileName.isEmpty()) return;
            QString path = wasmSaveToTemp(fileName, fileContent);
            if (path.isEmpty()) return;

            QString fn = QFileInfo(path).fileName();
            QString hemi = fn.contains("lh.") ? "lh" : (fn.contains("rh.") ? "rh" : "lh");

            FsAnnotation annot(path);
            if (!annot.isEmpty()) {
                m_model->addAnnotation("User", hemi, annot);
                trackLoadedFile(path, static_cast<int>(MnaFileRole::Annotation));
            }
        });
#else
        QString path = QFileDialog::getOpenFileName(this, "Select Atlas", "", "FreeSurfer Annotation (*.annot);;All Files (*)");
        if (path.isEmpty()) return;

        // Guess hemi
        QString fileName = QFileInfo(path).fileName();
        QString hemi = fileName.contains("lh.") ? "lh" : (fileName.contains("rh.") ? "rh" : "lh");

        FsAnnotation annot(path);
        if (!annot.isEmpty()) {
            m_model->addAnnotation("User", hemi, annot);
            trackLoadedFile(path, static_cast<int>(MnaFileRole::Annotation));
            qInfo() << "Loaded annotation:" << hemi;
        }

        // Auto-load opposite hemisphere annotation if available
        QString otherHemi = (hemi == "lh") ? "rh" : "lh";
        QString otherPath = QFileInfo(path).absolutePath() + "/"
            + fileName.replace(hemi + ".", otherHemi + ".");
        if (QFile::exists(otherPath)) {
            FsAnnotation otherAnnot(otherPath);
            if (!otherAnnot.isEmpty()) {
                m_model->addAnnotation("User", otherHemi, otherAnnot);
                trackLoadedFile(otherPath, static_cast<int>(MnaFileRole::Annotation));
                qInfo() << "Auto-loaded opposite hemisphere annotation:" << otherHemi;
            }
        }
#endif
    });

    // STC loading – add file to combo, which triggers the actual load
    connect(m_actLoadStc, &QAction::triggered, [this]() {
#ifdef WASMBUILD
        QFileDialog::getOpenFileContent("STC Files (*.stc)", [this](const QString &fileName, const QByteArray &fileContent) {
            if (fileName.isEmpty()) return;
            QString path = wasmSaveToTemp(fileName, fileContent);
            if (path.isEmpty()) return;
            trackLoadedFile(path, static_cast<int>(MnaFileRole::SourceEstimate));
            addStcEntry(path, true);
        });
#else
        QString path = QFileDialog::getOpenFileName(this, "Select Source Estimate", "", "STC Files (*-lh.stc *-rh.stc *.stc)");
        if (path.isEmpty()) return;
        trackLoadedFile(path, static_cast<int>(MnaFileRole::SourceEstimate));
        addStcEntry(path, true);
#endif
    });

    // STC combo selection changed – load the selected STC pair
    connect(m_stcCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        if (index < 0) return;

        QStringList pair = m_stcCombo->itemData(index).toStringList();
        if (pair.size() < 2) return;
        QString lhPath = pair[0];
        QString rhPath = pair[1];

        m_actLoadStc->setEnabled(false);
        m_stcStatusLabel->setText("Starting...");
        m_stcStatusLabel->show();
        m_stcProgressBar->setValue(0);
        m_stcProgressBar->show();

        m_brainView->loadSourceEstimate(lhPath, rhPath);
    });

    connect(m_brainView, &BrainView::stcLoadingProgress, [this](int percent, const QString &message) {
        m_stcProgressBar->setValue(percent);
        m_stcStatusLabel->setText(message);
    });

    connect(m_brainView, &BrainView::sourceEstimateLoaded, [this](int numPoints) {
        m_stcStatusLabel->hide();
        m_stcProgressBar->hide();
        m_actLoadStc->setEnabled(true);
        m_stcGroup->setEnabled(true);

        m_timeSlider->setEnabled(true);
        m_timeSlider->setRange(0, numPoints - 1);
        m_timeSlider->setValue(0);

        // Add Source Estimate option if not present
        if (m_overlayCombo->findText("Source Estimate") == -1) {
            m_overlayCombo->addItem("Source Estimate");
        }
        m_overlayCombo->setCurrentText("Source Estimate");

        m_playButton->setText("Play");
        m_stcTimer->stop();

        // Use a fixed display-rate interval; the wall-clock elapsed-time
        // approach in the timeout handler dynamically computes the correct
        // number of frames to advance for any speed factor.
        m_stcTimer->setInterval(16);  // ~60 fps
    });

    connect(m_timeSlider, &QSlider::valueChanged, [this](int value) {
        m_brainView->setTimePoint(value);
    });
    connect(m_brainView, &BrainView::timePointChanged, [this](int /*index*/, float time) {
        m_timeLabel->setText(QString("Time: %1 s").arg(time, 0, 'f', 3));

        // Sync sensor field time if enabled (skip if already syncing from evoked)
        if (!m_isSyncing && m_syncTimesCheck->isChecked() && m_sensorFieldTimeSlider->isEnabled()) {
            m_isSyncing = true;
            int sfIdx = m_brainView->closestSensorFieldIndex(time);
            if (sfIdx >= 0) {
                m_sensorFieldTimeSlider->blockSignals(true);
                m_sensorFieldTimeSlider->setValue(sfIdx);
                m_sensorFieldTimeSlider->blockSignals(false);
                m_brainView->setSensorFieldTimePoint(sfIdx);
            }
            m_isSyncing = false;
        }
    });

    connect(m_colormapCombo, &QComboBox::currentTextChanged, m_brainView, &BrainView::setSourceColormap);

    auto updateThresholds = [this]() {
        m_brainView->setSourceThresholds(
            static_cast<float>(m_minThresh->value()),
            static_cast<float>(m_midThresh->value()),
            static_cast<float>(m_maxThresh->value())
        );
    };
    connect(m_minThresh, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateThresholds);
    connect(m_midThresh, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateThresholds);
    connect(m_maxThresh, QOverload<double>::of(&QDoubleSpinBox::valueChanged), updateThresholds);

    // Auto-thresholds from loaded STC data
    connect(m_brainView, &BrainView::sourceThresholdsUpdated, [this](float min, float mid, float max) {
        m_minThresh->blockSignals(true);
        m_midThresh->blockSignals(true);
        m_maxThresh->blockSignals(true);
        m_minThresh->setValue(static_cast<double>(min));
        m_midThresh->setValue(static_cast<double>(mid));
        m_maxThresh->setValue(static_cast<double>(max));
        m_minThresh->blockSignals(false);
        m_midThresh->blockSignals(false);
        m_maxThresh->blockSignals(false);
    });

    // Playback
    connect(m_playButton, &QPushButton::clicked, [this]() {
        if (m_realtimeCheck->isChecked()) {
            // Real-time streaming mode
            if (m_brainView->isRealtimeStreaming()) {
                m_brainView->stopRealtimeStreaming();
                m_playButton->setText("Play");
            } else {
                if (m_overlayCombo->currentText() != "Source Estimate") {
                    m_overlayCombo->setCurrentText("Source Estimate");
                }
                m_brainView->startRealtimeStreaming();
                m_playButton->setText("Pause");
            }
        } else {
            // Simple timer mode
            if (m_stcTimer->isActive()) {
                m_stcTimer->stop();
                m_playButton->setText("Play");
            } else {
                if (m_overlayCombo->currentText() != "Source Estimate") {
                    m_overlayCombo->setCurrentText("Source Estimate");
                }
                m_stcStepAccum = 0.0;
                m_playbackClock.start();
                m_stcTimer->start();
                m_playButton->setText("Pause");
            }
        }
    });

    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        // Reset accumulator so the new speed takes effect cleanly
        m_stcStepAccum = 0.0;
        m_playbackClock.start();

        // Update real-time streaming interval for the threaded pipeline
        float tstep = m_brainView->stcStep();
        if (tstep > 0) {
            double factor = m_speedCombo->currentData().toDouble();
            int interval = static_cast<int>((tstep * 1000.0f) / factor);
            m_brainView->setRealtimeInterval(interval);
        }
    });

    connect(m_loopCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setRealtimeLooping(checked);
    });

    connect(m_realtimeCheck, &QCheckBox::toggled, [this](bool checked) {
        // If switching modes while playing, stop active playback
        if (checked && m_stcTimer->isActive()) {
            m_stcTimer->stop();
            m_playButton->setText("Play");
        } else if (!checked && m_brainView->isRealtimeStreaming()) {
            m_brainView->stopRealtimeStreaming();
            m_playButton->setText("Play");
        }
    });

    connect(m_stcTimer, &QTimer::timeout, [this]() {
        float tstep = m_brainView->stcStep();
        if (tstep <= 0) return;

        // Measure actual elapsed wall-clock time since last tick
        double elapsedMs = m_playbackClock.elapsed();
        m_playbackClock.start();

        double factor = m_speedCombo->currentData().toDouble();
        // How many samples worth of data time elapsed
        double samplesElapsed = (elapsedMs * factor) / (tstep * 1000.0);

        // Single accumulator, advance main slider
        m_stcStepAccum += samplesElapsed;
        int steps = static_cast<int>(m_stcStepAccum);
        if (steps < 1) return;
        m_stcStepAccum -= steps;

        int cur = m_timeSlider->value();
        int maxVal = m_timeSlider->maximum();
        int next = cur + steps;
        if (next > maxVal) {
            if (m_loopCheck->isChecked()) {
                next = next % (maxVal + 1);
            } else {
                next = maxVal;
                m_stcTimer->stop();
                m_playButton->setText("Play");
            }
        }
        m_timeSlider->setValue(next);
    });

    // Sensors
    connect(m_actLoadDigitizer, &QAction::triggered, [this]() {
#ifdef WASMBUILD
        QFileDialog::getOpenFileContent("FIF Files (*.fif)", [this](const QString &fileName, const QByteArray &fileContent) {
            if (fileName.isEmpty()) return;
            QString path = wasmSaveToTemp(fileName, fileContent);
            if (path.isEmpty()) return;

            if (m_brainView->loadSensors(path)) {
                trackLoadedFile(path, static_cast<int>(MnaFileRole::Digitizer));
                m_sensorGroup->setEnabled(true);
                m_showMegCheck->setEnabled(true);
                m_showEegCheck->setEnabled(true);
                m_showDigCheck->setEnabled(true);
                m_showHelmetCheck->setEnabled(true);
                m_helmetShapeCombo->setEnabled(true);
            }
        });
#else
        QString path = QFileDialog::getOpenFileName(this, "Select Sensor/Digitizer File", "", "FIF Files (*.fif)");
        if (path.isEmpty()) return;

        if (m_brainView->loadSensors(path)) {
            trackLoadedFile(path, static_cast<int>(MnaFileRole::Digitizer));
            m_sensorGroup->setEnabled(true);
            m_showMegCheck->setEnabled(true);
            m_showEegCheck->setEnabled(true);
            m_showDigCheck->setEnabled(true);
            m_showHelmetCheck->setEnabled(true);
            m_helmetShapeCombo->setEnabled(true);
            // Sub-category checkboxes stay disabled until master is toggled on
        }
#endif
    });

    connect(m_actLoadTransform, &QAction::triggered, [this]() {
#ifdef WASMBUILD
        QFileDialog::getOpenFileContent("FIF Files (*.fif)", [this](const QString &fileName, const QByteArray &fileContent) {
            if (fileName.isEmpty()) return;
            QString path = wasmSaveToTemp(fileName, fileContent);
            if (path.isEmpty()) return;
            m_brainView->loadTransformation(path);
            trackLoadedFile(path, static_cast<int>(MnaFileRole::Transform));
        });
#else
        QString path = QFileDialog::getOpenFileName(this, "Select Transformation", "", "FIF Files (*.fif)");
        if (path.isEmpty()) return;
        m_brainView->loadTransformation(path);
        trackLoadedFile(path, static_cast<int>(MnaFileRole::Transform));
#endif
    });

    connect(m_showMegCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("MEG", checked); });
    connect(m_showEegCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("EEG", checked); });
    connect(m_showDigCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setSensorVisible("Digitizer", checked);
        // Enable/disable sub-category checkboxes based on master toggle
        m_showDigCardinalCheck->setEnabled(checked);
        m_showDigHpiCheck->setEnabled(checked);
        m_showDigEegCheck->setEnabled(checked);
        m_showDigExtraCheck->setEnabled(checked);
        // When master is turned on, check all sub-categories; when off, uncheck them
        m_showDigCardinalCheck->setChecked(checked);
        m_showDigHpiCheck->setChecked(checked);
        m_showDigEegCheck->setChecked(checked);
        m_showDigExtraCheck->setChecked(checked);
    });
    connect(m_showDigCardinalCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("Digitizer/Cardinal", checked); });
    connect(m_showDigHpiCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("Digitizer/HPI", checked); });
    connect(m_showDigEegCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("Digitizer/EEG", checked); });
    connect(m_showDigExtraCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("Digitizer/Extra", checked); });
    connect(m_showHelmetCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSensorVisible("MEG Helmet", checked); });

    // Helmet shape selection
    connect(m_helmetShapeCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        // Map combo text to helmet filename
        static const QMap<QString, QString> helmetFileMap = {
            {"122m",           "122m.fif"},
            {"306m",           "306m.fif"},
            {"306m RT",        "306m_rt.fif"},
            {"BabyMEG",        "BabyMEG.fif"},
            {"BabySQUID",      "BabySQUID.fif"},
            {"CTF 275",        "CTF_275.fif"},
            {"KIT",            "KIT.fif"},
            {"Magnes 2500WH",  "Magnes_2500wh.fif"},
            {"Magnes 3600WH",  "Magnes_3600wh.fif"}
        };

        QString fileName = helmetFileMap.value(text, "306m.fif");
        QString helmetPath = QCoreApplication::applicationDirPath()
            + "/../resources/general/sensorSurfaces/" + fileName;

        m_brainView->setMegHelmetOverride(helmetPath);
        m_brainView->loadMegHelmetSurface(helmetPath);
    });

    connect(m_applyTransCheck, &QCheckBox::toggled, m_brainView, &BrainView::setSensorTransEnabled);

    // Dipoles
    connect(m_actLoadDipoles, &QAction::triggered, [this]() {
#ifdef WASMBUILD
        QFileDialog::getOpenFileContent("Dipole Files (*.dip *.bdip)", [this](const QString &fileName, const QByteArray &fileContent) {
            if (fileName.isEmpty()) return;
            QString path = wasmSaveToTemp(fileName, fileContent);
            if (path.isEmpty()) return;
            if (m_brainView->loadDipoles(path)) {
                m_dipoleGroup->setEnabled(true);
                m_showDipoleCheck->setEnabled(true);
                trackLoadedFile(path, static_cast<int>(MnaFileRole::Custom));
            }
        });
#else
        QString path = QFileDialog::getOpenFileName(this, "Select Dipoles", "", "Dipole Files (*.dip *.bdip)");
        if (path.isEmpty()) return;
        if (m_brainView->loadDipoles(path)) {
            m_dipoleGroup->setEnabled(true);
            m_showDipoleCheck->setEnabled(true);
            trackLoadedFile(path, static_cast<int>(MnaFileRole::Custom));
        }
#endif
    });

    connect(m_showDipoleCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setDipoleVisible(checked); });

    // Source Space
    connect(m_actLoadSrcSpace, &QAction::triggered, [this]() {
#ifdef WASMBUILD
        QFileDialog::getOpenFileContent("Source Space Files (*.fif)", [this](const QString &fileName, const QByteArray &fileContent) {
            if (fileName.isEmpty()) return;
            QString path = wasmSaveToTemp(fileName, fileContent);
            if (path.isEmpty()) return;
            if (m_brainView->loadSourceSpace(path)) {
                trackLoadedFile(path, static_cast<int>(MnaFileRole::SourceSpace));
                m_srcSpaceGroup->setEnabled(true);
                m_showSrcSpaceCheck->setEnabled(true);
                m_showSrcSpaceCheck->setChecked(false);
                m_brainView->setSourceSpaceVisible(false);
            }
        });
#else
        QString path = QFileDialog::getOpenFileName(this, "Select Source Space / Forward Solution", "",
            "Source Space Files (*-src.fif *-fwd.fif);;All FIF Files (*.fif)");
        if (path.isEmpty()) return;
        if (m_brainView->loadSourceSpace(path)) {
            trackLoadedFile(path, static_cast<int>(MnaFileRole::SourceSpace));
            m_srcSpaceGroup->setEnabled(true);
            m_showSrcSpaceCheck->setEnabled(true);
            m_showSrcSpaceCheck->setChecked(false);
            m_brainView->setSourceSpaceVisible(false);
        }
#endif
    });

    connect(m_showSrcSpaceCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setSourceSpaceVisible(checked); });

    // Connectivity Network
    connect(m_showNetworkCheck, &QCheckBox::toggled, [this](bool checked) { m_brainView->setNetworkVisible(checked); });
    connect(m_networkThresholdSlider, &QSlider::valueChanged, [this](int value) {
        m_brainView->setNetworkThreshold(value / 100.0);
    });
    connect(m_networkColormapCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        m_brainView->setNetworkColormap(text);
    });

    // ── Evoked ─────────────────────────────────────────────────────────

    // Load Evoked (average FIF) – probe for evoked sets first
    connect(m_actLoadEvoked, &QAction::triggered, [this]() {
#ifdef WASMBUILD
        QFileDialog::getOpenFileContent("Average FIF Files (*.fif)", [this](const QString &fileName, const QByteArray &fileContent) {
            if (fileName.isEmpty()) return;
            QString path = wasmSaveToTemp(fileName, fileContent);
            if (path.isEmpty()) return;

            QStringList sets = BrainView::probeEvokedSets(path);
            m_evokedSetCombo->blockSignals(true);
            m_evokedSetCombo->clear();
            if (sets.isEmpty()) {
                m_evokedSetCombo->addItem("(no evoked sets found)");
                m_evokedSetCombo->setEnabled(false);
                m_evokedSetCombo->blockSignals(false);
                return;
            }
            m_evokedSetCombo->addItems(sets);
            m_evokedSetCombo->setEnabled(sets.size() > 1);
            m_evokedSetCombo->setCurrentIndex(0);
            m_evokedSetCombo->blockSignals(false);

            m_evokedSetCombo->setProperty("evokedPath", path);
            trackLoadedFile(path, static_cast<int>(MnaFileRole::Evoked));
            m_brainView->loadSensorField(path, 0);
        });
#else
        QString path = QFileDialog::getOpenFileName(this,
            "Select Evoked / Average", "",
            "Average FIF Files (*-ave.fif);;All Files (*)");
        if (path.isEmpty()) return;

        // Probe available evoked sets and populate combo
        QStringList sets = BrainView::probeEvokedSets(path);
        m_evokedSetCombo->blockSignals(true);
        m_evokedSetCombo->clear();
        if (sets.isEmpty()) {
            m_evokedSetCombo->addItem("(no evoked sets found)");
            m_evokedSetCombo->setEnabled(false);
            m_evokedSetCombo->blockSignals(false);
            return;
        }
        m_evokedSetCombo->addItems(sets);
        m_evokedSetCombo->setEnabled(sets.size() > 1);
        m_evokedSetCombo->setCurrentIndex(0);
        m_evokedSetCombo->blockSignals(false);

        // Store the path for re-loading when the combo changes
        m_evokedSetCombo->setProperty("evokedPath", path);
        trackLoadedFile(path, static_cast<int>(MnaFileRole::Evoked));

        // Load the first evoked set
        m_brainView->loadSensorField(path, 0);
#endif
    });

    // Evoked set selection changed – reload with new index
    connect(m_evokedSetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        if (index < 0) return;
        QString path = m_evokedSetCombo->property("evokedPath").toString();
        if (path.isEmpty()) return;
        m_brainView->loadSensorField(path, index);
    });

    connect(m_brainView, &BrainView::sensorFieldLoaded, [this](int numTimePoints, int initialTimePoint) {
        // Enable field-map controls
        m_evokedGroup->setEnabled(true);
        m_sensorStreamGroup->setEnabled(true);
        m_showMegFieldCheck->setEnabled(true);
        m_showEegFieldCheck->setEnabled(true);
        m_showMegContourCheck->setEnabled(true);
        m_showEegContourCheck->setEnabled(true);
        m_megHelmetCombo->setEnabled(true);
        m_syncTimesCheck->setEnabled(true);

        // Enable time slider, preserving position when switching evoked sets
        m_sensorFieldTimeSlider->setEnabled(true);
        m_sensorFieldTimeSlider->blockSignals(true);
        m_sensorFieldTimeSlider->setRange(0, numTimePoints - 1);
        m_sensorFieldTimeSlider->setValue(qBound(0, initialTimePoint, numTimePoints - 1));
        m_sensorFieldTimeSlider->blockSignals(false);

        // Enable streaming controls
        m_sensorStreamBtn->setEnabled(true);
        m_sensorStreamModalityCombo->setEnabled(true);
        m_sensorStreamLoopCheck->setEnabled(true);
        m_sensorStreamAvgSpin->setEnabled(true);
        m_sensorStreamColormapCombo->setEnabled(true);

        // Auto-show MEG field map
        m_showMegFieldCheck->setChecked(true);
    });

    // Field map visibility
    connect(m_showMegFieldCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setSensorFieldVisible("MEG", checked);
    });
    connect(m_showEegFieldCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setSensorFieldVisible("EEG", checked);
    });

    // Contour visibility
    connect(m_showMegContourCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setSensorFieldContourVisible("MEG", checked);
    });
    connect(m_showEegContourCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setSensorFieldContourVisible("EEG", checked);
    });

    // Info panel
    connect(m_showInfoCheck, &QCheckBox::toggled, m_brainView, &BrainView::setInfoPanelVisible);

    // MEG helmet surface selection
    connect(m_megHelmetCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        m_brainView->setMegFieldMapOnHead(text == "Head");
    });

    // Sensor field time slider
    connect(m_sensorFieldTimeSlider, &QSlider::valueChanged, [this](int value) {
        m_brainView->setSensorFieldTimePoint(value);

        // Sync STC time if enabled (skip if already syncing from STC)
        if (!m_isSyncing && m_syncTimesCheck->isChecked() && m_timeSlider->isEnabled()) {
            m_isSyncing = true;
            float tmin = 0, tmax = 0;
            if (m_brainView->sensorFieldTimeRange(tmin, tmax)) {
                int maxIdx = m_sensorFieldTimeSlider->maximum();
                float timeSec = (maxIdx > 0) ?
                    tmin + (tmax - tmin) * static_cast<float>(value) / static_cast<float>(maxIdx) :
                    tmin;
                int stcIdx = m_brainView->closestStcIndex(timeSec);
                if (stcIdx >= 0) {
                    m_timeSlider->blockSignals(true);
                    m_timeSlider->setValue(stcIdx);
                    m_timeSlider->blockSignals(false);
                    m_brainView->setTimePoint(stcIdx);
                }
            }
            m_isSyncing = false;
        }
    });

    // Update sensor field time label
    connect(m_brainView, &BrainView::sensorFieldTimePointChanged, [this](int /*index*/, float time) {
        m_sensorFieldTimeLabel->setText(QString("Field Time: %1 s").arg(time, 0, 'f', 3));
    });

    // ── Sensor Streaming ────────────────────────────────────────────────
    connect(m_sensorStreamBtn, &QPushButton::clicked, [this]() {
        if (m_brainView->isRealtimeSensorStreaming()) {
            m_brainView->stopRealtimeSensorStreaming();
            m_sensorStreamBtn->setText("Start Sensor Streaming");
        } else {
            QString modality = m_sensorStreamModalityCombo->currentText();
            m_brainView->startRealtimeSensorStreaming(modality);
            if (m_brainView->isRealtimeSensorStreaming()) {
                m_sensorStreamBtn->setText("Stop Sensor Streaming");
            }
        }
    });

    connect(m_sensorStreamLoopCheck, &QCheckBox::toggled, [this](bool checked) {
        m_brainView->setRealtimeSensorLooping(checked);
    });

    connect(m_sensorStreamAvgSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        m_brainView->setRealtimeSensorAverages(value);
    });

    connect(m_sensorStreamColormapCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        m_brainView->setRealtimeSensorColormap(text);
    });

    // ── Sync UI widgets to saved BrainView state ──
    // BrainView::loadMultiViewSettings() has already restored the persisted
    // view state (surface type, visibility, shaders, overlay).  Sync the
    // MainWindow controls to match so that there is no mismatch between
    // what the user sees in the controls and what the renderer uses.
    const int target = m_brainView->visualizationEditTarget();

    // FsSurface type
    const QString savedSurf = m_brainView->activeSurfaceForTarget(target);
    if (m_surfCombo->findText(savedSurf) >= 0) {
        m_surfCombo->blockSignals(true);
        m_surfCombo->setCurrentText(savedSurf);
        m_surfCombo->blockSignals(false);
    }

    // Hemisphere visibility
    m_lhCheck->blockSignals(true);
    m_rhCheck->blockSignals(true);
    m_lhCheck->setChecked(m_brainView->objectVisibleForTarget("lh", target));
    m_rhCheck->setChecked(m_brainView->objectVisibleForTarget("rh", target));
    m_lhCheck->blockSignals(false);
    m_rhCheck->blockSignals(false);

    // BEM visibility
    const bool isInflated = (savedSurf == "inflated");
    m_headCheck->blockSignals(true);
    m_outerCheck->blockSignals(true);
    m_innerCheck->blockSignals(true);
    m_headCheck->setChecked(m_brainView->objectVisibleForTarget("bem_head", target));
    m_outerCheck->setChecked(m_brainView->objectVisibleForTarget("bem_outer_skull", target));
    m_innerCheck->setChecked(m_brainView->objectVisibleForTarget("bem_inner_skull", target));
    m_headCheck->setEnabled(!isInflated);
    m_outerCheck->setEnabled(!isInflated);
    m_innerCheck->setEnabled(!isInflated);
    m_headCheck->blockSignals(false);
    m_outerCheck->blockSignals(false);
    m_innerCheck->blockSignals(false);

    // Shader
    const QString savedShader = m_brainView->shaderModeForTarget(target);
    if (m_shaderCombo->findText(savedShader) >= 0) {
        m_shaderCombo->blockSignals(true);
        m_shaderCombo->setCurrentText(savedShader);
        m_shaderCombo->blockSignals(false);
    }

    const QString savedBemShader = m_brainView->bemShaderModeForTarget(target);
    if (m_bemShaderCombo->findText(savedBemShader) >= 0) {
        m_bemShaderCombo->blockSignals(true);
        m_bemShaderCombo->setCurrentText(savedBemShader);
        m_bemShaderCombo->blockSignals(false);
    }

    // Overlay
    const QString savedOverlay = m_brainView->overlayModeForTarget(target);
    if (m_overlayCombo->findText(savedOverlay) >= 0) {
        m_overlayCombo->blockSignals(true);
        m_overlayCombo->setCurrentText(savedOverlay);
        m_overlayCombo->blockSignals(false);
    }

    // Sensor visibility (checked state synced; enabled state stays as-is
    // until data is loaded at runtime)
    m_showMegCheck->blockSignals(true);
    m_showMegCheck->setChecked(m_brainView->objectVisibleForTarget("sens_meg", target));
    m_showMegCheck->blockSignals(false);

    m_showEegCheck->blockSignals(true);
    m_showEegCheck->setChecked(m_brainView->objectVisibleForTarget("sens_eeg", target));
    m_showEegCheck->blockSignals(false);

    m_showDigCheck->blockSignals(true);
    m_showDigCheck->setChecked(m_brainView->objectVisibleForTarget("dig", target));
    m_showDigCheck->blockSignals(false);

    m_showHelmetCheck->blockSignals(true);
    m_showHelmetCheck->setChecked(m_brainView->objectVisibleForTarget("sens_meg_helmet", target));
    m_showHelmetCheck->blockSignals(false);

    // Field map / contour visibility
    m_showMegFieldCheck->blockSignals(true);
    m_showMegFieldCheck->setChecked(m_brainView->objectVisibleForTarget("field_meg", target));
    m_showMegFieldCheck->blockSignals(false);

    m_showEegFieldCheck->blockSignals(true);
    m_showEegFieldCheck->setChecked(m_brainView->objectVisibleForTarget("field_eeg", target));
    m_showEegFieldCheck->blockSignals(false);

    m_showMegContourCheck->blockSignals(true);
    m_showMegContourCheck->setChecked(m_brainView->objectVisibleForTarget("contour_meg", target));
    m_showMegContourCheck->blockSignals(false);

    m_showEegContourCheck->blockSignals(true);
    m_showEegContourCheck->setChecked(m_brainView->objectVisibleForTarget("contour_eeg", target));
    m_showEegContourCheck->blockSignals(false);

    // MEG field map surface
    m_megHelmetCombo->blockSignals(true);
    m_megHelmetCombo->setCurrentText(m_brainView->megFieldMapOnHeadForTarget(target) ? "Head" : "Helmet");
    m_megHelmetCombo->blockSignals(false);

    // Info panel visibility
    m_showInfoCheck->blockSignals(true);
    m_showInfoCheck->setChecked(m_brainView->isInfoPanelVisible());
    m_showInfoCheck->blockSignals(false);

    // ── Menu Action Connections ────────────────────────────────────────

    connect(m_actPlayPause, &QAction::triggered, m_playButton, &QPushButton::click);
    connect(m_actStepFwd, &QAction::triggered, [this]() {
        if (m_timeSlider->isEnabled())
            m_timeSlider->setValue(m_timeSlider->value() + 1);
    });
    connect(m_actStepBack, &QAction::triggered, [this]() {
        if (m_timeSlider->isEnabled())
            m_timeSlider->setValue(qMax(0, m_timeSlider->value() - 1));
    });
    connect(m_actRealtimeToggle, &QAction::toggled, m_realtimeCheck, &QCheckBox::setChecked);
    connect(m_actSyncLock, &QAction::toggled, m_syncTimesCheck, &QCheckBox::setChecked);

    // ── Status Bar Updates ────────────────────────────────────────────

    connect(m_brainView, &BrainView::stcLoadingProgress, [this](int /*percent*/, const QString &message) {
        m_statusLabel->setText(message);
    });
    connect(m_brainView, &BrainView::sourceEstimateLoaded, [this](int /*numPoints*/) {
        m_statusLabel->setText("Ready");
    });
    connect(m_brainView, &BrainView::timePointChanged, [this](int /*index*/, float time) {
        m_statusTimeLabel->setText(QString("Time: %1 s").arg(time, 0, 'f', 3));
    });
}

//=============================================================================================================

void MainWindow::loadInitialData(const QString &subjectPath,
                                  const QString &subjectName,
                                  const QString &bemPath,
                                  const QString &transPath,
                                  const QStringList &stcPaths,
                                  const QString &digitizerPath,
                                  const QString &srcSpacePath,
                                  const QString &atlasPath,
                                  const QString &evokedPath)
{
    // Check if the subject directory actually exists before attempting to load
    QString subjectDir = subjectPath + "/" + subjectName + "/surf";
    QDir surfDir(subjectDir);
    if (!surfDir.exists()) {
        qWarning() << "No surface data found. Subject directory does not exist:" << subjectDir;
        qWarning() << "Use --subjectPath and --subject options to specify data location,";
        qWarning() << "or load data interactively through the UI.";
    } else {
        qInfo() << "Loading surfaces...";
        loadHemisphere(subjectPath, subjectName, "lh");
        loadHemisphere(subjectPath, subjectName, "rh");
        qInfo() << "Surfaces loaded.";
    }

    // Auto-load atlas annotation (explicit path overrides auto-discovery in loadHemisphere)
    if (!atlasPath.isEmpty() && QFile::exists(atlasPath)) {
        qInfo() << "Auto-loading atlas from:" << atlasPath;
        QString fileName = QFileInfo(atlasPath).fileName();
        QString lhAnnotPath, rhAnnotPath;

        if (fileName.startsWith("lh.")) {
            lhAnnotPath = atlasPath;
            rhAnnotPath = QFileInfo(atlasPath).absolutePath() + "/" + fileName.mid(3).prepend("rh.");
        } else if (fileName.startsWith("rh.")) {
            rhAnnotPath = atlasPath;
            lhAnnotPath = QFileInfo(atlasPath).absolutePath() + "/" + fileName.mid(3).prepend("lh.");
        } else {
            lhAnnotPath = atlasPath;
        }

        if (!lhAnnotPath.isEmpty() && QFile::exists(lhAnnotPath)) {
            FsAnnotation annot(lhAnnotPath);
            if (!annot.isEmpty()) {
                m_model->addAnnotation(subjectName, "lh", annot);
                qInfo() << "Added atlas annotation for lh";
            }
        }
        if (!rhAnnotPath.isEmpty() && QFile::exists(rhAnnotPath)) {
            FsAnnotation annot(rhAnnotPath);
            if (!annot.isEmpty()) {
                m_model->addAnnotation(subjectName, "rh", annot);
                qInfo() << "Added atlas annotation for rh";
            }
        }
    }

    if (!bemPath.isEmpty()) {
        loadBem(subjectName, bemPath);
    }

    // Auto-load digitizer
    if (!digitizerPath.isEmpty() && QFile::exists(digitizerPath)) {
        qInfo() << "Auto-loading digitizer from:" << digitizerPath;
        trackLoadedFile(digitizerPath, static_cast<int>(MnaFileRole::Digitizer));
        if (m_brainView->loadSensors(digitizerPath)) {
            m_sensorGroup->setEnabled(true);
            m_showMegCheck->setEnabled(true);
            m_showEegCheck->setEnabled(true);
            m_showDigCheck->setEnabled(true);
            m_showHelmetCheck->setEnabled(true);
            m_helmetShapeCombo->setEnabled(true);

            // Sync checkboxes to the persisted visibility profile so the
            // UI reflects what was saved last session.
            syncUIToEditTarget(m_brainView->visualizationEditTarget());

            // Now push the (possibly restored) checkbox states into BrainView.
            // Helmet is set AFTER MEG so the MEG→Helmet cascade doesn't
            // override the independently persisted helmet state.
            m_brainView->setSensorVisible("MEG", m_showMegCheck->isChecked());
            m_brainView->setSensorVisible("EEG", m_showEegCheck->isChecked());
            m_brainView->setSensorVisible("Digitizer", m_showDigCheck->isChecked());
            m_brainView->setSensorVisible("MEG Helmet", m_showHelmetCheck->isChecked());

            // Enable sub-category checkboxes if master is on
            bool digOn = m_showDigCheck->isChecked();
            m_showDigCardinalCheck->setEnabled(digOn);
            m_showDigHpiCheck->setEnabled(digOn);
            m_showDigEegCheck->setEnabled(digOn);
            m_showDigExtraCheck->setEnabled(digOn);
        }
    }

    // Auto-load transformation
    if (!transPath.isEmpty() && QFile::exists(transPath)) {
        qInfo() << "Auto-loading transformation from:" << transPath;
        trackLoadedFile(transPath, static_cast<int>(MnaFileRole::Transform));
        m_brainView->loadTransformation(transPath);
    }

    // Auto-load source space
    if (!srcSpacePath.isEmpty() && QFile::exists(srcSpacePath)) {
        qInfo() << "Auto-loading source space from:" << srcSpacePath;
        trackLoadedFile(srcSpacePath, static_cast<int>(MnaFileRole::SourceSpace));
        if (m_brainView->loadSourceSpace(srcSpacePath)) {
            m_srcSpaceGroup->setEnabled(true);
            m_showSrcSpaceCheck->setEnabled(true);
            m_showSrcSpaceCheck->setChecked(false);
            m_brainView->setSourceSpaceVisible(false);
        }
    }

    // Auto-load STC(s) – add all provided paths to the combo; activate only the first
    for (int i = 0; i < stcPaths.size(); ++i) {
        const QString &stcPath = stcPaths[i];
        if (!stcPath.isEmpty() && QFile::exists(stcPath)) {
            qInfo() << "Auto-loading source estimate from:" << stcPath;
            trackLoadedFile(stcPath, static_cast<int>(MnaFileRole::SourceEstimate));
            addStcEntry(stcPath, /*activate=*/ (i == 0));
        }
    }

    // Auto-load evoked
    if (!evokedPath.isEmpty() && QFile::exists(evokedPath)) {
        qInfo() << "Auto-loading evoked from:" << evokedPath;
        trackLoadedFile(evokedPath, static_cast<int>(MnaFileRole::Evoked));

        QStringList sets = BrainView::probeEvokedSets(evokedPath);
        m_evokedSetCombo->blockSignals(true);
        m_evokedSetCombo->clear();
        if (!sets.isEmpty()) {
            m_evokedSetCombo->addItems(sets);
            m_evokedSetCombo->setEnabled(sets.size() > 1);
            m_evokedSetCombo->setCurrentIndex(0);
        }
        m_evokedSetCombo->blockSignals(false);
        m_evokedSetCombo->setProperty("evokedPath", evokedPath);

        m_brainView->loadSensorField(evokedPath, 0);
    }
}

//=============================================================================================================

void MainWindow::addStcEntry(const QString &stcPath, bool activate)
{
    // Resolve the LH/RH pair
    QString lhPath, rhPath;
    if (stcPath.contains("-lh.stc")) {
        lhPath = stcPath;
        QString sibling = stcPath; sibling.replace("-lh.stc", "-rh.stc");
        if (QFile::exists(sibling)) rhPath = sibling;
    } else if (stcPath.contains("-rh.stc")) {
        rhPath = stcPath;
        QString sibling = stcPath; sibling.replace("-rh.stc", "-lh.stc");
        if (QFile::exists(sibling)) lhPath = sibling;
    } else {
        lhPath = stcPath;
    }

    // Derive a display name from the filename:
    // e.g. "sample_audvis-ave-spm-left_auditory-lh.stc" → "sample_audvis-ave-spm-left_auditory"
    QString displayName;
    QString refPath = !lhPath.isEmpty() ? lhPath : rhPath;
    QFileInfo fi(refPath);
    displayName = fi.fileName();
    displayName.replace("-lh.stc", "").replace("-rh.stc", "");

    // Avoid duplicates
    for (int i = 0; i < m_stcCombo->count(); ++i) {
        if (m_stcCombo->itemText(i) == displayName) {
            if (activate) {
                m_stcCombo->setCurrentIndex(i);
            }
            return;
        }
    }

    // Store the LH/RH paths as item data
    QStringList pair;
    pair << lhPath << rhPath;

    m_stcCombo->blockSignals(true);
    m_stcCombo->addItem(displayName, pair);
    m_stcCombo->setEnabled(m_stcCombo->count() > 1);
    m_stcCombo->blockSignals(false);

    if (activate) {
        m_stcCombo->setCurrentIndex(m_stcCombo->count() - 1);
        // If this is the first entry, combo didn't fire currentIndexChanged (was -1 → 0),
        // so we need to trigger the load manually via the signal
        if (m_stcCombo->count() == 1) {
            emit m_stcCombo->currentIndexChanged(0);
        }
    }
}

//=============================================================================================================

void MainWindow::loadHemisphere(const QString &subjectPath, const QString &subjectName, const QString &hemi)
{
    QStringList types = {"pial", "inflated", "white"};
    for (const auto &type : types) {
        QString surfPath = subjectPath + "/" + subjectName + "/surf/" + hemi + "." + type;
        if (!QFile::exists(surfPath)) {
            continue;
        }
        FsSurface surf(surfPath);
        if (!surf.isEmpty()) {
            m_model->addSurface(subjectName, hemi, type, surf);
            m_surfGroup->setEnabled(true);
            trackLoadedFile(surfPath, static_cast<int>(MnaFileRole::Surface));
            qInfo() << "Added" << hemi << type;
        }
    }

    // Load Atlas (FsAnnotation)
    QString annotPath = subjectPath + "/" + subjectName + "/label/" + hemi + ".aparc.annot";
    if (QFile::exists(annotPath)) {
        FsAnnotation annot(annotPath);
        if (!annot.isEmpty()) {
            m_model->addAnnotation(subjectName, hemi, annot);
            trackLoadedFile(annotPath, static_cast<int>(MnaFileRole::Annotation));
            qInfo() << "Added annotation for" << hemi;
        }
    }
}

//=============================================================================================================

void MainWindow::loadBem(const QString &subjectName, const QString &bemPath)
{
    QFile bemFile(bemPath);
    if (bemFile.exists()) {
        trackLoadedFile(bemPath, static_cast<int>(MnaFileRole::Bem));
        MNEBem bem(bemFile);
        for (int i = 0; i < bem.size(); ++i) {
            QString name;
            switch (bem[i].id) {
                case 4: name = "head"; break;
                case 3: name = "outer_skull"; break;
                case 1: name = "inner_skull"; break;
                default: name = QString("%1").arg(i); break;
            }
            m_model->addBemSurface(subjectName, name, bem[i]);
            qInfo() << "Added BEM:" << name;
        }
        m_bemGroup->setEnabled(true);
    } else {
        qWarning() << "BEM path provided but file not found:" << bemPath;
    }
}

//=============================================================================================================

void MainWindow::enableNetworkControls()
{
    m_networkGroup->setEnabled(true);
    m_showNetworkCheck->setEnabled(true);
    m_showNetworkCheck->setChecked(true);
    m_networkThresholdSlider->setEnabled(true);
    m_networkColormapCombo->setEnabled(true);
}
//=============================================================================================================

void MainWindow::syncUIToEditTarget(int target)
{
    // Sync the surface, shader, overlay, and BEM shader combos to reflect
    // the settings of the currently selected edit-target viewport.
    // Block signals to avoid recursive updates.

    m_surfCombo->blockSignals(true);
    m_surfCombo->setCurrentText(m_brainView->activeSurfaceForTarget(target));
    m_surfCombo->blockSignals(false);

    m_shaderCombo->blockSignals(true);
    m_shaderCombo->setCurrentText(m_brainView->shaderModeForTarget(target));
    m_shaderCombo->blockSignals(false);

    m_bemShaderCombo->blockSignals(true);
    m_bemShaderCombo->setCurrentText(m_brainView->bemShaderModeForTarget(target));
    m_bemShaderCombo->blockSignals(false);

    m_overlayCombo->blockSignals(true);
    m_overlayCombo->setCurrentText(m_brainView->overlayModeForTarget(target));
    m_overlayCombo->blockSignals(false);

    // Sync camera preset combo
    if (target >= 0) {
        m_cameraPresetCombo->blockSignals(true);
        m_cameraPresetCombo->setCurrentIndex(m_brainView->viewportCameraPreset(target));
        m_cameraPresetCombo->blockSignals(false);
    }

    // Sync hemisphere/BEM/dipole/network visibility from the target viewport
    m_lhCheck->blockSignals(true);
    m_lhCheck->setChecked(m_brainView->objectVisibleForTarget("lh", target));
    m_lhCheck->blockSignals(false);

    m_rhCheck->blockSignals(true);
    m_rhCheck->setChecked(m_brainView->objectVisibleForTarget("rh", target));
    m_rhCheck->blockSignals(false);

    m_headCheck->blockSignals(true);
    m_headCheck->setChecked(m_brainView->objectVisibleForTarget("bem_head", target));
    m_headCheck->blockSignals(false);

    m_outerCheck->blockSignals(true);
    m_outerCheck->setChecked(m_brainView->objectVisibleForTarget("bem_outer_skull", target));
    m_outerCheck->blockSignals(false);

    m_innerCheck->blockSignals(true);
    m_innerCheck->setChecked(m_brainView->objectVisibleForTarget("bem_inner_skull", target));
    m_innerCheck->blockSignals(false);

    // Sync sensor visibility
    m_showMegCheck->blockSignals(true);
    m_showMegCheck->setChecked(m_brainView->objectVisibleForTarget("sens_meg", target));
    m_showMegCheck->blockSignals(false);

    m_showEegCheck->blockSignals(true);
    m_showEegCheck->setChecked(m_brainView->objectVisibleForTarget("sens_eeg", target));
    m_showEegCheck->blockSignals(false);

    m_showHelmetCheck->blockSignals(true);
    m_showHelmetCheck->setChecked(m_brainView->objectVisibleForTarget("sens_meg_helmet", target));
    m_showHelmetCheck->blockSignals(false);

    // Sync digitizer visibility
    m_showDigCheck->blockSignals(true);
    m_showDigCheck->setChecked(m_brainView->objectVisibleForTarget("dig", target));
    m_showDigCheck->blockSignals(false);

    m_showDigCardinalCheck->blockSignals(true);
    m_showDigCardinalCheck->setChecked(m_brainView->objectVisibleForTarget("dig_cardinal", target));
    m_showDigCardinalCheck->blockSignals(false);

    m_showDigHpiCheck->blockSignals(true);
    m_showDigHpiCheck->setChecked(m_brainView->objectVisibleForTarget("dig_hpi", target));
    m_showDigHpiCheck->blockSignals(false);

    m_showDigEegCheck->blockSignals(true);
    m_showDigEegCheck->setChecked(m_brainView->objectVisibleForTarget("dig_eeg", target));
    m_showDigEegCheck->blockSignals(false);

    m_showDigExtraCheck->blockSignals(true);
    m_showDigExtraCheck->setChecked(m_brainView->objectVisibleForTarget("dig_extra", target));
    m_showDigExtraCheck->blockSignals(false);

    // Sync field map / contour visibility
    m_showMegFieldCheck->blockSignals(true);
    m_showMegFieldCheck->setChecked(m_brainView->objectVisibleForTarget("field_meg", target));
    m_showMegFieldCheck->blockSignals(false);

    m_showEegFieldCheck->blockSignals(true);
    m_showEegFieldCheck->setChecked(m_brainView->objectVisibleForTarget("field_eeg", target));
    m_showEegFieldCheck->blockSignals(false);

    m_showMegContourCheck->blockSignals(true);
    m_showMegContourCheck->setChecked(m_brainView->objectVisibleForTarget("contour_meg", target));
    m_showMegContourCheck->blockSignals(false);

    m_showEegContourCheck->blockSignals(true);
    m_showEegContourCheck->setChecked(m_brainView->objectVisibleForTarget("contour_eeg", target));
    m_showEegContourCheck->blockSignals(false);

    // Sync MEG field map surface (helmet vs head)
    m_megHelmetCombo->blockSignals(true);
    m_megHelmetCombo->setCurrentIndex(m_brainView->megFieldMapOnHeadForTarget(target) ? 1 : 0);
    m_megHelmetCombo->blockSignals(false);

    // Sync dipole, source space, network visibility
    m_showDipoleCheck->blockSignals(true);
    m_showDipoleCheck->setChecked(m_brainView->objectVisibleForTarget("dipoles", target));
    m_showDipoleCheck->blockSignals(false);

    m_showSrcSpaceCheck->blockSignals(true);
    m_showSrcSpaceCheck->setChecked(m_brainView->objectVisibleForTarget("source_space", target));
    m_showSrcSpaceCheck->blockSignals(false);

    m_showNetworkCheck->blockSignals(true);
    m_showNetworkCheck->setChecked(m_brainView->objectVisibleForTarget("network", target));
    m_showNetworkCheck->blockSignals(false);
}

//=============================================================================================================

void MainWindow::updateViewportCheckboxes(int count)
{
    Q_UNUSED(count);
}

//=============================================================================================================

void MainWindow::trackLoadedFile(const QString &path, int role)
{
    if (path.isEmpty()) return;
    // Avoid duplicates
    for (const auto &entry : m_loadedFiles) {
        if (entry.first == path) return;
    }
    m_loadedFiles.append(qMakePair(path, role));
    addLoadedFileEntry(path, role);
}

//=============================================================================================================

void MainWindow::unloadFileFromScene(MnaFileRole role, const QString &path)
{
    if (!m_brainView) return;

    switch (role) {
    case MnaFileRole::Surface:
    case MnaFileRole::Annotation:
        m_brainView->clearSurfaces();
        m_surfGroup->setEnabled(false);
        break;
    case MnaFileRole::Bem:
        m_brainView->clearBem();
        m_bemGroup->setEnabled(false);
        break;
    case MnaFileRole::SourceEstimate:
        m_brainView->clearSourceEstimate();
        m_stcGroup->setEnabled(false);
        break;
    case MnaFileRole::SourceSpace:
        m_brainView->clearSourceSpace();
        m_srcSpaceGroup->setEnabled(false);
        break;
    case MnaFileRole::Digitizer:
        m_brainView->clearSensors();
        m_sensorGroup->setEnabled(false);
        break;
    case MnaFileRole::Transform:
        m_brainView->clearTransformation();
        break;
    case MnaFileRole::Evoked:
        m_brainView->clearEvoked();
        m_evokedGroup->setEnabled(false);
        break;
    case MnaFileRole::Custom:
        // Dipole files use Custom role
        if (path.endsWith(".dip") || path.endsWith(".bdip")) {
            m_brainView->clearDipoles();
            m_dipoleGroup->setEnabled(false);
        }
        break;
    default:
        break;
    }
}

//=============================================================================================================

void MainWindow::importMnaProject(const QString &path)
{
    MnaProject proj = MnaIO::read(path);
    if (proj.name.isEmpty() && proj.subjects.isEmpty()) {
        qWarning() << "Failed to read MNA project from:" << path;
        return;
    }

    qInfo() << "Importing MNA project:" << proj.name;
    const QString projectDir = QFileInfo(path).absolutePath();

    // Create a unique extraction root under /tmp using the project name
    const QString extractRoot = QStringLiteral("/tmp/mna_") +
        proj.name.toLower().replace(QRegularExpression(QStringLiteral("[^a-z0-9]+")), QStringLiteral("_"));

    // Collect all file refs from all subjects/sessions/recordings
    for (const MnaSubject &subj : proj.subjects) {
        for (const MnaSession &sess : subj.sessions) {
            for (const MnaRecording &rec : sess.recordings) {
                for (const MnaFileRef &ref : rec.files) {
                    // Resolve the file: embedded → extract to BIDS tree, external → resolve relative path
                    QString filePath;
                    if (ref.embedded && !ref.data.isEmpty()) {
                        // Preserve BIDS directory structure: <extractRoot>/<ref.path>
                        filePath = extractRoot + QStringLiteral("/") + ref.path;
                        QDir().mkpath(QFileInfo(filePath).absolutePath());
                        QFile f(filePath);
                        if (f.open(QIODevice::WriteOnly)) {
                            f.write(ref.data);
                            f.close();
                        } else {
                            qWarning() << "Failed to extract embedded file:" << ref.path;
                            continue;
                        }
                    } else {
                        // Resolve relative path against the project file directory
                        filePath = QDir(projectDir).filePath(ref.path);
                        if (!QFile::exists(filePath)) {
                            qWarning() << "Referenced file not found:" << filePath;
                            continue;
                        }
                    }

                    trackLoadedFile(filePath, static_cast<int>(ref.role));

                    // Dispatch to the appropriate loader based on role
                    switch (ref.role) {
                    case MnaFileRole::Surface: {
                        QString fn = QFileInfo(filePath).fileName();
                        QString hemi = fn.contains("lh.") ? "lh" : (fn.contains("rh.") ? "rh" : "lh");
                        QString type = "pial";
                        if (fn.contains("inflated")) type = "inflated";
                        else if (fn.contains("white")) type = "white";
                        else if (fn.contains("orig")) type = "orig";
                        FsSurface surf(filePath);
                        if (!surf.isEmpty()) {
                            m_model->addSurface(subj.id, hemi, type, surf);
                            m_surfGroup->setEnabled(true);
                            qInfo() << "Imported surface:" << fn;
                        }
                        break;
                    }
                    case MnaFileRole::Annotation: {
                        QString fn = QFileInfo(filePath).fileName();
                        QString hemi = fn.contains("lh.") ? "lh" : (fn.contains("rh.") ? "rh" : "lh");
                        FsAnnotation annot(filePath);
                        if (!annot.isEmpty()) {
                            m_model->addAnnotation(subj.id, hemi, annot);
                            qInfo() << "Imported annotation:" << fn;
                        }
                        break;
                    }
                    case MnaFileRole::Bem:
                        loadBem(subj.id, filePath);
                        qInfo() << "Imported BEM:" << ref.path;
                        break;
                    case MnaFileRole::Digitizer:
                        if (m_brainView->loadSensors(filePath)) {
                            m_sensorGroup->setEnabled(true);
                            m_showMegCheck->setEnabled(true);
                            m_showEegCheck->setEnabled(true);
                            m_showDigCheck->setEnabled(true);
                            m_showHelmetCheck->setEnabled(true);
                            m_helmetShapeCombo->setEnabled(true);
                            qInfo() << "Imported digitizer:" << ref.path;
                        }
                        break;
                    case MnaFileRole::Transform:
                        m_brainView->loadTransformation(filePath);
                        qInfo() << "Imported transformation:" << ref.path;
                        break;
                    case MnaFileRole::SourceSpace:
                        if (m_brainView->loadSourceSpace(filePath)) {
                            m_srcSpaceGroup->setEnabled(true);
                            m_showSrcSpaceCheck->setEnabled(true);
                            m_showSrcSpaceCheck->setChecked(false);
                            m_brainView->setSourceSpaceVisible(false);
                            qInfo() << "Imported source space:" << ref.path;
                        }
                        break;
                    case MnaFileRole::SourceEstimate:
                        addStcEntry(filePath, m_stcCombo->count() == 0);
                        qInfo() << "Imported STC:" << ref.path;
                        break;
                    case MnaFileRole::Evoked: {
                        QStringList sets = BrainView::probeEvokedSets(filePath);
                        m_evokedSetCombo->blockSignals(true);
                        m_evokedSetCombo->clear();
                        if (!sets.isEmpty()) {
                            m_evokedSetCombo->addItems(sets);
                            m_evokedSetCombo->setEnabled(sets.size() > 1);
                            m_evokedSetCombo->setCurrentIndex(0);
                        }
                        m_evokedSetCombo->blockSignals(false);
                        m_evokedSetCombo->setProperty("evokedPath", filePath);
                        m_brainView->loadSensorField(filePath, 0);
                        qInfo() << "Imported evoked:" << ref.path;
                        break;
                    }
                    default:
                        qInfo() << "Preserving foreign file with role:" << mnaFileRoleToString(ref.role) << ref.path;
                        break;
                    }
                }
            }
        }
    }

    // Resolve all non-embedded file ref paths to absolute before storing,
    // so we can re-relativize correctly when saving to a different location.
    for (auto &s : proj.subjects)
        for (auto &se : s.sessions)
            for (auto &r : se.recordings)
                for (auto &f : r.files)
                    if (!f.embedded)
                        f.path = QDir(projectDir).absoluteFilePath(f.path);

    m_loadedMnaProject = proj;

    qInfo() << "MNA project import complete.";
    addRecentFile(path);
}

//=============================================================================================================

void MainWindow::exportMnaProject(const QString &path, bool embedData)
{
    // ── Enriching mode: start from loaded project to preserve foreign data ──
    // Roles that mne_inspect owns and will replace with current state
    static const QSet<MnaFileRole> ownedRoles = {
        MnaFileRole::Surface,
        MnaFileRole::Annotation,
        MnaFileRole::Bem,
        MnaFileRole::Digitizer,
        MnaFileRole::Transform,
        MnaFileRole::SourceSpace,
        MnaFileRole::SourceEstimate,
        MnaFileRole::Evoked
    };

    MnaProject proj;
    const bool hasBase = !m_loadedMnaProject.subjects.isEmpty();
    const QString projectDir = QFileInfo(path).absolutePath();

    if (hasBase) {
        // Preserve all metadata, pipeline, and subject structure from the loaded project
        proj = m_loadedMnaProject;
        proj.modified = QDateTime::currentDateTimeUtc();

        // Strip only our owned roles from the first recording; keep everything else
        if (!proj.subjects.isEmpty()
            && !proj.subjects[0].sessions.isEmpty()
            && !proj.subjects[0].sessions[0].recordings.isEmpty())
        {
            auto &files = proj.subjects[0].sessions[0].recordings[0].files;
            files.erase(std::remove_if(files.begin(), files.end(),
                [](const MnaFileRef &r){ return ownedRoles.contains(r.role); }),
                files.end());

            // Re-serialize foreign refs for the target format
            for (auto &ref : files) {
                if (embedData) {
                    if (!ref.embedded && !ref.path.isEmpty()) {
                        QFile f(ref.path);
                        if (f.open(QIODevice::ReadOnly)) {
                            ref.data = f.readAll();
                            ref.sizeBytes = ref.data.size();
                            f.close();
                        }
                        ref.path = proj.subjects[0].id + QStringLiteral("/")
                                   + proj.subjects[0].sessions[0].id + QStringLiteral("/passthrough/")
                                   + QFileInfo(ref.path).fileName();
                        ref.embedded = true;
                    }
                } else {
                    if (ref.embedded && !ref.data.isEmpty()) {
                        const QString outPath = projectDir + QDir::separator() + QFileInfo(ref.path).fileName();
                        QFile f(outPath);
                        if (f.open(QIODevice::WriteOnly)) {
                            f.write(ref.data);
                            f.close();
                        }
                        ref.path = QDir(projectDir).relativeFilePath(outPath);
                        ref.embedded = false;
                        ref.data.clear();
                    } else if (!ref.embedded) {
                        ref.path = QDir(projectDir).relativeFilePath(ref.path);
                    }
                }
            }
        }
    } else {
        // Brand-new project — create minimal structure
        proj.name = QStringLiteral("MNE Inspect Export");
        proj.description = QStringLiteral("Exported from MNE Inspect");
        proj.mnaVersion = QString::fromLatin1(MnaProject::CURRENT_SCHEMA_VERSION);
        proj.created = QDateTime::currentDateTimeUtc();
        proj.modified = QDateTime::currentDateTimeUtc();

        MnaSubject subj;
        subj.id = QStringLiteral("User");
        MnaSession session;
        session.id = QStringLiteral("session-01");
        MnaRecording rec;
        rec.id = QStringLiteral("recording-01");
        session.recordings.append(rec);
        subj.sessions.append(session);
        proj.subjects.append(subj);
    }

    // Reference to the first recording where we add our files
    auto &rec = proj.subjects[0].sessions[0].recordings[0];

    // Add current owned file refs
    for (const auto &entry : m_loadedFiles) {
        MnaFileRef ref;
        ref.role = static_cast<MnaFileRole>(entry.second);
        ref.format = QFileInfo(entry.first).suffix();

        if (embedData) {
            ref.path = bidsBuildRelPath(proj.subjects[0].id,
                                        proj.subjects[0].sessions[0].id,
                                        ref.role, entry.first);
            ref.embedded = true;
            QFile f(entry.first);
            if (f.open(QIODevice::ReadOnly)) {
                ref.data = f.readAll();
                ref.sizeBytes = ref.data.size();
                f.close();
            }
        } else {
            ref.path = QDir(projectDir).relativeFilePath(entry.first);
            ref.embedded = false;
            QFileInfo fi(entry.first);
            ref.sizeBytes = fi.size();
        }

        rec.files.append(ref);
    }

    if (MnaIO::write(proj, path)) {
        m_loadedMnaProject = proj;
        qInfo() << "Exported MNA project to:" << path;
    } else {
        qWarning() << "Failed to export MNA project to:" << path;
    }
}

//=============================================================================================================

void MainWindow::createMenus()
{
    // ── File Menu ──────────────────────────────────────────────────────

    m_fileMenu = menuBar()->addMenu("&File");

    m_actOpenProject = m_fileMenu->addAction("&Open Project...");
    m_actOpenProject->setShortcut(QKeySequence("Ctrl+O"));

    m_actExportProject = m_fileMenu->addAction("&Export Project...");
    m_actExportProject->setShortcut(QKeySequence("Ctrl+Shift+E"));

    m_fileMenu->addSeparator();

    m_actLoadSurface = m_fileMenu->addAction("Load &Surface...");
    m_actLoadSurface->setShortcut(QKeySequence("Ctrl+L"));

    m_actLoadAtlas = m_fileMenu->addAction("Load &Atlas...");
    m_actLoadBem = m_fileMenu->addAction("Load &BEM...");
    m_actLoadStc = m_fileMenu->addAction("Load Source &Estimate...");
    m_actLoadDipoles = m_fileMenu->addAction("Load &Dipoles...");
    m_actLoadSrcSpace = m_fileMenu->addAction("Load Source S&pace...");
    m_actLoadEvoked = m_fileMenu->addAction("Load E&voked...");
    m_actLoadDigitizer = m_fileMenu->addAction("Load Digi&tizer...");
    m_actLoadTransform = m_fileMenu->addAction("Load T&ransformation...");

    m_fileMenu->addSeparator();
    m_actLoadElectrodes = m_fileMenu->addAction("Load &Electrodes...");
    connect(m_actLoadElectrodes, &QAction::triggered, this, &MainWindow::onLoadElectrodes);
    m_actLoadMri = m_fileMenu->addAction("Load &MRI...");
    connect(m_actLoadMri, &QAction::triggered, this, &MainWindow::onLoadMri);

    m_fileMenu->addSeparator();

#ifndef WASMBUILD
    m_recentFilesMenu = m_fileMenu->addMenu("Recent Projects");
    updateRecentFilesMenu();
    m_fileMenu->addSeparator();
#endif

    m_actQuit = m_fileMenu->addAction("&Quit");
    m_actQuit->setShortcut(QKeySequence("Ctrl+Q"));
    connect(m_actQuit, &QAction::triggered, this, &QWidget::close);

    // ── View Menu ──────────────────────────────────────────────────────

    m_viewMenu = menuBar()->addMenu("&View");

    m_actShowControls = m_controlsDock->toggleViewAction();
    m_actShowControls->setShortcut(QKeySequence("Ctrl+1"));
    m_actShowControls->setText("Show Controls Panel");
    m_viewMenu->addAction(m_actShowControls);

    m_actShowLoadedFiles = m_loadedFilesDock->toggleViewAction();
    m_actShowLoadedFiles->setShortcut(QKeySequence("Ctrl+2"));
    m_actShowLoadedFiles->setText("Show Loaded Files Panel");
    m_viewMenu->addAction(m_actShowLoadedFiles);

    if (m_pickDock) {
        m_actShowPick = m_pickDock->toggleViewAction();
        m_actShowPick->setText("Show Pick Panel");
        m_actShowPick->setShortcut(QKeySequence("Ctrl+3"));
        m_viewMenu->addAction(m_actShowPick);
    }
    if (m_layersDock) {
        m_actShowLayers = m_layersDock->toggleViewAction();
        m_actShowLayers->setText("Show Layers Panel");
        m_actShowLayers->setShortcut(QKeySequence("Ctrl+4"));
        m_viewMenu->addAction(m_actShowLayers);
    }
    if (m_overlayDock) {
        m_actShowOverlay = m_overlayDock->toggleViewAction();
        m_actShowOverlay->setText("Show Overlay Panel");
        m_actShowOverlay->setShortcut(QKeySequence("Ctrl+5"));
        m_viewMenu->addAction(m_actShowOverlay);
    }

    m_viewMenu->addSeparator();

    m_cameraPresetsMenu = m_viewMenu->addMenu("Camera Presets");
    const QStringList presets = {"Left", "Right", "Top", "Bottom", "Front", "Back"};
    for (const QString &preset : presets) {
        QAction *act = m_cameraPresetsMenu->addAction(preset);
        connect(act, &QAction::triggered, [this, preset]() {
            int idx = m_cameraPresetCombo->findText(preset);
            if (idx >= 0) m_cameraPresetCombo->setCurrentIndex(idx);
        });
    }

    m_actResetCamera = m_viewMenu->addAction("&Reset Camera");
    m_actResetCamera->setShortcut(QKeySequence("Ctrl+R"));
    connect(m_actResetCamera, &QAction::triggered, [this]() {
        m_cameraPresetCombo->setCurrentIndex(1);  // Perspective
    });

    // ── Tools Menu ─────────────────────────────────────────────────────

    m_toolsMenu = menuBar()->addMenu("&Tools");

    m_playbackMenu = m_toolsMenu->addMenu("Playback Controls");

    m_actPlayPause = m_playbackMenu->addAction("Play / Pause");
    m_actPlayPause->setShortcut(QKeySequence(Qt::Key_Space));

    m_actStepFwd = m_playbackMenu->addAction("Step Forward");
    m_actStepBack = m_playbackMenu->addAction("Step Back");

    m_playbackMenu->addSeparator();
    m_actSyncLock = m_playbackMenu->addAction("Sync Lock");
    m_actSyncLock->setCheckable(true);
    m_actSyncLock->setChecked(true);

    m_actRealtimeToggle = m_toolsMenu->addAction("Realtime Streaming");
    m_actRealtimeToggle->setCheckable(true);

    // ── Help Menu ──────────────────────────────────────────────────────

    m_helpMenu = menuBar()->addMenu("&Help");

    m_helpMenu->addAction("About MNE Inspect", [this]() {
        QMessageBox::about(this, "About MNE Inspect",
            "MNE Inspect\n\n"
            "Brain visualization and analysis tool.\n\n"
            "Part of the MNE-CPP project.\n"
            "https://mne-cpp.github.io");
    });

    m_helpMenu->addAction("About Qt", [this]() {
        QApplication::aboutQt();
    });
}

//=============================================================================================================

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel("Ready");
    m_statusTimeLabel = new QLabel("");

    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_statusTimeLabel);
}

//=============================================================================================================

void MainWindow::createLoadedFilesDock()
{
    m_loadedFilesDock = new QDockWidget("Loaded Files", this);
    m_loadedFilesDock->setObjectName("loadedFilesDock");
    m_loadedFilesDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    m_loadedFilesDock->setTitleBarWidget(createFlatDockTitleBar(m_loadedFilesDock, "Loaded Files"));

    m_loadedFilesTree = new QTreeWidget;
    m_loadedFilesTree->setHeaderLabels({"Name", "Type", "Path"});
    m_loadedFilesTree->setColumnCount(3);
    m_loadedFilesTree->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_loadedFilesTree, &QTreeWidget::customContextMenuRequested, [this](const QPoint &pos) {
        QTreeWidgetItem *item = m_loadedFilesTree->itemAt(pos);
        if (!item) return;

        QMenu menu;
        QAction *removeAct = menu.addAction("Remove");
#ifndef WASMBUILD
        QAction *showInFinderAct = menu.addAction("Show in Finder");
#endif
        QAction *copyPathAct = menu.addAction("Copy Path");

        QAction *chosen = menu.exec(m_loadedFilesTree->viewport()->mapToGlobal(pos));

        if (chosen == removeAct) {
            QString path = item->text(2);
            int role = item->data(0, Qt::UserRole).toInt();
            // Unload from 3D scene based on role
            unloadFileFromScene(static_cast<MnaFileRole>(role), path);
            // Remove from tracking list
            for (int i = m_loadedFiles.size() - 1; i >= 0; --i) {
                if (m_loadedFiles[i].first == path) {
                    m_loadedFiles.removeAt(i);
                    break;
                }
            }
            delete item;
            if (m_statusLabel)
                m_statusLabel->setText("Ready");
#ifndef WASMBUILD
        } else if (chosen == showInFinderAct) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(item->text(2)).absolutePath()));
#endif
        } else if (chosen == copyPathAct) {
            QApplication::clipboard()->setText(item->text(2));
        }
    });

    // Double-click on STC entry activates it in the STC combo box
    connect(m_loadedFilesTree, &QTreeWidget::itemDoubleClicked, [this](QTreeWidgetItem *item, int /*column*/) {
        if (!item) return;
        int role = item->data(0, Qt::UserRole).toInt();
        if (static_cast<MnaFileRole>(role) == MnaFileRole::SourceEstimate) {
            QString fileName = item->text(0);
            // Match against STC combo entries
            QString baseName = fileName;
            baseName.replace("-lh.stc", "").replace("-rh.stc", "");
            for (int i = 0; i < m_stcCombo->count(); ++i) {
                if (m_stcCombo->itemText(i) == baseName) {
                    m_stcCombo->setCurrentIndex(i);
                    return;
                }
            }
        }
    });

    m_loadedFilesDock->setWidget(m_loadedFilesTree);
    addDockWidget(Qt::RightDockWidgetArea, m_loadedFilesDock);
}

//=============================================================================================================

void MainWindow::addLoadedFileEntry(const QString &path, int role)
{
    if (!m_loadedFilesTree) return;

    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText(0, QFileInfo(path).fileName());
    item->setText(1, mnaFileRoleToString(static_cast<MnaFileRole>(role)));
    item->setText(2, path);
    item->setData(0, Qt::UserRole, role);
    m_loadedFilesTree->addTopLevelItem(item);

    // Update status bar
    if (m_statusLabel) {
        m_statusLabel->setText(QString("Loaded: %1").arg(QFileInfo(path).fileName()));
    }
}

//=============================================================================================================

void MainWindow::saveSettings()
{
#ifndef WASMBUILD
    QSettings settings("mne-cpp", "mne_inspect");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("recentFiles", m_recentFiles);
#endif
}

//=============================================================================================================

void MainWindow::restoreSettings()
{
#ifndef WASMBUILD
    QSettings settings("mne-cpp", "mne_inspect");
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
        restoreState(settings.value("windowState").toByteArray());
    } else {
        resize(1200, 800);
    }
    m_recentFiles = settings.value("recentFiles").toStringList();
    updateRecentFilesMenu();
#else
    resize(1200, 800);
#endif
}

//=============================================================================================================

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

//=============================================================================================================

void MainWindow::updateRecentFilesMenu()
{
#ifndef WASMBUILD
    if (!m_recentFilesMenu) return;
    m_recentFilesMenu->clear();

    const int maxRecent = qMin(m_recentFiles.size(), 10);
    for (int i = 0; i < maxRecent; ++i) {
        const QString &filePath = m_recentFiles[i];
        QAction *act = m_recentFilesMenu->addAction(QFileInfo(filePath).fileName());
        connect(act, &QAction::triggered, [this, filePath]() {
            importMnaProject(filePath);
        });
    }
    m_recentFilesMenu->setEnabled(!m_recentFiles.isEmpty());
#endif
}

//=============================================================================================================

void MainWindow::addRecentFile(const QString &path)
{
#ifndef WASMBUILD
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > 10) {
        m_recentFiles.removeLast();
    }
    updateRecentFilesMenu();
#else
    Q_UNUSED(path);
#endif
}

//=============================================================================================================
// Multimodal scene wiring (pick dock, layers dock, overlay dock, plugin loaders)
//=============================================================================================================

void MainWindow::createPickDock()
{
    m_pickDock = new QDockWidget("Pick", this);
    m_pickDock->setObjectName("pickDock");
    m_pickDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    m_pickDock->setTitleBarWidget(createFlatDockTitleBar(m_pickDock, "Pick"));

    QWidget *body = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout(body);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(4);

    m_pickLabelLine = new QLabel(m_pickReadout.labelRow());
    m_pickWorldLine = new QLabel(m_pickReadout.worldRow());
    m_pickVoxelLine = new QLabel(m_pickReadout.voxelRow());
    m_pickValueLine = new QLabel(m_pickReadout.valueRow());
    for (QLabel* l : {m_pickLabelLine, m_pickWorldLine, m_pickVoxelLine, m_pickValueLine}) {
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        lay->addWidget(l);
    }
    lay->addStretch(1);

    m_pickDock->setWidget(body);
    addDockWidget(Qt::RightDockWidgetArea, m_pickDock);

    connect(&m_pickReadout, &MNEINSPECT::PickReadoutModel::readoutChanged,
            this, &MainWindow::refreshPickDockLabels);
}

//=============================================================================================================

void MainWindow::createLayersDock()
{
    m_layersDock = new QDockWidget("Layers", this);
    m_layersDock->setObjectName("layersDock");
    m_layersDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    m_layersDock->setTitleBarWidget(createFlatDockTitleBar(m_layersDock, "Layers"));

    QWidget *body = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout(body);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(6);

    m_layerBrainCheck         = new QCheckBox("Cortical Surface");
    m_layerElectrodesCheck    = new QCheckBox("Electrodes");
    m_layerMriCheck           = new QCheckBox("MRI Slices");
    m_layerSensorsCheck       = new QCheckBox("Sensors");
    m_layerSourceOverlayCheck = new QCheckBox("Source Overlay");
    for (QCheckBox *c : {m_layerBrainCheck, m_layerElectrodesCheck, m_layerMriCheck,
                         m_layerSensorsCheck, m_layerSourceOverlayCheck}) {
        c->setChecked(true);
        lay->addWidget(c);
    }
    lay->addStretch(1);

    // Toggling a kind-checkbox flips visibility on every layer of that kind.
    auto toggleKind = [this](DISP3DLIB::SceneLayerKind kind, bool visible) {
        const auto layers = m_scene.layers();
        for (const auto& layer : layers) {
            if (layer.kind == kind) {
                m_scene.setLayerVisible(layer.id, visible);
            }
        }
    };
    connect(m_layerBrainCheck, &QCheckBox::toggled, this, [toggleKind](bool v) {
        toggleKind(DISP3DLIB::SceneLayerKind::BrainSurface, v);
    });
    connect(m_layerElectrodesCheck, &QCheckBox::toggled, this, [toggleKind](bool v) {
        toggleKind(DISP3DLIB::SceneLayerKind::Electrode, v);
    });
    connect(m_layerMriCheck, &QCheckBox::toggled, this, [toggleKind](bool v) {
        toggleKind(DISP3DLIB::SceneLayerKind::MriSlice, v);
    });
    connect(m_layerSensorsCheck, &QCheckBox::toggled, this, [toggleKind](bool v) {
        toggleKind(DISP3DLIB::SceneLayerKind::Sensor, v);
    });
    connect(m_layerSourceOverlayCheck, &QCheckBox::toggled, this, [toggleKind](bool v) {
        toggleKind(DISP3DLIB::SceneLayerKind::SourceOverlay, v);
    });

    m_layersDock->setWidget(body);
    addDockWidget(Qt::RightDockWidgetArea, m_layersDock);
}

//=============================================================================================================

void MainWindow::createOverlayDock()
{
    m_overlayDock = new QDockWidget("Overlay", this);
    m_overlayDock->setObjectName("overlayDock");
    m_overlayDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    m_overlayDock->setTitleBarWidget(createFlatDockTitleBar(m_overlayDock, "Overlay"));

    QWidget *body = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout(body);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(6);

    QFormLayout *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignLeft);

    m_overlaySourceCombo = new QComboBox;
    m_overlaySourceCombo->addItems({"iEEG amplitude", "iEEG power",
                                    "Source value", "MRI intensity"});
    form->addRow("Data source", m_overlaySourceCombo);

    m_overlayCmapCombo = new QComboBox;
    m_overlayCmapCombo->addItems({"Sequential", "Divergent", "Hot/Cool"});
    form->addRow("Colormap", m_overlayCmapCombo);

    m_overlayFminSpin = new QDoubleSpinBox;
    m_overlayFminSpin->setRange(-1e6, 1e6);
    m_overlayFminSpin->setDecimals(4);
    m_overlayFminSpin->setValue(m_scene.overlayFmin());
    form->addRow("fmin", m_overlayFminSpin);

    m_overlayFmidSpin = new QDoubleSpinBox;
    m_overlayFmidSpin->setRange(-1e6, 1e6);
    m_overlayFmidSpin->setDecimals(4);
    m_overlayFmidSpin->setValue(m_scene.overlayFmid());
    form->addRow("fmid", m_overlayFmidSpin);

    m_overlayFmaxSpin = new QDoubleSpinBox;
    m_overlayFmaxSpin->setRange(-1e6, 1e6);
    m_overlayFmaxSpin->setDecimals(4);
    m_overlayFmaxSpin->setValue(m_scene.overlayFmax());
    form->addRow("fmax", m_overlayFmaxSpin);

    lay->addLayout(form);

    QHBoxLayout *timeRow = new QHBoxLayout;
    timeRow->addWidget(new QLabel("Time"));
    m_overlayTimeSlider = new QSlider(Qt::Horizontal);
    m_overlayTimeSlider->setRange(0, 1000);
    m_overlayTimeSlider->setValue(0);
    timeRow->addWidget(m_overlayTimeSlider, 1);
    m_overlayTimeLabel = new QLabel("0.000 s");
    timeRow->addWidget(m_overlayTimeLabel);
    lay->addLayout(timeRow);

    m_overlayColorBar = new OverlayColorBar;
    m_overlayColorBar->setRange(m_scene.overlayFmin(),
                                m_scene.overlayFmid(),
                                m_scene.overlayFmax());
    lay->addWidget(m_overlayColorBar);

    lay->addStretch(1);

    m_overlayDock->setWidget(body);
    addDockWidget(Qt::RightDockWidgetArea, m_overlayDock);

    auto pushThresholds = [this]() { pushOverlayToScene(); };
    connect(m_overlayFminSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, pushThresholds);
    connect(m_overlayFmidSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, pushThresholds);
    connect(m_overlayFmaxSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, pushThresholds);
    connect(m_overlayCmapCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
                if (m_overlayColorBar) {
                    m_overlayColorBar->setColormap(static_cast<OverlayColorBar::Colormap>(idx));
                }
            });
    connect(m_overlayTimeSlider, &QSlider::valueChanged, this, [this](int v) {
        // Map [0, 1000] -> [0, 1] s by default; downstream consumers map further.
        const double seconds = static_cast<double>(v) / 1000.0;
        m_overlayTimeLabel->setText(QString::number(seconds, 'f', 3) + " s");
        m_scene.setTimeCursor(seconds);
    });

    // Bidirectional sync: external setTimeCursor / setOverlayThresholds
    // updates should reflect in the dock without firing a feedback loop.
    connect(&m_scene, &DISP3DLIB::MultimodalScene::overlayThresholdsChanged,
            this, [this](float fmin, float fmid, float fmax) {
        QSignalBlocker b1(m_overlayFminSpin);
        QSignalBlocker b2(m_overlayFmidSpin);
        QSignalBlocker b3(m_overlayFmaxSpin);
        m_overlayFminSpin->setValue(fmin);
        m_overlayFmidSpin->setValue(fmid);
        m_overlayFmaxSpin->setValue(fmax);
        if (m_overlayColorBar) {
            m_overlayColorBar->setRange(fmin, fmid, fmax);
        }
    });
    connect(&m_scene, &DISP3DLIB::MultimodalScene::timeCursorChanged,
            this, [this](double seconds) {
        QSignalBlocker b(m_overlayTimeSlider);
        const int v = qBound(0, static_cast<int>(std::lround(seconds * 1000.0)), 1000);
        m_overlayTimeSlider->setValue(v);
        m_overlayTimeLabel->setText(QString::number(seconds, 'f', 3) + " s");
    });
}

//=============================================================================================================

void MainWindow::onLoadElectrodes()
{
    const QString filter = QStringLiteral("Electrode data (*.fif *.csv);;FIFF (*.fif);;CSV (*.csv);;All files (*)");
    const QString path = QFileDialog::getOpenFileName(this, tr("Load Electrodes"),
                                                      QString(), filter);
    if (path.isEmpty()) {
        return;
    }
    const QString suffix = QFileInfo(path).suffix().toLower();
    const bool ok = (suffix == QLatin1String("csv"))
                    ? m_electrodesPlugin.loadCsv(path)
                    : m_electrodesPlugin.loadFiff(path);
    if (!ok) {
        QMessageBox::warning(this, tr("Load Electrodes"),
                             tr("Failed to load electrode data from:\n%1").arg(path));
        return;
    }
    if (m_statusLabel) {
        m_statusLabel->setText(QStringLiteral("Loaded electrodes: %1 (%2 contacts)")
                                .arg(QFileInfo(path).fileName())
                                .arg(m_electrodesPlugin.contactCount()));
    }
    if (m_layerElectrodesCheck) {
        m_layerElectrodesCheck->setChecked(true);
    }
}

//=============================================================================================================

void MainWindow::onLoadMri()
{
    const QString filter = QStringLiteral("MRI volume (*.mgh *.mgz);;All files (*)");
    const QString path = QFileDialog::getOpenFileName(this, tr("Load MRI"),
                                                      QString(), filter);
    if (path.isEmpty()) {
        return;
    }
    if (!m_mriSlicesPlugin.loadVolume(path)) {
        QMessageBox::warning(this, tr("Load MRI"),
                             tr("Failed to load MRI volume from:\n%1").arg(path));
        return;
    }
    if (m_statusLabel) {
        m_statusLabel->setText(QStringLiteral("Loaded MRI: %1")
                                .arg(QFileInfo(path).fileName()));
    }
    if (m_layerMriCheck) {
        m_layerMriCheck->setChecked(true);
    }
}

//=============================================================================================================

void MainWindow::pushOverlayToScene()
{
    if (!m_overlayFminSpin || !m_overlayFmidSpin || !m_overlayFmaxSpin) {
        return;
    }
    const float fmin = static_cast<float>(m_overlayFminSpin->value());
    const float fmid = static_cast<float>(m_overlayFmidSpin->value());
    const float fmax = static_cast<float>(m_overlayFmaxSpin->value());
    m_scene.setOverlayThresholds(fmin, fmid, fmax);
    if (m_overlayColorBar) {
        m_overlayColorBar->setRange(m_scene.overlayFmin(),
                                    m_scene.overlayFmid(),
                                    m_scene.overlayFmax());
    }
}

//=============================================================================================================

void MainWindow::refreshPickDockLabels()
{
    if (!m_pickLabelLine) {
        return;
    }
    m_pickLabelLine->setText(m_pickReadout.labelRow());
    m_pickWorldLine->setText(m_pickReadout.worldRow());
    m_pickVoxelLine->setText(m_pickReadout.voxelRow());
    m_pickValueLine->setText(m_pickReadout.valueRow());
}