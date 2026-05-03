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

    // Restore toolbar combos without triggering applyViewConfiguration().
    const int viewCount    = qBound(1, s.value(QStringLiteral("viewCount"),    1).toInt(), 4);
    const int cameraPreset = qBound(0, s.value(QStringLiteral("cameraPreset"), 1).toInt(), 6);
    const QString renderMode = s.value(QStringLiteral("renderMode"),
                                       QStringLiteral("Anatomical")).toString();

    if (m_pViewCountCombo) {
        m_pViewCountCombo->blockSignals(true);
        m_pViewCountCombo->setCurrentIndex(viewCount - 1);
        m_pViewCountCombo->blockSignals(false);
    }
    if (m_pCameraPresetCombo) {
        m_pCameraPresetCombo->blockSignals(true);
        m_pCameraPresetCombo->setCurrentIndex(cameraPreset);
        m_pCameraPresetCombo->blockSignals(false);
    }
    if (m_pRenderModeCombo) {
        m_pRenderModeCombo->blockSignals(true);
        const int idx = m_pRenderModeCombo->findText(renderMode);
        m_pRenderModeCombo->setCurrentIndex(idx >= 0 ? idx : 0);
        m_pRenderModeCombo->blockSignals(false);
    }

    // Reload last BEM if path is still accessible.
    const QString bemPath = s.value(QStringLiteral("lastBemPath")).toString();
    s.endGroup();

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
    if (m_pViewCountCombo)
        s.setValue(QStringLiteral("viewCount"),    m_pViewCountCombo->currentIndex() + 1);
    if (m_pCameraPresetCombo)
        s.setValue(QStringLiteral("cameraPreset"), m_pCameraPresetCombo->currentIndex());
    if (m_pRenderModeCombo)
        s.setValue(QStringLiteral("renderMode"),   m_pRenderModeCombo->currentText());
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
    // Auto-scan first: pick a likely FastTrak/FastSCAN by USB vendor id so
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
            QStringLiteral("No FastTrak auto-detected. Pick a serial port:"),
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
            "<p>Stripped-down <b>mne_inspect</b> variant for Polhemus FastTrak "
            "digitisation against a head BEM and an EEG cap.</p>"
            "<p>v2.3.0</p>"));
}
