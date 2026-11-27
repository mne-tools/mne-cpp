//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     filtersettingsview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the FilterSettingsView wrapper panel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filtersettingsview.h"
#include "../detail/scoped_flag.h"
#include "filterdesignview.h"
#include "../kit/settingspanel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterSettingsView::FilterSettingsView(const QString& sSettingsPath,
                                       QWidget* parent,
                                       Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pForm(std::make_shared<viz::SettingsForm>("Filter Settings"))
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Filter Settings");

    m_pForm->add_flag("clinical");
    m_pForm->add_bool("filterActivated", "Activate filter", false);
    m_pForm->add_choice("filterChannelType", "Channel type", {{"All", "All"}, {"MEG", "MEG"}, {"EEG", "EEG"}, {"ECG", "ECG"}, {"EOG", "EOG"}}, "All");
    for (const auto& [key, label] : {std::pair{"filterFrom", "From"}, std::pair{"filterTo", "To"}}) {
        viz::SettingField frequency;
        frequency.key = key;
        frequency.kind = viz::SettingField::Kind::Double;
        frequency.label = label;
        frequency.default_value = 0.0;
        frequency.maximum = 10000000.0;
        frequency.step = 0.1;
        frequency.decimals = 1;
        frequency.unit = "Hz";
        frequency.commit = viz::SettingField::Commit::WhenFinished;
        m_pForm->add(frequency);
    }
    viz::SettingField advanced;
    advanced.key = "showFilterOptions";
    advanced.kind = viz::SettingField::Kind::Action;
    advanced.label = "Advanced filter design";
    advanced.visible_if = "!clinical";
    m_pForm->add(advanced);

    m_listener = m_pForm->add_listener([this](const std::string& key, const std::optional<viz::SettingValue>&) {
        onFormChanged(key);
    });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_pPanel = new SettingsPanel(m_pForm, this);
    layout->addWidget(m_pPanel);

    loadSettings();

    //Create and connect design viewer
    m_pFilterView = std::make_shared<FilterDesignView>(m_sSettingsPath,
                                                       nullptr,
                                                       Qt::Dialog);

    // The design view's pass band is shown, not handed back to it
    connect(m_pFilterView.get(), &FilterDesignView::updateFilterFrom, this, [this](double dFrom) {
        const detail::ScopedFlag silent(m_bSilent);
        m_pForm->set_value("filterFrom", dFrom);
    });
    connect(m_pFilterView.get(), &FilterDesignView::updateFilterTo, this, [this](double dTo) {
        const detail::ScopedFlag silent(m_bSilent);
        m_pForm->set_value("filterTo", dTo);
    });

    connect(this, &FilterSettingsView::guiStyleChanged,
            m_pFilterView.get(), &FilterDesignView::guiStyleChanged);

    const detail::ScopedFlag silent(m_bSilent);
    m_pForm->set_value("filterFrom", m_pFilterView->getFrom());
    m_pForm->set_value("filterTo", m_pFilterView->getTo());
}

//=============================================================================================================

FilterSettingsView::~FilterSettingsView()
{
    m_pForm->remove_listener(m_listener);
    saveSettings();
}

//=============================================================================================================

std::shared_ptr<FilterDesignView> FilterSettingsView::getFilterView()
{
    return m_pFilterView;
}

//=============================================================================================================

bool FilterSettingsView::getFilterActive()
{
    return m_pForm->get<bool>("filterActivated");
}

//=============================================================================================================

void FilterSettingsView::setSamplingRate(double dSFreq)
{
    //Update min max of spin boxes to nyquist
    const double nyquistFrequency = dSFreq / 2;
    {
        const detail::ScopedFlag silent(m_bSilent);
        m_pForm->set_range("filterFrom", 0.0, nyquistFrequency);
        m_pForm->set_range("filterTo", 0.0, nyquistFrequency);
    }

    m_pFilterView->setSamplingRate(dSFreq);
}

//=============================================================================================================

void FilterSettingsView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    m_pPanel->saveSettings(m_sSettingsPath + QString("/FilterSettingsView"));
}

//=============================================================================================================

void FilterSettingsView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    const detail::ScopedFlag silent(m_bSilent);
    m_pPanel->loadSettings(m_sSettingsPath + QString("/FilterSettingsView"));
}

//=============================================================================================================

void FilterSettingsView::updateGuiMode(GuiMode mode)
{
    m_pForm->set_value("clinical", mode == GuiMode::Clinical);
}

//=============================================================================================================

void FilterSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void FilterSettingsView::onShowFilterView()
{
    if (m_pFilterView->isActiveWindow()) {
        m_pFilterView->hide();
    } else {
        m_pFilterView->activateWindow();
        m_pFilterView->show();
        m_pFilterView->updateFilterPlot();
    }
}

//=============================================================================================================

void FilterSettingsView::onFormChanged(const std::string& key)
{
    if (m_bSilent || !m_pFilterView) {
        return;
    }

    if (key == "showFilterOptions") {
        onShowFilterView();
        return;
    }
    if (key == "filterActivated") {
        emit filterActivationChanged(m_pForm->get<bool>(key));
    } else if (key == "filterFrom") {
        m_pFilterView->setFrom(m_pForm->get<double>(key));
    } else if (key == "filterTo") {
        const double to = m_pForm->get<double>(key);
        {
            // The lower edge stays below the upper one
            const detail::ScopedFlag silent(m_bSilent);
            m_pForm->set_range("filterFrom", 0.0, m_pForm->get<double>("filterFrom") >= 2 ? to - 1 : to);
        }
        m_pFilterView->setTo(to);
    } else if (key == "filterChannelType") {
        m_pFilterView->setChannelType(QString::fromStdString(m_pForm->get<std::string>(key)));
    } else {
        return;
    }

    saveSettings();
}

//=============================================================================================================

void FilterSettingsView::clearView()
{
}
