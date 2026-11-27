//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     connectivitysettingsview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           johaenns <j.vorw01@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the ConnectivitySettingsView connectivity-metric panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivitysettingsview.h"
#include "../detail/scoped_flag.h"
#include "../kit/settingspanel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <initializer_list>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE STATIC HELPERS
//=============================================================================================================

namespace
{

/** Choices whose value is their label. */
std::vector<SettingChoice> plainChoices(std::initializer_list<const char*> values)
{
    std::vector<SettingChoice> choices;
    for (const char* value : values) {
        choices.push_back({value, value});
    }
    return choices;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectivitySettingsView::ConnectivitySettingsView(const QString& sSettingsPath,
                                                   QWidget* parent,
                                                   Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pForm(std::make_shared<viz::SettingsForm>("Connectivity Settings"))
, m_iNumberTrials(10)
{
    m_sSettingsPath = sSettingsPath;

    // The keys are the QSettings keys of the fields
    m_pForm->add_choice("connMethod", "Metric:", plainChoices({"COR", "COH", "IMAGCOH", "XCOR", "PLI", "DSWPLI", "USPLI", "WPLI", "PLV"}), "COR");
    m_pForm->add_choice("connWindowType", "Window Type:", plainChoices({"Hanning", "Ones"}), "Hanning");

    viz::SettingField trials;
    trials.key = "connNrTrials";
    trials.kind = viz::SettingField::Kind::Int;
    trials.label = "Number of trials:";
    trials.default_value = 10;
    trials.minimum = 1;
    trials.maximum = 1000;
    trials.decimals = 0;
    trials.commit = viz::SettingField::Commit::WhenFinished;
    m_pForm->add(trials);

    m_pForm->add_choice("connTriggerType", "Trigger type:", {}, {});
    m_pForm->add_double("connFreqLow", "Lower frequency:", 7.0, 0.0, 10000.0, 1.0, 1);
    m_pForm->add_double("connFreqHigh", "Upper frequency:", 13.0, 0.0, 10000.0, 1.0, 1);

    m_listener = m_pForm->add_listener([this](const std::string& key, const std::optional<viz::SettingValue>&) {
        if (m_bSilent) {
            return;
        }
        if (key == "connMethod") {
            onMetricChanged(getConnectivityMetric());
        } else if (key == "connWindowType") {
            onWindowTypeChanged(getWindowType());
        } else if (key == "connNrTrials") {
            onNumberTrialsChanged();
        } else if (key == "connTriggerType") {
            onTriggerTypeChanged(getTriggerType());
        } else if (key == "connFreqLow" || key == "connFreqHigh") {
            onFrequencyBandChanged();
        }
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_pPanel = new SettingsPanel(m_pForm, this);
    layout->addWidget(m_pPanel);

    {
        const detail::ScopedFlag silent(m_bSilent);
        loadSettings();
    }

    this->setWindowTitle("Connectivity Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}

//=============================================================================================================

ConnectivitySettingsView::~ConnectivitySettingsView()
{
    m_pForm->remove_listener(m_listener);
    saveSettings();
}

//=============================================================================================================

void ConnectivitySettingsView::setTriggerTypes(const QStringList& lTriggerTypes)
{
    std::vector<viz::SettingChoice> choices = m_pForm->field("connTriggerType")->choices;
    const std::size_t count = choices.size();
    for (const QString& sTriggerType : lTriggerTypes) {
        const std::string type = sTriggerType.toStdString();
        if (std::ranges::none_of(choices, [&type](const viz::SettingChoice& choice) { return choice.value == type; })) {
            choices.push_back({type, type});
        }
    }
    if (choices.size() != count) {
        m_pForm->set_choices("connTriggerType", std::move(choices));
    }
}

//=============================================================================================================

void ConnectivitySettingsView::setNumberTrials(int iNumberTrials)
{
    // Like setting the spin box: only finished user edits report the number
    const detail::ScopedFlag silent(m_bSilent);
    m_pForm->set_value("connNrTrials", iNumberTrials);
}

//=============================================================================================================

void ConnectivitySettingsView::setFrequencyBand(double dFreqLow, double dFreqHigh)
{
    m_pForm->set_value("connFreqLow", dFreqLow);
    m_pForm->set_value("connFreqHigh", dFreqHigh);
}

//=============================================================================================================

QString ConnectivitySettingsView::getConnectivityMetric()
{
    return QString::fromStdString(m_pForm->get<std::string>("connMethod"));
}

//=============================================================================================================

QString ConnectivitySettingsView::getWindowType()
{
    return QString::fromStdString(m_pForm->get<std::string>("connWindowType"));
}

//=============================================================================================================

int ConnectivitySettingsView::getNumberTrials()
{
    return m_pForm->get<int>("connNrTrials");
}

//=============================================================================================================

QString ConnectivitySettingsView::getTriggerType()
{
    return QString::fromStdString(m_pForm->get<std::string>("connTriggerType"));
}

//=============================================================================================================

double ConnectivitySettingsView::getLowerFreq()
{
    return m_pForm->get<double>("connFreqLow");
}

//=============================================================================================================

double ConnectivitySettingsView::getUpperFreq()
{
    return m_pForm->get<double>("connFreqHigh");
}

//=============================================================================================================

void ConnectivitySettingsView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    m_pPanel->saveSettings(m_sSettingsPath + QString("/ConnectivitySettingsView"));
}

//=============================================================================================================

void ConnectivitySettingsView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    m_pPanel->loadSettings(m_sSettingsPath + QString("/ConnectivitySettingsView"));
    m_iNumberTrials = getNumberTrials();
}

//=============================================================================================================

void ConnectivitySettingsView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ConnectivitySettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void ConnectivitySettingsView::onMetricChanged(const QString& sMetric)
{
    emit connectivityMetricChanged(sMetric);
    saveSettings();
}

//=============================================================================================================

void ConnectivitySettingsView::onWindowTypeChanged(const QString& sWindowType)
{
    emit windowTypeChanged(sWindowType);
    saveSettings();
}

//=============================================================================================================

void ConnectivitySettingsView::onNumberTrialsChanged()
{
    if (m_iNumberTrials == getNumberTrials()) {
        return;
    }

    m_iNumberTrials = getNumberTrials();

    emit numberTrialsChanged(m_iNumberTrials);
    saveSettings();
}

//=============================================================================================================

void ConnectivitySettingsView::onTriggerTypeChanged(const QString& sTriggerType)
{
    emit triggerTypeChanged(sTriggerType);
    saveSettings();
}

//=============================================================================================================

void ConnectivitySettingsView::onFrequencyBandChanged()
{
    emit freqBandChanged(static_cast<float>(getLowerFreq()),
                         static_cast<float>(getUpperFreq()));
    saveSettings();
}

//=============================================================================================================

void ConnectivitySettingsView::clearView()
{
}
