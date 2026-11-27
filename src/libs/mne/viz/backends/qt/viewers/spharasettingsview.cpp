//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     spharasettingsview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the SpharaSettingsView SPHARA configuration panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spharasettingsview.h"
#include "../detail/scoped_flag.h"
#include "../kit/settingspanel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>
#include <QVBoxLayout>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SpharaSettingsView::SpharaSettingsView(const QString& sSettingsPath,
                                       QWidget* parent,
                                       Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pForm(std::make_shared<viz::SettingsForm>("SPHARA Settings"))
{
    m_sSettingsPath = sSettingsPath;

    m_pForm->add_bool("activateSphara", "Activate SPHARA", false);
    m_pForm->add_choice("system", "System:", {{"BabyMEG", "BabyMEG"}, {"VectorView", "VectorView"}, {"EEG", "EEG"}}, "BabyMEG");
    m_pForm->add_flag("hasSecond", true);
    m_pForm->add_int("first", "Inner layer:", 270, 1, 270);

    viz::SettingField first = *m_pForm->field("first");
    first.commit = viz::SettingField::Commit::WhenFinished;
    m_pForm->add(first);

    viz::SettingField second = first;
    second.key = "second";
    second.label = "Outer layer:";
    second.default_value = 105;
    second.maximum = 105;
    second.visible_if = "hasSecond";
    m_pForm->add(second);

    m_listener = m_pForm->add_listener([this](const std::string& key, const std::optional<viz::SettingValue>&) {
        if (m_bSilent) {
            return;
        }
        if (key == "activateSphara") {
            onSpharaButtonClicked(m_pForm->get<bool>("activateSphara"));
        } else if (key == "system" || key == "first" || key == "second") {
            onSpharaOptionsChanged();
        }
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new SettingsPanel(m_pForm, this));

    this->setWindowTitle("SPHARA Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
    loadSettings();
}

//=============================================================================================================

SpharaSettingsView::~SpharaSettingsView()
{
    m_pForm->remove_listener(m_listener);
    saveSettings();
}

//=============================================================================================================

void SpharaSettingsView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void SpharaSettingsView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void SpharaSettingsView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void SpharaSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void SpharaSettingsView::onSpharaButtonClicked(bool state)
{
    emit spharaActivationChanged(state);
}

//=============================================================================================================

void SpharaSettingsView::onSpharaOptionsChanged()
{
    updateSystemFields();

    emit spharaOptionsChanged(QString::fromStdString(m_pForm->get<std::string>("system")),
                              m_pForm->get<int>("first"),
                              m_pForm->get<int>("second"));
}

//=============================================================================================================

void SpharaSettingsView::updateSystemFields()
{
    // Relabelling and clamping the fields must not report options of a half-switched system
    const detail::ScopedFlag silent(m_bSilent);
    const std::string system = m_pForm->get<std::string>("system");
    m_pForm->set_value("hasSecond", true);

    if (system == "VectorView") {
        m_pForm->set_label("first", "Mag");
        m_pForm->set_range("first", 1, 102);
        m_pForm->set_label("second", "Grad");
        m_pForm->set_range("second", 1, 102);
    }

    if (system == "BabyMEG") {
        m_pForm->set_label("first", "Inner layer");
        m_pForm->set_range("first", 1, 270);
        m_pForm->set_label("second", "Outer layer");
        m_pForm->set_range("second", 1, 105);
    }

    if (system == "EEG") {
        m_pForm->set_label("first", "EEG");
        m_pForm->set_range("first", 1, 256);
        m_pForm->set_value("hasSecond", false);
    }
}

//=============================================================================================================

void SpharaSettingsView::clearView()
{
}
