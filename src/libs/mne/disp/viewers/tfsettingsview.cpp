//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     tfsettingsview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2019
 * @brief    Implementation of the TfSettingsView TF-analysis configuration panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tfsettingsview.h"
#include "settingspanel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TfSettingsView::TfSettingsView(const QString& sSettingsPath,
                               QWidget* parent,
                               Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pForm(std::make_shared<scene::SettingsForm>("Time frequency Settings"))
{
    m_sSettingsPath = sSettingsPath;

    m_pForm->add_int("trialNumber", "Trial number", 0, 0, 10000);
    m_pForm->add_int("rowNumber", "Row number", 0, 0, 10000);
    m_listener = m_pForm->add_listener([this](const std::string&, const std::optional<scene::SettingValue>&) {
        onNumberTrialRowChanged();
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new SettingsPanel(m_pForm, this));

    loadSettings();

    this->setWindowTitle("Time frequency Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}

//=============================================================================================================

TfSettingsView::~TfSettingsView()
{
    m_pForm->remove_listener(m_listener);
    saveSettings();
}

//=============================================================================================================

void TfSettingsView::saveSettings()
{
    // The trial and row number come from the plugin; nothing is persisted
}

//=============================================================================================================

void TfSettingsView::loadSettings()
{
    // The trial and row number come from the plugin; nothing is persisted
}

//=============================================================================================================

void TfSettingsView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void TfSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void TfSettingsView::onNumberTrialRowChanged()
{
    emit numberTrialRowChanged(m_pForm->get<int>("trialNumber"), m_pForm->get<int>("rowNumber"));
}

//=============================================================================================================

void TfSettingsView::clearView()
{
}
