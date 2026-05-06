//=============================================================================================================
/**
 * @file     mne_align.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    Implementation of the MNE Align main window.
 */

#include "mne_align.h"

#include "acquired_points.h"
#include "align_3d_view.h"
#include "align_wizard.h"
#include "polhemus_connection.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_dig_point_set.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <mne/mne_icp.h>
#include <mne/mne_project_to_surface.h>
#include <fiff/fiff_coord_trans.h>

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>

using namespace MNEALIGN;

//=============================================================================================================

MneAlign::MneAlign(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("MNE Align"));
    resize(1100, 720);

    m_pPoints    = new AcquiredPoints(this);
    m_pDigitizer = new PolhemusConnection(this);

    buildUi();
    buildMenus();

    connect(m_pDigitizer, &PolhemusConnection::connectedChanged,
            this, &MneAlign::onDigitizerConnectedChanged);
    connect(m_pDigitizer, &PolhemusConnection::errorOccurred, this,
            [this](const QString& msg) {
                statusBar()->showMessage(QStringLiteral("Digitizer: %1").arg(msg), 5000);
            });

    connect(m_pWizard, &AlignWizard::requestExport,
            this, &MneAlign::onWizardExportRequest);
    connect(m_pWizard, &AlignWizard::requestIcpFit,
            this, &MneAlign::onIcpFit);
    connect(m_pWizard, &AlignWizard::bemPathChanged,
            this, &MneAlign::onWizardBemPathChanged);
        connect(m_pWizard, &AlignWizard::stepChanged,
            this, &MneAlign::onWizardStepChanged);

    onDigitizerConnectedChanged(false);
    onWizardStepChanged(m_pWizard->currentStep());

    loadSettings();
}

MneAlign::~MneAlign() = default;

//=============================================================================================================

void MneAlign::buildUi()
{
    m_pWizard = new AlignWizard(m_pPoints, m_pDigitizer, this);
    m_pView3d = new Align3DView(m_pPoints, this);
    m_pWizard->setMinimumWidth(240);

    m_pSplitter = new QSplitter(Qt::Horizontal, this);
    m_pSplitter->addWidget(m_pWizard);
    m_pSplitter->addWidget(m_pView3d);
    m_pSplitter->setChildrenCollapsible(false);
    m_pSplitter->setStretchFactor(0, 0);
    m_pSplitter->setStretchFactor(1, 4);
    m_pSplitter->setSizes({280, 920});
    setCentralWidget(m_pSplitter);

    m_pStatusBem       = new QLabel(QStringLiteral("BEM: (none)"), this);
    m_pStatusDigitizer = new QLabel(QStringLiteral("Digitizer: disconnected"), this);
    statusBar()->addPermanentWidget(m_pStatusBem);
    statusBar()->addPermanentWidget(m_pStatusDigitizer);

    if (m_pDigitizer && m_pView3d) {
        connect(m_pDigitizer, &PolhemusConnection::pointReceived,
                m_pView3d, &Align3DView::setLiveDigitizerPose);
        connect(m_pDigitizer, &PolhemusConnection::connectedChanged,
                m_pView3d, &Align3DView::setDigitizerConnected);
    }
    if (m_pView3d && m_pWizard) {
        connect(m_pView3d, &Align3DView::surfacePointDoubleClicked,
                m_pWizard, &AlignWizard::onSurfaceDoubleClicked);
        // Push updated tracker→MRI transform to the wizard whenever points change
        connect(m_pPoints, &AcquiredPoints::pointsChanged, this, [this]{
            if (m_pWizard && m_pView3d)
                m_pWizard->setTrackerTransform(m_pView3d->trackerToMri());
        });
    }
}

void MneAlign::buildMenus()
{
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("Open &BEM…"), this, &MneAlign::onOpenBem);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("E&xit"), this, &QWidget::close);

    auto* digMenu = menuBar()->addMenu(QStringLiteral("&Digitizer"));
    digMenu->addAction(QStringLiteral("&Connect…"),    this, &MneAlign::onConnectDigitizer);
    digMenu->addAction(QStringLiteral("&Disconnect"),  this, &MneAlign::onDisconnectDigitizer);

    auto* helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    helpMenu->addAction(QStringLiteral("&About"), this, &MneAlign::onAbout);

    auto* navToolBar = addToolBar(QStringLiteral("Workflow"));
    navToolBar->setMovable(false);
    m_pBackAction = navToolBar->addAction(QStringLiteral("Back"), m_pWizard, &AlignWizard::back);
    m_pNextAction = navToolBar->addAction(QStringLiteral("Next"), m_pWizard, &AlignWizard::next);
    navToolBar->addSeparator();
    m_pStepLabel = new QLabel(this);
    navToolBar->addWidget(m_pStepLabel);
    navToolBar->addSeparator();
    navToolBar->addWidget(new QLabel(QStringLiteral("Views:"), this));
    m_pViewCountCombo = new QComboBox(this);
    m_pViewCountCombo->addItems({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4")});
    m_pViewCountCombo->setCurrentIndex(0);
    navToolBar->addWidget(m_pViewCountCombo);
    navToolBar->addWidget(new QLabel(QStringLiteral("Camera:"), this));
    m_pCameraPresetCombo = new QComboBox(this);
    m_pCameraPresetCombo->addItems({
        QStringLiteral("Top"),
        QStringLiteral("Perspective"),
        QStringLiteral("Front"),
        QStringLiteral("Left"),
        QStringLiteral("Bottom"),
        QStringLiteral("Back"),
        QStringLiteral("Right")
    });
    m_pCameraPresetCombo->setCurrentIndex(1);
    navToolBar->addWidget(m_pCameraPresetCombo);
    navToolBar->addWidget(new QLabel(QStringLiteral("Render:"), this));
    m_pRenderModeCombo = new QComboBox(this);
    m_pRenderModeCombo->addItems({QStringLiteral("Anatomical"), QStringLiteral("Holographic")});
    navToolBar->addWidget(m_pRenderModeCombo);
    navToolBar->addWidget(new QLabel(QStringLiteral("Focus:"), this));
    m_pCameraFocusCombo = new QComboBox(this);
    m_pCameraFocusCombo->addItems({QStringLiteral("Brain"), QStringLiteral("Pointer")});
    navToolBar->addWidget(m_pCameraFocusCombo);
    navToolBar->addWidget(new QLabel(QStringLiteral("Pen:"), this));
    m_pPenStationCombo = new QComboBox(this);
    m_pPenStationCombo->addItems({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4")});
    navToolBar->addWidget(m_pPenStationCombo);

    connect(m_pViewCountCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (m_pView3d) {
                    m_pView3d->setViewCount(index + 1);
                }
            });
    connect(m_pCameraPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (m_pView3d) {
                    m_pView3d->setCameraPreset(index);
                }
            });
    connect(m_pRenderModeCombo, &QComboBox::currentTextChanged,
            this, [this](const QString& text) {
                if (m_pView3d) {
                    m_pView3d->setRenderMode(text);
                }
            });
    connect(m_pCameraFocusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (m_pView3d) {
                    m_pView3d->setCameraFocus(index == 0 ? CameraFocus::Brain
                                                         : CameraFocus::Pointer);
                }
            });
    connect(m_pPenStationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                const int station = index + 1;
                if (m_pView3d) m_pView3d->setPenStation(station);
                if (m_pWizard) m_pWizard->setPenStation(station);
            });

    // Push-based UI sync: when BrainView changes its persisted state —
    // either at startup (after loadMultiViewSettings) or on user action —
    // it forwards through Align3DView and we update the combos here.
    // blockSignals prevents the combo's currentIndexChanged from looping
    // back into setViewCount/setRenderMode.
    if (m_pView3d) {
        connect(m_pView3d, &Align3DView::viewCountChanged,
                this, [this](int count) {
                    if (!m_pViewCountCombo) return;
                    QSignalBlocker block(m_pViewCountCombo);
                    m_pViewCountCombo->setCurrentIndex(qBound(1, count, 4) - 1);
                });
        connect(m_pView3d, &Align3DView::renderModeChanged,
                this, [this](const QString& mode) {
                    if (!m_pRenderModeCombo) return;
                    const int idx = m_pRenderModeCombo->findText(mode);
                    if (idx < 0) return;
                    QSignalBlocker block(m_pRenderModeCombo);
                    m_pRenderModeCombo->setCurrentIndex(idx);
                });
    }
}

//=============================================================================================================

void MneAlign::closeEvent(QCloseEvent* event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

//=============================================================================================================

void MneAlign::loadSettings()
{
    QSettings s;
    s.beginGroup(QStringLiteral("MneAlign"));

    restoreGeometry(s.value(QStringLiteral("geometry")).toByteArray());
    if (m_pSplitter) {
        const QByteArray ss = s.value(QStringLiteral("splitterState")).toByteArray();
        if (!ss.isEmpty())
            m_pSplitter->restoreState(ss);
    }

    // viewCount and renderMode are owned by BrainView and have already
    // been restored in its constructor; the toolbar combos are kept in
    // sync via the BrainView → Align3DView → MneAlign signal chain set up
    // in buildMenus(). Here we only restore the cameraPreset (which has
    // no BrainView equivalent — BrainView stores the resulting camera
    // quaternion) and the last BEM path.
    const int cameraPreset = qBound(0, s.value(QStringLiteral("cameraPreset"), 1).toInt(), 6);
    const QString bemPath  = s.value(QStringLiteral("lastBemPath")).toString();
    s.endGroup();

    if (m_pCameraPresetCombo) {
        QSignalBlocker block(m_pCameraPresetCombo);
        m_pCameraPresetCombo->setCurrentIndex(cameraPreset);
    }

    if (!bemPath.isEmpty() && QFile::exists(bemPath)) {
        if (m_pWizard)
            m_pWizard->setBemPath(bemPath);
        onWizardBemPathChanged(bemPath);
    }
}

//=============================================================================================================

void MneAlign::saveSettings()
{
    QSettings s;
    s.beginGroup(QStringLiteral("MneAlign"));
    s.setValue(QStringLiteral("geometry"),      saveGeometry());
    if (m_pSplitter)
        s.setValue(QStringLiteral("splitterState"), m_pSplitter->saveState());
    if (m_pCameraPresetCombo)
        s.setValue(QStringLiteral("cameraPreset"), m_pCameraPresetCombo->currentIndex());
    s.endGroup();
}

//=============================================================================================================

void MneAlign::onOpenBem()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open BEM surface"),
        QString(), QStringLiteral("FIFF BEM (*.fif *.fif.gz);;All files (*)"));
    if (path.isEmpty()) return;
    if (m_pWizard) {
        m_pWizard->setBemPath(path);
    }
    onWizardBemPathChanged(path);
}

void MneAlign::onWizardBemPathChanged(const QString& path)
{
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.exists()) {
        QMessageBox::warning(this, windowTitle(),
                              QStringLiteral("BEM file does not exist:\n%1").arg(path));
        return;
    }

    auto bem = std::make_shared<MNELIB::MNEBem>(file);
    if (m_pView3d) m_pView3d->setBem(bem);
    if (m_pStatusBem) {
        m_pStatusBem->setText(QStringLiteral("BEM: %1").arg(QFileInfo(path).fileName()));
    }
    statusBar()->showMessage(QStringLiteral("Loaded BEM: %1").arg(path), 4000);

    QSettings s;
    s.beginGroup(QStringLiteral("MneAlign"));
    s.setValue(QStringLiteral("lastBemPath"), path);
    s.endGroup();
}

//=============================================================================================================

void MneAlign::onConnectDigitizer()
{
    // Auto-scan first: pick a likely Fastrak/FastSCAN by USB vendor id so
    // the user doesn't have to type a device path. If detection fails, fall
    // back to a chooser populated with every visible serial port (plus a
    // "Mock backend" entry that maps to an empty port name).
    const QString detected = PolhemusConnection::autoDetectPortName();

    QString port;
    if (!detected.isEmpty()) {
        port = detected;
        statusBar()->showMessage(
            QStringLiteral("Detected digitizer on '%1' \u2014 connecting\u2026").arg(port), 3000);
    } else {
        QStringList items;
        items << QStringLiteral("Mock backend (no hardware)");
        items += PolhemusConnection::availablePorts();

        bool ok = false;
        const QString choice = QInputDialog::getItem(
            this, QStringLiteral("Connect digitizer"),
            QStringLiteral("No Fastrak auto-detected. Pick a serial port:"),
            items, /*current*/ 0, /*editable*/ false, &ok);
        if (!ok) return;
        port = (choice == items.first()) ? QString() : choice;
    }

    if (!m_pDigitizer->open(port)) {
        QMessageBox::warning(this, windowTitle(),
                              QStringLiteral("Failed to open digitizer on '%1'.")
                                  .arg(port.isEmpty() ? QStringLiteral("mock") : port));
    }
}

void MneAlign::onDisconnectDigitizer()
{
    m_pDigitizer->close();
}

void MneAlign::onDigitizerConnectedChanged(bool connected)
{
    if (!m_pStatusDigitizer) return;
    m_pStatusDigitizer->setText(connected
        ? QStringLiteral("Digitizer: %1").arg(m_pDigitizer->backendName())
        : QStringLiteral("Digitizer: disconnected"));
}

void MneAlign::onWizardStepChanged(MNEALIGN::AlignStep step)
{
    if (m_pStepLabel) {
        m_pStepLabel->setText(QStringLiteral("Step %1/%2 — %3")
            .arg(static_cast<int>(step) + 1)
            .arg(AlignWizard::stepCount())
            .arg(AlignWizard::titleFor(step)));
    }

    if (m_pBackAction) {
        m_pBackAction->setEnabled(step != AlignStep::Setup);
    }
    if (m_pNextAction) {
        m_pNextAction->setEnabled(step != AlignStep::Export);
    }
}

//=============================================================================================================

void MneAlign::onWizardExportRequest(const QString& outPath)
{
    using namespace FIFFLIB;

    FiffDigPointSet set;
    int eegIdent = 1;
    int hspIdent = 1;
    for (const auto& p : m_pPoints->points()) {
        FiffDigPoint dig;
        dig.coord_frame = FIFFV_COORD_HEAD;
        dig.r[0] = p.position.x();
        dig.r[1] = p.position.y();
        dig.r[2] = p.position.z();
        switch (p.kind) {
            case PointKind::Fiducial:
                dig.kind  = FIFFV_POINT_CARDINAL;
                dig.ident = p.identNumber;
                break;
            case PointKind::Eeg:
                dig.kind  = FIFFV_POINT_EEG;
                dig.ident = eegIdent++;
                break;
            case PointKind::HeadShape:
                dig.kind  = FIFFV_POINT_EXTRA;
                dig.ident = hspIdent++;
                break;
        }
        set << dig;
    }

    QString err;
    if (!set.write(outPath, &err)) {
        QMessageBox::warning(this, windowTitle(),
                              QStringLiteral("Export failed:\n%1").arg(err));
        return;
    }
    statusBar()->showMessage(
        QStringLiteral("Wrote digitization to %1").arg(outPath), 5000);
}

//=============================================================================================================

void MneAlign::onAbout()
{
    QMessageBox::about(this, windowTitle(),
        QStringLiteral(
            "<h3>MNE Align</h3>"
            "<p>Stripped-down <b>mne_inspect</b> variant for Polhemus Fastrak "
            "digitisation against a head BEM and an EEG cap.</p>"
            "<p>v2.3.0</p>"));
}

void MneAlign::onIcpFit()
{
    using namespace MNELIB;
    using namespace FIFFLIB;

    if (!m_pPoints || !m_pView3d) return;
    auto bem = m_pView3d->bem();
    if (!bem || bem->isEmpty()) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("No BEM surface loaded."));
        return;
    }

    // Require all 3 twin fiducials and all 3 captured fiducials
    if (!m_pPoints->hasAllTwinFiducials()) {
        QMessageBox::warning(this, windowTitle(),
            QStringLiteral("Twin fiducials incomplete. Click NAS, LPA, RPA on the BEM surface in Step 1."));
        return;
    }
    if (!m_pPoints->hasFiducial(FiducialId::NAS)
        || !m_pPoints->hasFiducial(FiducialId::LPA)
        || !m_pPoints->hasFiducial(FiducialId::RPA)) {
        QMessageBox::warning(this, windowTitle(),
            QStringLiteral("Captured fiducials incomplete. Capture NAS, LPA, RPA with the pen in Step 2."));
        return;
    }

    // Use the progressive alignment as initial transform
    const QMatrix4x4 initQt = m_pView3d->trackerToMri();
    Eigen::Matrix4f matInit;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            matInit(r, c) = initQt(r, c);

    // Build MNEProjectToSurface from the head BEM
    MNEProjectToSurface::SPtr projSurface =
        MNEProjectToSurface::SPtr::create((*bem)[0]);

    // Build point cloud from all captured points (raw tracker space)
    const auto& pts = m_pPoints->points();
    if (pts.isEmpty()) return;

    Eigen::MatrixXf cloud(pts.size(), 3);
    for (int i = 0; i < pts.size(); ++i) {
        cloud(i, 0) = pts[i].position.x();
        cloud(i, 1) = pts[i].position.y();
        cloud(i, 2) = pts[i].position.z();
    }

    // Run ICP refinement
    FiffCoordTrans trans(FIFFV_COORD_HEAD, FIFFV_COORD_MRI, matInit);
    FiffCoordTrans::addInverse(trans);
    float rmse = 0.0f;
    if (!performIcp(projSurface, cloud, trans, rmse, false, 20, 0.001f)) {
        QMessageBox::warning(this, windowTitle(), QStringLiteral("ICP convergence failed."));
        return;
    }

    // Store the refined transform back — points stay in raw tracker space,
    // the view applies the transform for display.
    QMatrix4x4 refined;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            refined(r, c) = trans.trans(r, c);
    // TODO: push refined transform to Align3DView and persist for export

    m_pPoints->emitChanged();

    statusBar()->showMessage(
        QStringLiteral("ICP fit complete — RMSE: %1 mm").arg(rmse * 1000.0f, 0, 'f', 2), 10000);
}
