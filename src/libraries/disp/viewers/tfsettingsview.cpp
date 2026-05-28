//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Andreas Griesshammer <ag@fieldlineinc.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file tfsettingsview.cpp
 * @since 2022
 * @date  March 2023
 * @brief Implementation of the TfSettingsView TF-analysis configuration panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tfsettingsview.h"

#include "ui_tfsettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TfSettingsView::TfSettingsView(const QString& sSettingsPath,
                               QWidget *parent,
                               Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(new Ui::TfSettingsViewWidget)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    loadSettings();

    connect(m_pUi->m_spinBox_trialNumber, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TfSettingsView::onNumberTrialRowChanged);

    connect(m_pUi->m_spinBox_rowNumber, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TfSettingsView::onNumberTrialRowChanged);

    this->setWindowTitle("Time frequency Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}

//=============================================================================================================

TfSettingsView::~TfSettingsView()
{
    saveSettings();

    delete m_pUi;
}

//=============================================================================================================

void TfSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
}

//=============================================================================================================

void TfSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");
}

//=============================================================================================================

void TfSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void TfSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void TfSettingsView::onNumberTrialRowChanged()
{
    emit numberTrialRowChanged(m_pUi->m_spinBox_trialNumber->value(), m_pUi->m_spinBox_rowNumber->value());
    saveSettings();
}

//=============================================================================================================

void TfSettingsView::clearView()
{

}
