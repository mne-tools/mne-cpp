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
 * @brief    Implementation of the five-step MNE Align capture wizard.
 */

#include "align_wizard.h"

#include "polhemus_connection.h"

#include <utils/montage/standard_montage.h>

#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
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
constexpr int kExportIdx    = 4;

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
    addWidget(buildExportPage());

    if (m_pDigitizer) {
        connect(m_pDigitizer, &PolhemusConnection::pointReceived,
                this, &AlignWizard::onLivePoint);
    }
    connect(m_pPoints, &AcquiredPoints::pointsChanged,
            this, &AlignWizard::onAcquiredChanged);

    onAcquiredChanged();
    connect(this, &QStackedWidget::currentChanged, this,
            [this](int) { emit stepChanged(currentStep()); });
}

AlignWizard::~AlignWizard() = default;

//=============================================================================================================

int     AlignWizard::stepCount() { return 5; }

QString AlignWizard::titleFor(AlignStep step)
{
    switch (step) {
        case AlignStep::Setup:     return QStringLiteral("Setup");
        case AlignStep::Fiducials: return QStringLiteral("Fiducials");
        case AlignStep::EegCap:    return QStringLiteral("EEG Cap");
        case AlignStep::HeadShape: return QStringLiteral("Head Shape");
        case AlignStep::Export:    return QStringLiteral("Export");
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
    refreshExportUi();
}

void AlignWizard::next() { if (currentIndex() < count() - 1) setCurrentIndex(currentIndex() + 1); }
void AlignWizard::back() { if (currentIndex() > 0)           setCurrentIndex(currentIndex() - 1); }

//=============================================================================================================
// Live cursor
//=============================================================================================================

void AlignWizard::onLivePoint(int /*station*/, const QVector3D& pos, const QQuaternion& /*ori*/)
{
    m_lastLivePos = pos;
    m_haveLive = true;

    const auto liveStr = QStringLiteral("live: (%1, %2, %3) m")
                             .arg(pos.x(), 0, 'f', 4)
                             .arg(pos.y(), 0, 'f', 4)
                             .arg(pos.z(), 0, 'f', 4);
    if (m_pFidLiveLabel) m_pFidLiveLabel->setText(liveStr);
    if (m_pEegLiveLabel) m_pEegLiveLabel->setText(liveStr);
    if (m_pHspLiveLabel) m_pHspLiveLabel->setText(liveStr);
}

void AlignWizard::onAcquiredChanged()
{
    refreshFiducialUi();
    refreshEegUi();
    refreshHeadShapeUi();
    refreshExportUi();
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

//=============================================================================================================
// Fiducials page
//=============================================================================================================

QWidget* AlignWizard::buildFiducialsPage()
{
    auto* page = new QWidget(this);
    auto* lay  = new QVBoxLayout(page);

    lay->addWidget(new QLabel(QStringLiteral("<h2>Step 2 — Fiducials</h2>"
                                              "Capture <b>NAS</b>, <b>LPA</b>, <b>RPA</b> in turn. "
                                              "Hold the stylus on the landmark and press "
                                              "<b>Capture</b>."), page));

    m_pFidStatusLabel = new QLabel(page);
    m_pFidStatusLabel->setTextFormat(Qt::RichText);
    lay->addWidget(m_pFidStatusLabel);

    m_pFidLiveLabel = new QLabel(QStringLiteral("live: —"), page);
    lay->addWidget(m_pFidLiveLabel);

    auto* btnRow = new QHBoxLayout;
    m_pFidCaptureBtn = new QPushButton(QStringLiteral("Capture"), page);
    m_pFidUndoBtn    = new QPushButton(QStringLiteral("Undo last"), page);
    btnRow->addWidget(m_pFidCaptureBtn);
    btnRow->addWidget(m_pFidUndoBtn);
    btnRow->addStretch(1);
    lay->addLayout(btnRow);

    lay->addStretch(1);

    connect(m_pFidCaptureBtn, &QPushButton::clicked, this, &AlignWizard::onCaptureFiducial);
    connect(m_pFidUndoBtn,    &QPushButton::clicked, this, &AlignWizard::onUndoFiducial);
    return page;
}

FiducialId AlignWizard::nextFiducial(bool* allDone) const
{
    static constexpr FiducialId kOrder[] = {FiducialId::NAS, FiducialId::LPA, FiducialId::RPA};
    for (FiducialId id : kOrder) {
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

    QString html = QStringLiteral("<table>");
    static constexpr FiducialId kOrder[] = {FiducialId::NAS, FiducialId::LPA, FiducialId::RPA};
    for (FiducialId id : kOrder) {
        const bool have = m_pPoints->hasFiducial(id);
        const QString mark = have ? QStringLiteral("✔") : QStringLiteral("○");
        html += QStringLiteral("<tr><td>%1</td><td><b>%2</b></td></tr>")
                    .arg(mark, fiducialName(id));
    }
    html += QStringLiteral("</table>");

    bool allDone = false;
    const FiducialId next = nextFiducial(&allDone);
    if (allDone) {
        html += QStringLiteral("<p>All fiducials captured. Continue with EEG.</p>");
    } else {
        html += QStringLiteral("<p>Next: <b>%1</b></p>").arg(fiducialName(next));
    }
    m_pFidStatusLabel->setText(html);

    if (m_pFidCaptureBtn) {
        m_pFidCaptureBtn->setEnabled(!allDone && m_haveLive);
        m_pFidCaptureBtn->setText(allDone
            ? QStringLiteral("Capture")
            : QStringLiteral("Capture %1").arg(fiducialName(next)));
    }
    if (m_pFidUndoBtn) {
        m_pFidUndoBtn->setEnabled(m_pPoints->countOf(PointKind::Fiducial) > 0);
    }
}

void AlignWizard::onCaptureFiducial()
{
    if (!m_haveLive) return;
    bool allDone = false;
    const FiducialId next = nextFiducial(&allDone);
    if (allDone) return;

    DigitizedPoint p;
    p.kind        = PointKind::Fiducial;
    p.label       = fiducialName(next);
    p.identNumber = static_cast<int>(next);
    p.position    = m_lastLivePos;
    m_pPoints->append(p);
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
                                              "The list shows the configured montage; "
                                              "captured electrodes are marked with ✔."), page));

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
                                              "Sweep the stylus over the scalp and press "
                                              "<b>Capture</b> to add additional head-shape points. "
                                              "These are written as FIFF <code>EXTRA</code> points."), page));

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
// Export page
//=============================================================================================================

QWidget* AlignWizard::buildExportPage()
{
    auto* page = new QWidget(this);
    auto* lay  = new QVBoxLayout(page);

    lay->addWidget(new QLabel(QStringLiteral("<h2>Step 5 — Export</h2>"
                                              "Save the captured digitisation as a FIFF "
                                              "<code>FIFFB_ISOTRAK</code> point set."), page));

    m_pExportSummaryLabel = new QLabel(page);
    m_pExportSummaryLabel->setTextFormat(Qt::RichText);
    lay->addWidget(m_pExportSummaryLabel);

    auto* btn = new QPushButton(QStringLiteral("Save digitization…"), page);
    lay->addWidget(btn);
    lay->addStretch(1);

    connect(btn, &QPushButton::clicked, this, &AlignWizard::onExportClicked);
    return page;
}

void AlignWizard::refreshExportUi()
{
    if (!m_pExportSummaryLabel) return;
    const int nFid = m_pPoints->countOf(PointKind::Fiducial);
    const int nEeg = m_pPoints->countOf(PointKind::Eeg);
    const int nHsp = m_pPoints->countOf(PointKind::HeadShape);
    m_pExportSummaryLabel->setText(
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

void AlignWizard::onExportClicked()
{
    const QString out = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save digitization"),
        QStringLiteral("digitization.fif"),
        QStringLiteral("FIFF (*.fif)"));
    if (out.isEmpty()) return;
    emit requestExport(out);
}
