//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     tfsettingsview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2019
 * @brief    Implementation of the TfSettingsView TF-analysis configuration panel.
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
