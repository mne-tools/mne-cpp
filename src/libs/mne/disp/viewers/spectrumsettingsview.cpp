//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     spectrumsettingsview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     May 2014
 * @brief    Implementation of the SpectrumSettingsView frequency-bound panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectrumsettingsview.h"
#include "settingspanel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SpectrumSettingsView::SpectrumSettingsView(const QString& sSettingsPath,
                                           QWidget* parent,
                                           Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pForm(std::make_shared<scene::SettingsForm>("Spectrum Settings"))
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Spectrum Settings");

    // Sliders in steps of 1 mHz; setBoundaries() sets the range to the Nyquist frequency
    for (const auto& [key, label] : {std::pair{"lower", "Lower Frequency"}, std::pair{"upper", "Upper Frequency"}}) {
        scene::SettingField bound;
        bound.key = key;
        bound.kind = scene::SettingField::Kind::Double;
        bound.label = label;
        bound.default_value = 0.0;
        bound.maximum = 0.1;
        bound.step = 0.001;
        bound.decimals = 3;
        bound.unit = " Hz";
        bound.style = scene::SettingField::Style::Slider;
        m_pForm->add(bound);
    }
    m_listener = m_pForm->add_listener([this](const std::string&, const std::optional<scene::SettingValue>&) {
        updateValue(0);
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new SettingsPanel(m_pForm, this));

    loadSettings();
}

//=============================================================================================================

SpectrumSettingsView::~SpectrumSettingsView()
{
    m_pForm->remove_listener(m_listener);
    saveSettings();
}

//=============================================================================================================

void SpectrumSettingsView::updateValue(qint32 value)
{
    Q_UNUSED(value)

    // The lower bound never exceeds the upper one
    const double lower = m_pForm->get<double>("lower");
    const double upper = m_pForm->get<double>("upper");
    if (lower > upper) {
        m_pForm->set_value("lower", upper);
        return;
    }

    emit settingsChanged();
}

//=============================================================================================================

void SpectrumSettingsView::saveSettings()
{
    // The bounds come from the plugin; nothing is persisted
}

//=============================================================================================================

void SpectrumSettingsView::loadSettings()
{
    // The bounds come from the plugin; nothing is persisted
}

//=============================================================================================================

void SpectrumSettingsView::setBoundaries(float fSFreq,
                                         float fLowerBound,
                                         float fUpperBound)
{
    const double nyquist = static_cast<int>(fSFreq / 2);
    m_pForm->set_range("lower", 0.0, nyquist);
    m_pForm->set_range("upper", 0.0, nyquist);
    m_pForm->set_value("upper", static_cast<double>(fUpperBound));
    m_pForm->set_value("lower", static_cast<double>(fLowerBound));
}

//=============================================================================================================

float SpectrumSettingsView::getLowerBound()
{
    return static_cast<float>(m_pForm->get<double>("lower"));
}

//=============================================================================================================

float SpectrumSettingsView::getUpperBound()
{
    return static_cast<float>(m_pForm->get<double>("upper"));
}

//=============================================================================================================

void SpectrumSettingsView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void SpectrumSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void SpectrumSettingsView::clearView()
{
}
