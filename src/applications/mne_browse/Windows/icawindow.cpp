//=============================================================================================================
/**
 * @file     icawindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     June, 2026
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
 * @brief    Definition of the IcaWindow class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "icawindow.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================

IcaWindow::IcaWindow(QWidget *parent)
    : QDockWidget(tr("ICA Browser"), parent)
{
    setupUi();
}

//=============================================================================================================

void IcaWindow::init()
{
    // Nothing else needed at start
}

//=============================================================================================================

void IcaWindow::setupUi()
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setMinimumWidth(320);

    m_pCentralWidget = new QWidget(this);
    m_pMainLayout = new QVBoxLayout(m_pCentralWidget);

    // ── Controls group ──────────────────────────────────────────────
    auto *controlGroup = new QGroupBox(tr("ICA Settings"), m_pCentralWidget);
    auto *controlLayout = new QHBoxLayout(controlGroup);

    auto *nCompLabel = new QLabel(tr("Components:"), controlGroup);
    m_pNComponentsSpin = new QSpinBox(controlGroup);
    m_pNComponentsSpin->setRange(2, 500);
    m_pNComponentsSpin->setValue(20);
    m_pNComponentsSpin->setToolTip(tr("Number of ICA components to extract"));

    m_pComputeButton = new QPushButton(tr("Compute"), controlGroup);
    m_pComputeButton->setToolTip(tr("Run FastICA on the loaded raw data"));
    m_pComputeButton->setEnabled(false);

    controlLayout->addWidget(nCompLabel);
    controlLayout->addWidget(m_pNComponentsSpin);
    controlLayout->addWidget(m_pComputeButton);
    controlLayout->addStretch();

    m_pMainLayout->addWidget(controlGroup);

    // ── Status label ────────────────────────────────────────────────
    m_pStatusLabel = new QLabel(tr("No raw data loaded."), m_pCentralWidget);
    m_pStatusLabel->setWordWrap(true);
    m_pMainLayout->addWidget(m_pStatusLabel);

    // ── Scrollable component list ───────────────────────────────────
    m_pScrollArea = new QScrollArea(m_pCentralWidget);
    m_pScrollArea->setWidgetResizable(true);
    m_pComponentListWidget = new QWidget();
    m_pComponentListLayout = new QVBoxLayout(m_pComponentListWidget);
    m_pComponentListLayout->setAlignment(Qt::AlignTop);
    m_pScrollArea->setWidget(m_pComponentListWidget);
    m_pMainLayout->addWidget(m_pScrollArea, 1);

    // ── Apply button ────────────────────────────────────────────────
    m_pApplyButton = new QPushButton(tr("Apply Exclusion"), m_pCentralWidget);
    m_pApplyButton->setToolTip(tr("Remove the checked components from the raw data"));
    m_pApplyButton->setEnabled(false);
    m_pMainLayout->addWidget(m_pApplyButton);

    setWidget(m_pCentralWidget);

    // Connections
    connect(m_pComputeButton, &QPushButton::clicked, this, &IcaWindow::onCompute);
    connect(m_pApplyButton,   &QPushButton::clicked, this, &IcaWindow::onApply);
}

//=============================================================================================================

void IcaWindow::setRawData(const MatrixXd &rawData,
                            QSharedPointer<FIFFLIB::FiffInfo> fiffInfo,
                            int firstSample)
{
    m_rawData     = rawData;
    m_pFiffInfo   = fiffInfo;
    m_firstSample = firstSample;
    m_bHasResult  = false;

    m_pComputeButton->setEnabled(rawData.rows() > 0 && rawData.cols() > 0);

    const int nCh  = static_cast<int>(rawData.rows());
    const int nSmp = static_cast<int>(rawData.cols());
    m_pNComponentsSpin->setMaximum(qMin(nCh, 200));
    m_pNComponentsSpin->setValue(qMin(m_pNComponentsSpin->value(), nCh));

    m_pStatusLabel->setText(tr("Raw data loaded: %1 channels × %2 samples.\nPress Compute to run FastICA.")
                                .arg(nCh).arg(nSmp));
    clearIca();
}

//=============================================================================================================

void IcaWindow::clearIca()
{
    m_bHasResult = false;
    m_icaResult  = IcaResult();
    m_pApplyButton->setEnabled(false);

    // Clear component list
    for (auto *cb : m_componentCheckboxes)
        cb->deleteLater();
    m_componentCheckboxes.clear();

    // Remove all items from the component list layout
    while (QLayoutItem *item = m_pComponentListLayout->takeAt(0)) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }
}

//=============================================================================================================

void IcaWindow::onCompute()
{
    if (m_rawData.rows() == 0 || m_rawData.cols() == 0) {
        QMessageBox::warning(this, tr("ICA"),
                             tr("No raw data available."));
        return;
    }

    const int nComp = m_pNComponentsSpin->value();

    m_pStatusLabel->setText(tr("Computing %1 ICA components...").arg(nComp));
    m_pComputeButton->setEnabled(false);
    QApplication::setOverrideCursor(Qt::BusyCursor);
    QApplication::processEvents();

    m_icaResult = ICA::run(m_rawData, nComp);
    m_bHasResult = true;

    QApplication::restoreOverrideCursor();
    m_pComputeButton->setEnabled(true);

    const QString convergenceMsg = m_icaResult.bConverged
        ? tr("All components converged.")
        : tr("Warning: some components did NOT converge.");

    m_pStatusLabel->setText(tr("ICA decomposition complete (%1 components).\n%2\n"
                               "Check components to exclude, then press Apply.")
                                .arg(nComp).arg(convergenceMsg));

    rebuildComponentList();
    m_pApplyButton->setEnabled(true);
}

//=============================================================================================================

void IcaWindow::onApply()
{
    if (!m_bHasResult || m_rawData.rows() == 0)
        return;

    QVector<int> excluded;
    for (int i = 0; i < m_componentCheckboxes.size(); ++i) {
        if (m_componentCheckboxes[i]->isChecked())
            excluded.append(i);
    }

    if (excluded.isEmpty()) {
        QMessageBox::information(this, tr("ICA"),
                                 tr("No components selected for exclusion."));
        return;
    }

    QApplication::setOverrideCursor(Qt::BusyCursor);
    MatrixXd cleaned = ICA::excludeComponents(m_rawData, m_icaResult, excluded);
    QApplication::restoreOverrideCursor();

    m_pStatusLabel->setText(tr("Applied: excluded %1 component(s). Data cleaned.")
                                .arg(excluded.size()));

    emit icaCleaned(cleaned);
}

//=============================================================================================================

void IcaWindow::rebuildComponentList()
{
    clearIca();
    m_bHasResult = true; // restore after clearIca resets it
    m_pApplyButton->setEnabled(true);

    const int nComp = static_cast<int>(m_icaResult.matSources.rows());
    const int waveWidth  = 260;
    const int waveHeight = 40;

    for (int i = 0; i < nComp; ++i) {
        auto *rowWidget = new QWidget(m_pComponentListWidget);
        auto *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(2, 2, 2, 2);

        auto *cb = new QCheckBox(tr("IC %1").arg(i), rowWidget);
        cb->setToolTip(tr("Check to exclude this component"));
        m_componentCheckboxes.append(cb);

        QImage waveImg = renderComponentWaveform(i, waveWidth, waveHeight);
        auto *waveLabel = new QLabel(rowWidget);
        waveLabel->setPixmap(QPixmap::fromImage(waveImg));
        waveLabel->setFixedSize(waveWidth, waveHeight);

        rowLayout->addWidget(cb);
        rowLayout->addWidget(waveLabel, 1);

        m_pComponentListLayout->addWidget(rowWidget);
    }
}

//=============================================================================================================

QImage IcaWindow::renderComponentWaveform(int compIdx, int width, int height) const
{
    QImage img(width, height, QImage::Format_ARGB32_Premultiplied);
    img.fill(qRgba(245, 245, 245, 255));

    if (!m_bHasResult || compIdx < 0 || compIdx >= m_icaResult.matSources.rows())
        return img;

    const int nSamples = static_cast<int>(m_icaResult.matSources.cols());
    if (nSamples < 2)
        return img;

    // Compute min/max for normalisation
    const auto &row = m_icaResult.matSources.row(compIdx);
    double minVal = row.minCoeff();
    double maxVal = row.maxCoeff();
    double range  = maxVal - minVal;
    if (range < 1e-30)
        range = 1.0;

    // Min/max decimation: for each pixel column, find min and max sample values
    const float spp = static_cast<float>(nSamples) / width;
    const int margin = 2;
    const int plotH  = height - 2 * margin;

    for (int px = 0; px < width; ++px) {
        int sB = static_cast<int>(px       * spp);
        int sE = static_cast<int>((px + 1) * spp);
        sE = qMin(sE, nSamples);
        if (sB >= sE) sB = qMax(sE - 1, 0);

        double lo = row(sB);
        double hi = row(sB);
        for (int s = sB + 1; s < sE; ++s) {
            double v = row(s);
            if (v < lo) lo = v;
            if (v > hi) hi = v;
        }

        int yLo = margin + static_cast<int>((1.0 - (lo - minVal) / range) * plotH);
        int yHi = margin + static_cast<int>((1.0 - (hi - minVal) / range) * plotH);
        yLo = qBound(0, yLo, height - 1);
        yHi = qBound(0, yHi, height - 1);
        if (yHi > yLo) qSwap(yHi, yLo); // yHi should be smaller (higher on screen)

        for (int y = yHi; y <= yLo; ++y)
            img.setPixel(px, y, qRgb(30, 90, 170));
    }

    return img;
}
