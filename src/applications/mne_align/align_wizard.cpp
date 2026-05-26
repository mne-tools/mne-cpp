//=============================================================================================================
/**
 * @file     align_wizard.cpp
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
 * @brief    Implementation of the seven-step MNE Align coregistration wizard.
 */

#include "align_wizard.h"

#include "polhemus_connection.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_dig_point_set.h>

#include <utils/montage/standard_montage.h>

#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

using namespace MNEALIGN;
using UTILSLIB::StandardMontage;

namespace {

constexpr int kSetupIdx     = 0;
constexpr int kFiducialsIdx = 1;
constexpr int kEegIdx       = 2;
constexpr int kHspIdx       = 3;
constexpr int kVerifyIdx    = 4;
constexpr int kSaveIdx      = 5;
constexpr int kDoneIdx      = 6;

/** Canonical fiducial capture order. */
constexpr FiducialId kFiducialOrder[] = {FiducialId::NAS, FiducialId::LPA, FiducialId::RPA};

QString fiducialName(FiducialId id)
{
    switch (id) {
        case FiducialId::LPA: return QStringLiteral("LPA");
        case FiducialId::NAS: return QStringLiteral("NAS");
        case FiducialId::RPA: return QStringLiteral("RPA");
    }
    return {};
}

QString systemLabel(StandardMontage::System sys)
{
    switch (sys) {
        case StandardMontage::System::Standard_1020: return QStringLiteral("10-20");
        case StandardMontage::System::Standard_1010: return QStringLiteral("10-10");
        case StandardMontage::System::Standard_1005: return QStringLiteral("10-05");
    }
    return QStringLiteral("?");
}

} // namespace

//=============================================================================================================

AlignWizard::AlignWizard(AcquiredPoints* acquired,
                         PolhemusConnection* digitizer,
                         QWidget* parent)
    : QStackedWidget(parent)
    , m_pPoints(acquired)
    , m_pDigitizer(digitizer)
{
    Q_ASSERT(m_pPoints);

    addWidget(buildSetupPage());
    addWidget(buildFiducialsPage());
    addWidget(buildEegCapPage());
    addWidget(buildHeadShapePage());
    addWidget(buildVerifyPage());
    addWidget(buildSavePage());
    addWidget(buildDonePage());

    if (m_pDigitizer) {
        connect(m_pDigitizer, &PolhemusConnection::pointReceived,
                this, &AlignWizard::onLivePoint);
        connect(m_pDigitizer, &PolhemusConnection::penButtonPressed,
                this, &AlignWizard::onPenButtonPressed);
    }
    connect(m_pPoints, &AcquiredPoints::pointsChanged,
            this, &AlignWizard::onAcquiredChanged);

    onAcquiredChanged();
    connect(this, &QStackedWidget::currentChanged, this,
            [this](int) { emit stepChanged(currentStep()); });
}

AlignWizard::~AlignWizard() = default;

//=============================================================================================================

int     AlignWizard::stepCount() { return 7; }

QString AlignWizard::titleFor(AlignStep step)
{
    switch (step) {
        case AlignStep::Setup:     return QStringLiteral("Setup");
        case AlignStep::Fiducials: return QStringLiteral("Fiducials");
        case AlignStep::EegCap:    return QStringLiteral("EEG Cap");
        case AlignStep::HeadShape: return QStringLiteral("Head Shape");
        case AlignStep::Verify:    return QStringLiteral("Verify");
        case AlignStep::Save:      return QStringLiteral("Save");
        case AlignStep::Done:      return QStringLiteral("Done");
    }
    return {};
}

AlignStep AlignWizard::currentStep() const
{
    return static_cast<AlignStep>(currentIndex());
}

void AlignWizard::goToStep(AlignStep step)
{
    setCurrentIndex(static_cast<int>(step));
}

void AlignWizard::setBemPath(const QString& path)
{
    m_bemPath = path;
    if (m_pSetupBemLabel) {
        m_pSetupBemLabel->setText(path.isEmpty()
            ? QStringLiteral("<i>(no BEM loaded)</i>")
            : QFileInfo(path).fileName());
    }
    refreshSaveUi();
    refreshDoneUi();
}

void AlignWizard::next() { if (currentIndex() < count() - 1) setCurrentIndex(currentIndex() + 1); }
void AlignWizard::back() { if (currentIndex() > 0)           setCurrentIndex(currentIndex() - 1); }

void AlignWizard::setPenStation(int station)
{
    m_penStation = qBound(1, station, 4);
}

//=============================================================================================================
// Live cursor
//=============================================================================================================

void AlignWizard::onLivePoint(int station, const QVector3D& pos, const QQuaternion& /*ori*/)
{
    // Only track the pen station for capture
    if (station != m_penStation) return;

    m_lastLivePos = pos;
    const bool wasLive = m_haveLive;
    m_haveLive = true;

    // First live sample: enable capture buttons
    if (!wasLive) {
        refreshFiducialUi();
        refreshEegUi();
        refreshHeadShapeUi();
    }

    const QVector3D mapped = m_trackerToMri.map(pos);
    const auto liveStr = QStringLiteral("live: (%1, %2, %3) m")
                             .arg(mapped.x(), 0, 'f', 4)
                             .arg(mapped.y(), 0, 'f', 4)
                             .arg(mapped.z(), 0, 'f', 4);
    if (m_pFidLiveLabel) m_pFidLiveLabel->setText(liveStr);
    if (m_pEegLiveLabel) m_pEegLiveLabel->setText(liveStr);
    if (m_pHspLiveLabel) m_pHspLiveLabel->setText(liveStr);
}

void AlignWizard::onPenButtonPressed(int station, const QVector3D& pos, const QQuaternion& /*ori*/)
{
    if (station != m_penStation) return;

    // Use the frozen position as the capture point
    m_lastLivePos = pos;
    m_haveLive    = true;

    switch (currentStep()) {
    case AlignStep::Fiducials:
        onCaptureFiducial();
        break;
    case AlignStep::EegCap:
        onCaptureEeg();
        break;
    case AlignStep::HeadShape:
        onCaptureHsp();
        break;
    default:
        break;
    }
}

void AlignWizard::onAcquiredChanged()
{
    refreshSetupTwinUi();
    refreshFiducialUi();
    refreshEegUi();
    refreshHeadShapeUi();
    refreshVerifyUi();
    refreshSaveUi();
    refreshDoneUi();
}

//=============================================================================================================
// Setup page
//=============================================================================================================

QWidget* AlignWizard::buildSetupPage()
{
    auto* page = new QWidget(this);
    auto* lay  = new QVBoxLayout(page);

    lay->addWidget(new QLabel(QStringLiteral("<h2>Step 1 — Setup</h2>"
                                              "Pick the head BEM surface and the EEG cap "
                                              "to be used for the digitisation session."), page));

    auto* bemRow = new QHBoxLayout;
    auto* bemBtn = new QPushButton(QStringLiteral("Load BEM…"), page);
    m_pSetupBemLabel = new QLabel(QStringLiteral("<i>(no BEM loaded)</i>"), page);
    m_pSetupBemLabel->setWordWrap(true);
    bemRow->addWidget(bemBtn);
    bemRow->addWidget(m_pSetupBemLabel, /*stretch*/ 1);
    lay->addLayout(bemRow);

    auto* capRow = new QHBoxLayout;
    capRow->addWidget(new QLabel(QStringLiteral("EEG cap:"), page));
    m_pSetupCapCombo = new QComboBox(page);
    m_pSetupCapCombo->addItem(QStringLiteral("10-20 (21 electrodes)"),
                               QVariant::fromValue(int(StandardMontage::System::Standard_1020)));
    m_pSetupCapCombo->addItem(QStringLiteral("10-10 (81 electrodes)"),
                               QVariant::fromValue(int(StandardMontage::System::Standard_1010)));
    m_pSetupCapCombo->addItem(QStringLiteral("10-05 (345 electrodes)"),
                               QVariant::fromValue(int(StandardMontage::System::Standard_1005)));
    capRow->addWidget(m_pSetupCapCombo, 1);
    lay->addLayout(capRow);

    lay->addWidget(new QLabel(QStringLiteral(
        "<b>Twin fiducials:</b> Select a fiducial below, then "
        "<b>double-click</b> on the BEM surface to place it."), page));

    // Fiducial selection buttons
    auto* fidRow = new QHBoxLayout;
    m_pTwinNasBtn = new QPushButton(QStringLiteral("NAS"), page);
    m_pTwinLpaBtn = new QPushButton(QStringLiteral("LPA"), page);
    m_pTwinRpaBtn = new QPushButton(QStringLiteral("RPA"), page);
    m_pTwinNasBtn->setCheckable(true);
    m_pTwinLpaBtn->setCheckable(true);
    m_pTwinRpaBtn->setCheckable(true);
    m_pTwinNasBtn->setChecked(true); // NAS selected by default
    fidRow->addWidget(m_pTwinNasBtn);
    fidRow->addWidget(m_pTwinLpaBtn);
    fidRow->addWidget(m_pTwinRpaBtn);
    lay->addLayout(fidRow);

    auto selectFid = [this](FiducialId id) {
        m_selectedTwinFid = id;
        m_pTwinNasBtn->setChecked(id == FiducialId::NAS);
        m_pTwinLpaBtn->setChecked(id == FiducialId::LPA);
        m_pTwinRpaBtn->setChecked(id == FiducialId::RPA);
    };
    connect(m_pTwinNasBtn, &QPushButton::clicked, this, [selectFid]{ selectFid(FiducialId::NAS); });
    connect(m_pTwinLpaBtn, &QPushButton::clicked, this, [selectFid]{ selectFid(FiducialId::LPA); });
    connect(m_pTwinRpaBtn, &QPushButton::clicked, this, [selectFid]{ selectFid(FiducialId::RPA); });

    m_pSetupTwinFidLabel = new QLabel(page);
    m_pSetupTwinFidLabel->setTextFormat(Qt::RichText);
    lay->addWidget(m_pSetupTwinFidLabel);

    lay->addStretch(1);

    connect(bemBtn, &QPushButton::clicked, this, &AlignWizard::onPickBemFile);
    connect(m_pSetupCapCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &AlignWizard::onCapComboChanged);
    return page;
}

void AlignWizard::onPickBemFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open BEM surface"),
        QString(), QStringLiteral("FIFF BEM (*.fif *.fif.gz);;All files (*)"));
    if (path.isEmpty()) return;

    setBemPath(path);
    emit bemPathChanged(path);
}

void AlignWizard::onCapComboChanged(int index)
{
    if (!m_pSetupCapCombo) return;
    const QVariant v = m_pSetupCapCombo->itemData(index);
    if (!v.isValid()) return;
    m_capSystem = static_cast<StandardMontage::System>(v.toInt());
    emit capChanged(m_capSystem);
    refreshEegUi();
}

void AlignWizard::onSurfaceDoubleClicked(const QVector3D& worldPos)
{
    // Only handle double-clicks in the Setup step
    if (currentStep() != AlignStep::Setup) return;

    // Place the currently selected twin fiducial
    m_pPoints->setTwinFiducial(m_selectedTwinFid, worldPos);
    refreshSetupTwinUi();

    // Auto-advance to next unset fiducial
    for (FiducialId id : kFiducialOrder) {
        if (!m_pPoints->hasTwinFiducial(id)) {
            m_selectedTwinFid = id;
            if (m_pTwinNasBtn) m_pTwinNasBtn->setChecked(id == FiducialId::NAS);
            if (m_pTwinLpaBtn) m_pTwinLpaBtn->setChecked(id == FiducialId::LPA);
            if (m_pTwinRpaBtn) m_pTwinRpaBtn->setChecked(id == FiducialId::RPA);
            break;
        }
    }
}

void AlignWizard::refreshSetupTwinUi()
{
    if (!m_pSetupTwinFidLabel) return;

    QString html;
    for (FiducialId id : kFiducialOrder) {
        const bool have = m_pPoints->hasTwinFiducial(id);
        const bool selected = (id == m_selectedTwinFid);
        const QString mark = have ? QStringLiteral("✔") : QStringLiteral("○");
        QString posStr;
        if (have) {
            const QVector3D p = m_pPoints->twinFiducial(id);
            posStr = QStringLiteral(" (%1, %2, %3)")
                         .arg(p.x(), 0, 'f', 4).arg(p.y(), 0, 'f', 4).arg(p.z(), 0, 'f', 4);
        }
        const QString style = selected ? QStringLiteral(" style='color:#FFD700;'") : QString();
        html += QStringLiteral("<span%4>%1 <b>%2</b>%3</span><br>")
                    .arg(mark, fiducialName(id), posStr, style);
    }

    html += QStringLiteral("Double-click on surface to place <b>%1</b>")
                .arg(fiducialName(m_selectedTwinFid));
    m_pSetupTwinFidLabel->setText(html);
}

//=============================================================================================================
// Fiducials page
//=============================================================================================================

QWidget* AlignWizard::buildFiducialsPage()
{
    auto* page = new QWidget(this);
    auto* lay  = new QVBoxLayout(page);

    lay->addWidget(new QLabel(QStringLiteral("<h2>Step 2 — Fiducials</h2>"
                                              "Capture <b>NAS</b>, <b>LPA</b>, <b>RPA</b> in turn. "
                                              "Hold the stylus on the landmark and press the "
                                              "<b>pen button</b> to capture, or use the button below."), page));

    m_pFidStatusLabel = new QLabel(page);
    m_pFidStatusLabel->setTextFormat(Qt::RichText);
    lay->addWidget(m_pFidStatusLabel);

    // Fiducial selection buttons — click to (re)select target
    auto* fidRow = new QHBoxLayout;
    m_pFidNasBtn = new QPushButton(QStringLiteral("NAS"), page);
    m_pFidLpaBtn = new QPushButton(QStringLiteral("LPA"), page);
    m_pFidRpaBtn = new QPushButton(QStringLiteral("RPA"), page);
    m_pFidNasBtn->setCheckable(true);
    m_pFidLpaBtn->setCheckable(true);
    m_pFidRpaBtn->setCheckable(true);
    m_pFidNasBtn->setChecked(true);
    fidRow->addWidget(m_pFidNasBtn);
    fidRow->addWidget(m_pFidLpaBtn);
    fidRow->addWidget(m_pFidRpaBtn);
    lay->addLayout(fidRow);

    auto selectFidTarget = [this](FiducialId id) {
        m_selectedFiducial = id;
        m_pFidNasBtn->setChecked(id == FiducialId::NAS);
        m_pFidLpaBtn->setChecked(id == FiducialId::LPA);
        m_pFidRpaBtn->setChecked(id == FiducialId::RPA);
        refreshFiducialUi();
    };
    connect(m_pFidNasBtn, &QPushButton::clicked, this, [selectFidTarget]{ selectFidTarget(FiducialId::NAS); });
    connect(m_pFidLpaBtn, &QPushButton::clicked, this, [selectFidTarget]{ selectFidTarget(FiducialId::LPA); });
    connect(m_pFidRpaBtn, &QPushButton::clicked, this, [selectFidTarget]{ selectFidTarget(FiducialId::RPA); });

    m_pFidLiveLabel = new QLabel(QStringLiteral("live: —"), page);
    lay->addWidget(m_pFidLiveLabel);

    auto* btnRow = new QHBoxLayout;
    m_pFidCaptureBtn = new QPushButton(QStringLiteral("Capture"), page);
    m_pFidUndoBtn    = new QPushButton(QStringLiteral("Undo last"), page);
    auto* fidLoadBtn = new QPushButton(QStringLiteral("Load FIFF…"), page);
    fidLoadBtn->setToolTip(QStringLiteral(
        "Import an existing digitisation (ISOTRAK) from a FIFF .fif file "
        "instead of capturing live with the Polhemus FASTRAK."));
    btnRow->addWidget(m_pFidCaptureBtn);
    btnRow->addWidget(m_pFidUndoBtn);
    btnRow->addWidget(fidLoadBtn);
    btnRow->addStretch(1);
    lay->addLayout(btnRow);

    lay->addStretch(1);

    connect(m_pFidCaptureBtn, &QPushButton::clicked, this, &AlignWizard::onCaptureFiducial);
    connect(m_pFidUndoBtn,    &QPushButton::clicked, this, &AlignWizard::onUndoFiducial);
    connect(fidLoadBtn,       &QPushButton::clicked, this, &AlignWizard::onLoadDigiFiff);
    return page;
}

FiducialId AlignWizard::nextFiducial(bool* allDone) const
{
    for (FiducialId id : kFiducialOrder) {
        if (!m_pPoints->hasFiducial(id)) {
            if (allDone) *allDone = false;
            return id;
        }
    }
    if (allDone) *allDone = true;
    return FiducialId::NAS;
}

void AlignWizard::refreshFiducialUi()
{
    if (!m_pFidStatusLabel) return;

    QString html;
    for (FiducialId id : kFiducialOrder) {
        const bool have = m_pPoints->hasFiducial(id);
        const bool selected = (id == m_selectedFiducial);
        const QString mark = have ? QStringLiteral("✔") : QStringLiteral("○");
        const QString style = selected ? QStringLiteral(" style='color:#FFD700;'") : QString();
        html += QStringLiteral("<span%3>%1 <b>%2</b></span><br>")
                    .arg(mark, fiducialName(id), style);
    }
    html += QStringLiteral("Target: <b>%1</b> — press pen button or Capture")
                .arg(fiducialName(m_selectedFiducial));
    m_pFidStatusLabel->setText(html);

    if (m_pFidCaptureBtn) {
        m_pFidCaptureBtn->setEnabled(m_haveLive);
        m_pFidCaptureBtn->setText(
            QStringLiteral("Capture %1").arg(fiducialName(m_selectedFiducial)));
    }
    if (m_pFidUndoBtn) {
        m_pFidUndoBtn->setEnabled(m_pPoints->countOf(PointKind::Fiducial) > 0);
    }
}

void AlignWizard::onCaptureFiducial()
{
    if (!m_haveLive) return;

    const FiducialId target = m_selectedFiducial;

    // Remove existing capture of this fiducial if re-recording
    if (m_pPoints->hasFiducial(target)) {
        m_pPoints->removeFiducial(target);
    }

    DigitizedPoint p;
    p.kind        = PointKind::Fiducial;
    p.label       = fiducialName(target);
    p.identNumber = static_cast<int>(target);
    p.position    = m_lastLivePos;
    m_pPoints->append(p);

    // Auto-advance to next uncaptured fiducial
    for (FiducialId id : kFiducialOrder) {
        if (!m_pPoints->hasFiducial(id)) {
            m_selectedFiducial = id;
            if (m_pFidNasBtn) m_pFidNasBtn->setChecked(id == FiducialId::NAS);
            if (m_pFidLpaBtn) m_pFidLpaBtn->setChecked(id == FiducialId::LPA);
            if (m_pFidRpaBtn) m_pFidRpaBtn->setChecked(id == FiducialId::RPA);
            break;
        }
    }
    refreshFiducialUi();
}

void AlignWizard::onUndoFiducial()
{
    m_pPoints->undoLast(PointKind::Fiducial);
}

//=============================================================================================================
// EEG cap page
//=============================================================================================================

QWidget* AlignWizard::buildEegCapPage()
{
    auto* page = new QWidget(this);
    auto* lay  = new QVBoxLayout(page);

    lay->addWidget(new QLabel(QStringLiteral("<h2>Step 3 — EEG electrodes</h2>"
                                              "Walk the cap and capture every electrode. "
                                              "Press the <b>pen button</b> or the button below. "
                                              "Captured electrodes are marked with ✔."), page));

    m_pEegList = new QListWidget(page);
    m_pEegList->setSelectionMode(QAbstractItemView::NoSelection);
    lay->addWidget(m_pEegList, 1);

    m_pEegLiveLabel = new QLabel(QStringLiteral("live: —"), page);
    lay->addWidget(m_pEegLiveLabel);

    auto* btnRow = new QHBoxLayout;
    m_pEegCaptureBtn = new QPushButton(QStringLiteral("Capture"), page);
    m_pEegUndoBtn    = new QPushButton(QStringLiteral("Undo last"), page);
    btnRow->addWidget(m_pEegCaptureBtn);
    btnRow->addWidget(m_pEegUndoBtn);
    btnRow->addStretch(1);
    lay->addLayout(btnRow);

    connect(m_pEegCaptureBtn, &QPushButton::clicked, this, &AlignWizard::onCaptureEeg);
    connect(m_pEegUndoBtn,    &QPushButton::clicked, this, &AlignWizard::onUndoEeg);
    return page;
}

QString AlignWizard::nextEegLabel(int* outIdent) const
{
    const QStringList names = StandardMontage::getElectrodeNames(m_capSystem);
    const int captured = m_pPoints->countOf(PointKind::Eeg);
    if (captured >= names.size()) {
        if (outIdent) *outIdent = 0;
        return {};
    }
    if (outIdent) *outIdent = captured + 1;
    return names.at(captured);
}

void AlignWizard::refreshEegUi()
{
    if (!m_pEegList) return;

    const QStringList names = StandardMontage::getElectrodeNames(m_capSystem);
    const int captured      = m_pPoints->countOf(PointKind::Eeg);

    if (m_pEegList->count() != names.size()) {
        m_pEegList->clear();
        for (const QString& n : names) {
            m_pEegList->addItem(n);
        }
    }
    for (int i = 0; i < m_pEegList->count(); ++i) {
        QListWidgetItem* it = m_pEegList->item(i);
        const bool done = (i < captured);
        const bool nextItem = (i == captured);
        it->setText(QStringLiteral("%1 %2%3")
                        .arg(done ? QStringLiteral("✔") : QStringLiteral("○"),
                             names.at(i),
                             nextItem ? QStringLiteral("    ← next") : QString()));
    }
    if (captured < m_pEegList->count()) {
        m_pEegList->scrollToItem(m_pEegList->item(captured));
    }

    int nextIdent = 0;
    const QString nextName = nextEegLabel(&nextIdent);
    if (m_pEegCaptureBtn) {
        m_pEegCaptureBtn->setEnabled(!nextName.isEmpty() && m_haveLive);
        m_pEegCaptureBtn->setText(nextName.isEmpty()
            ? QStringLiteral("Capture")
            : QStringLiteral("Capture %1").arg(nextName));
    }
    if (m_pEegUndoBtn) {
        m_pEegUndoBtn->setEnabled(captured > 0);
    }
}

void AlignWizard::onCaptureEeg()
{
    if (!m_haveLive) return;
    int ident = 0;
    const QString name = nextEegLabel(&ident);
    if (name.isEmpty()) return;

    DigitizedPoint p;
    p.kind        = PointKind::Eeg;
    p.label       = name;
    p.identNumber = ident;
    p.position    = m_lastLivePos;
    m_pPoints->append(p);
}

void AlignWizard::onUndoEeg()
{
    m_pPoints->undoLast(PointKind::Eeg);
}

//=============================================================================================================
// Head-shape page
//=============================================================================================================

QWidget* AlignWizard::buildHeadShapePage()
{
    auto* page = new QWidget(this);
    auto* lay  = new QVBoxLayout(page);

    lay->addWidget(new QLabel(QStringLiteral("<h2>Step 4 — Head shape</h2>"
                                              "Sweep the stylus over the scalp and press the "
                                              "<b>pen button</b> or <b>Capture</b> below to add "
                                              "head-shape points (FIFF <code>EXTRA</code>)."), page));

    m_pHspCountLabel = new QLabel(page);
    lay->addWidget(m_pHspCountLabel);

    m_pHspLiveLabel  = new QLabel(QStringLiteral("live: —"), page);
    lay->addWidget(m_pHspLiveLabel);

    auto* btnRow = new QHBoxLayout;
    m_pHspCaptureBtn = new QPushButton(QStringLiteral("Capture"), page);
    m_pHspUndoBtn    = new QPushButton(QStringLiteral("Undo last"), page);
    btnRow->addWidget(m_pHspCaptureBtn);
    btnRow->addWidget(m_pHspUndoBtn);
    btnRow->addStretch(1);
    lay->addLayout(btnRow);

    lay->addStretch(1);

    connect(m_pHspCaptureBtn, &QPushButton::clicked, this, &AlignWizard::onCaptureHsp);
    connect(m_pHspUndoBtn,    &QPushButton::clicked, this, &AlignWizard::onUndoHsp);
    return page;
}

void AlignWizard::refreshHeadShapeUi()
{
    if (!m_pHspCountLabel) return;
    const int n = m_pPoints->countOf(PointKind::HeadShape);
    m_pHspCountLabel->setText(QStringLiteral("Captured head-shape points: <b>%1</b>").arg(n));
    if (m_pHspCaptureBtn) m_pHspCaptureBtn->setEnabled(m_haveLive);
    if (m_pHspUndoBtn)    m_pHspUndoBtn->setEnabled(n > 0);
}

void AlignWizard::onCaptureHsp()
{
    if (!m_haveLive) return;
    const int n = m_pPoints->countOf(PointKind::HeadShape);
    DigitizedPoint p;
    p.kind        = PointKind::HeadShape;
    p.label       = QStringLiteral("HSP-%1").arg(n + 1);
    p.identNumber = n + 1;
    p.position    = m_lastLivePos;
    m_pPoints->append(p);
}

void AlignWizard::onUndoHsp()
{
    m_pPoints->undoLast(PointKind::HeadShape);
}

//=============================================================================================================
// Verify page
//=============================================================================================================

QWidget* AlignWizard::buildVerifyPage()
{
    auto* page = new QWidget(this);
    auto* lay  = new QVBoxLayout(page);

    lay->addWidget(new QLabel(QStringLiteral(
        "<h2>Step 5 — Verify</h2>"
        "Run ICP to refine the fiducial-based coregistration, then "
        "inspect the per-fiducial residuals and overall RMSE before "
        "saving the transform."), page));

    m_pVerifyIcpBtn = new QPushButton(QStringLiteral("Run ICP Fit"), page);
    lay->addWidget(m_pVerifyIcpBtn);

    m_pVerifyLabel = new QLabel(page);
    m_pVerifyLabel->setTextFormat(Qt::RichText);
    m_pVerifyLabel->setWordWrap(true);
    lay->addWidget(m_pVerifyLabel);

    lay->addStretch(1);

    connect(m_pVerifyIcpBtn, &QPushButton::clicked, this,
            [this]() { emit requestIcpFit(); });
    return page;
}

void AlignWizard::refreshVerifyUi()
{
    if (!m_pVerifyLabel) return;

    if (!m_haveIcpResult) {
        m_pVerifyLabel->setText(QStringLiteral(
            "<i>No ICP fit yet. Capture or load all three fiducials, then "
            "click <b>Run ICP Fit</b> above.</i>"));
        return;
    }

    QString html = QStringLiteral(
        "<b>Source:</b> %1<br>"
        "<b>RMSE:</b> %2 mm<br><br>"
        "<b>Per-point residuals:</b><br>")
        .arg(m_lastIcpSource)
        .arg(m_lastIcpRmse * 1000.0f, 0, 'f', 2);

    if (m_lastIcpResiduals.isEmpty()) {
        html += QStringLiteral("<i>(none reported)</i>");
    } else {
        for (const auto& r : m_lastIcpResiduals) {
            html += QStringLiteral("&nbsp;&nbsp;%1: %2 mm<br>")
                        .arg(r.first)
                        .arg(r.second * 1000.0f, 0, 'f', 2);
        }
    }
    m_pVerifyLabel->setText(html);
}

//=============================================================================================================
// Save page
//=============================================================================================================

QWidget* AlignWizard::buildSavePage()
{
    auto* page = new QWidget(this);
    auto* lay  = new QVBoxLayout(page);

    lay->addWidget(new QLabel(QStringLiteral(
        "<h2>Step 6 — Save</h2>"
        "Write the head→MRI coregistration to a <code>-trans.fif</code> "
        "file. Optionally, also export the captured digitisation as a "
        "FIFF <code>FIFFB_ISOTRAK</code> point set."), page));

    m_pSaveSummaryLabel = new QLabel(page);
    m_pSaveSummaryLabel->setTextFormat(Qt::RichText);
    lay->addWidget(m_pSaveSummaryLabel);

    m_pIcpStatusLabel = new QLabel(page);
    m_pIcpStatusLabel->setTextFormat(Qt::RichText);
    lay->addWidget(m_pIcpStatusLabel);

    m_pSaveTransBtn = new QPushButton(QStringLiteral("Save -trans.fif…"), page);
    lay->addWidget(m_pSaveTransBtn);

    m_pSaveDigiBtn  = new QPushButton(QStringLiteral("Save digitisation FIFF…"), page);
    lay->addWidget(m_pSaveDigiBtn);

    lay->addStretch(1);

    connect(m_pSaveTransBtn, &QPushButton::clicked, this, &AlignWizard::onSaveTransClicked);
    connect(m_pSaveDigiBtn,  &QPushButton::clicked, this, &AlignWizard::onSaveDigitisationClicked);
    return page;
}

void AlignWizard::refreshSaveUi()
{
    if (m_pSaveSummaryLabel) {
        const int nFid = m_pPoints->countOf(PointKind::Fiducial);
        const int nEeg = m_pPoints->countOf(PointKind::Eeg);
        const int nHsp = m_pPoints->countOf(PointKind::HeadShape);
        m_pSaveSummaryLabel->setText(
            QStringLiteral("BEM: <b>%1</b><br>"
                           "Cap: <b>%2</b><br>"
                           "Fiducials: <b>%3 / 3</b><br>"
                           "EEG electrodes: <b>%4</b><br>"
                           "Head-shape points: <b>%5</b>")
                .arg(m_bemPath.isEmpty() ? QStringLiteral("(none)") : QFileInfo(m_bemPath).fileName(),
                     systemLabel(m_capSystem))
                .arg(nFid)
                .arg(nEeg)
                .arg(nHsp));
    }
    if (m_pIcpStatusLabel) {
        if (m_haveIcpResult) {
            m_pIcpStatusLabel->setText(
                QStringLiteral("Coregistration: <b>%1</b> — RMSE %2 mm")
                    .arg(m_lastIcpSource)
                    .arg(m_lastIcpRmse * 1000.0f, 0, 'f', 2));
        } else {
            m_pIcpStatusLabel->setText(QStringLiteral(
                "<i>Coregistration: fiducial fit only (no ICP refinement yet).</i>"));
        }
    }
    if (m_pSaveTransBtn) {
        m_pSaveTransBtn->setEnabled(m_pPoints->hasAllTwinFiducials()
                                    && m_pPoints->hasFiducial(FiducialId::NAS)
                                    && m_pPoints->hasFiducial(FiducialId::LPA)
                                    && m_pPoints->hasFiducial(FiducialId::RPA));
    }
    if (m_pSaveDigiBtn) {
        m_pSaveDigiBtn->setEnabled(m_pPoints->countOf(PointKind::Fiducial) > 0
                                   || m_pPoints->countOf(PointKind::Eeg) > 0
                                   || m_pPoints->countOf(PointKind::HeadShape) > 0);
    }
}

//=============================================================================================================
// Done page
//=============================================================================================================

QWidget* AlignWizard::buildDonePage()
{
    auto* page = new QWidget(this);
    auto* lay  = new QVBoxLayout(page);

    lay->addWidget(new QLabel(QStringLiteral(
        "<h2>Step 7 — Done</h2>"
        "The coregistration session is complete. The summary below "
        "lists the artefacts produced. You can return to any earlier "
        "step with the Back button to refine the fit."), page));

    m_pDoneLabel = new QLabel(page);
    m_pDoneLabel->setTextFormat(Qt::RichText);
    m_pDoneLabel->setWordWrap(true);
    lay->addWidget(m_pDoneLabel);

    lay->addStretch(1);
    return page;
}

void AlignWizard::refreshDoneUi()
{
    if (!m_pDoneLabel) return;
    const int nFid = m_pPoints->countOf(PointKind::Fiducial);
    const int nEeg = m_pPoints->countOf(PointKind::Eeg);
    const int nHsp = m_pPoints->countOf(PointKind::HeadShape);

    QString html = QStringLiteral(
        "<b>BEM:</b> %1<br>"
        "<b>Cap:</b> %2<br>"
        "<b>Captured:</b> %3 fiducials, %4 EEG, %5 HSP<br>")
        .arg(m_bemPath.isEmpty() ? QStringLiteral("(none)") : QFileInfo(m_bemPath).fileName(),
             systemLabel(m_capSystem))
        .arg(nFid).arg(nEeg).arg(nHsp);

    if (m_haveIcpResult) {
        html += QStringLiteral("<b>ICP RMSE:</b> %1 mm (%2)<br>")
                    .arg(m_lastIcpRmse * 1000.0f, 0, 'f', 2)
                    .arg(m_lastIcpSource);
    } else {
        html += QStringLiteral("<b>ICP:</b> <i>not performed</i><br>");
    }

    if (!m_lastSavedTransPath.isEmpty()) {
        html += QStringLiteral("<b>Saved -trans.fif:</b> %1")
                    .arg(QFileInfo(m_lastSavedTransPath).fileName());
    } else {
        html += QStringLiteral("<i>-trans.fif not yet saved.</i>");
    }
    m_pDoneLabel->setText(html);
}

//=============================================================================================================
// Save handlers
//=============================================================================================================

void AlignWizard::onSaveTransClicked()
{
    const QString suggested = m_bemPath.isEmpty()
        ? QStringLiteral("subject-trans.fif")
        : QFileInfo(m_bemPath).completeBaseName() + QStringLiteral("-trans.fif");
    const QString out = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save head→MRI transform"),
        suggested, QStringLiteral("FIFF transforms (*.fif)"));
    if (out.isEmpty()) return;
    emit requestSaveTrans(out);
}

void AlignWizard::onSaveDigitisationClicked()
{
    const QString out = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save digitisation"),
        QStringLiteral("digitisation.fif"),
        QStringLiteral("FIFF (*.fif)"));
    if (out.isEmpty()) return;
    emit requestSaveDigitisation(out);
}

//=============================================================================================================
// FIFF digitisation load
//=============================================================================================================

void AlignWizard::onLoadDigiFiff()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Load digitisation FIFF"),
        QString(), QStringLiteral("FIFF (*.fif *.fif.gz)"));
    if (path.isEmpty()) return;

    QFile file(path);
    FIFFLIB::FiffDigPointSet set;
    try {
        set = FIFFLIB::FiffDigPointSet(file);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, QStringLiteral("Load digitisation"),
            QStringLiteral("Failed to read %1:\n%2").arg(path, QString::fromUtf8(e.what())));
        return;
    }
    if (set.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Load digitisation"),
            QStringLiteral("No digitisation points found in:\n%1").arg(path));
        return;
    }

    // Replace current capture with the imported set.
    m_pPoints->clear();
    int hspIdent = 1;
    int eegIdent = 1;
    for (qint32 i = 0; i < set.size(); ++i) {
        const FIFFLIB::FiffDigPoint& dp = set[i];
        DigitizedPoint p;
        p.position = QVector3D(dp.r[0], dp.r[1], dp.r[2]);
        switch (dp.kind) {
            case FIFFV_POINT_CARDINAL: {
                p.kind = PointKind::Fiducial;
                // FIFF cardinal idents: 1=LPA, 2=Nasion, 3=RPA (mne-python convention).
                FiducialId id = FiducialId::NAS;
                switch (dp.ident) {
                    case 1: id = FiducialId::LPA; break;
                    case 2: id = FiducialId::NAS; break;
                    case 3: id = FiducialId::RPA; break;
                    default: continue;
                }
                p.identNumber = static_cast<int>(id);
                p.label       = (id == FiducialId::LPA) ? QStringLiteral("LPA")
                              : (id == FiducialId::NAS) ? QStringLiteral("NAS")
                              :                           QStringLiteral("RPA");
                break;
            }
            case FIFFV_POINT_EEG:
                p.kind        = PointKind::Eeg;
                p.identNumber = eegIdent++;
                p.label       = QStringLiteral("EEG-%1").arg(p.identNumber);
                break;
            case FIFFV_POINT_EXTRA:
                p.kind        = PointKind::HeadShape;
                p.identNumber = hspIdent++;
                p.label       = QStringLiteral("HSP-%1").arg(p.identNumber);
                break;
            default:
                // Skip HPI / ECG / unknown for now.
                continue;
        }
        m_pPoints->append(p);
    }

    emit digitisationLoaded(path);
}

//=============================================================================================================
// ICP result reporting
//=============================================================================================================

void AlignWizard::setIcpResult(const QMatrix4x4& headToMri,
                               float rmseMeters,
                               const QVector<QPair<QString, float>>& residuals,
                               const QString& sourceLabel)
{
    m_haveIcpResult     = true;
    m_lastIcpHeadToMri  = headToMri;
    m_lastIcpRmse       = rmseMeters;
    m_lastIcpResiduals  = residuals;
    m_lastIcpSource     = sourceLabel;
    refreshVerifyUi();
    refreshSaveUi();
    refreshDoneUi();
}

void AlignWizard::setLastSavedTrans(const QString& outPath)
{
    m_lastSavedTransPath = outPath;
    refreshDoneUi();
}
