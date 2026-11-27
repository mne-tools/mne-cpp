//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     covariancesettingsview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.3
 * @date     June 2020
 * @brief    Implementation of the CovarianceSettingsView covariance-length panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariancesettingsview.h"
#include "../kit/settingspanel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CovarianceSettingsView::CovarianceSettingsView(const QString& sSettingsPath,
                                               QWidget* parent)
: AbstractView(parent)
, m_pForm(std::make_shared<viz::SettingsForm>("Covariance Settings"))
, m_sSettingsPath(sSettingsPath)
{
    this->setWindowTitle("Covariance Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    constexpr int minSamples = 600;
    m_pForm->add_int("samples", "Number of Samples", minSamples, minSamples, minSamples * 60, minSamples);
    m_listener = m_pForm->add_listener([this](const std::string& key, const std::optional<viz::SettingValue>&) {
        if (key == "samples") {
            emit samplesChanged(m_pForm->get<int>("samples"));
        }
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new SettingsPanel(m_pForm, this));

    loadSettings();
}

//=============================================================================================================

CovarianceSettingsView::~CovarianceSettingsView()
{
    m_pForm->remove_listener(m_listener);
    saveSettings();
}

//=============================================================================================================

void CovarianceSettingsView::setCurrentSamples(int iSamples)
{
    m_pForm->set_value("samples", iSamples);
}

//=============================================================================================================

void CovarianceSettingsView::setMinSamples(int iSamples)
{
    m_pForm->set_range("samples", iSamples, iSamples * 60);
}

//=============================================================================================================

void CovarianceSettingsView::saveSettings()
{
    // The number of samples comes from the plugin; nothing is persisted
}

//=============================================================================================================

void CovarianceSettingsView::loadSettings()
{
    // The number of samples comes from the plugin; nothing is persisted
}

//=============================================================================================================

void CovarianceSettingsView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void CovarianceSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void CovarianceSettingsView::clearView()
{
}
