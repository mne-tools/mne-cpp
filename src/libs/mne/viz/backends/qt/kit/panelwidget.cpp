//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     panelwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    PanelWidget class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "panelwidget.h"
#include "settingspanel.h"
#include "../qtsettingsstore.h"
#include <mne/viz/panels/panel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>
#include <QVBoxLayout>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PanelWidget::PanelWidget(std::shared_ptr<Panel> panel, const QString& settingsPath, QWidget* parent)
: AbstractView(parent)
, m_panel(std::move(panel))
{
    m_sSettingsPath = settingsPath;
    setWindowTitle(QString::fromStdString(m_panel->title()));
    if (m_panel->width() > 0) {
        setFixedWidth(m_panel->width());
    }
    loadSettings();
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_formPanel = new SettingsPanel(m_panel->shared_form(), this);
    layout->addWidget(m_formPanel);
}

//=============================================================================================================

PanelWidget::~PanelWidget()
{
    saveSettings();
}

//=============================================================================================================

void PanelWidget::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }
    QSettings settings(QStringLiteral("MNECPP"));
    QtSettingsStore store(settings);
    m_panel->save(store, m_sSettingsPath.toStdString());
}

//=============================================================================================================

void PanelWidget::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }
    QSettings settings(QStringLiteral("MNECPP"));
    const QtSettingsStore store(settings);
    m_panel->load(store, m_sSettingsPath.toStdString());
}

//=============================================================================================================

void PanelWidget::updateGuiMode(GuiMode mode)
{
    m_panel->set_gui_mode(mode == GuiMode::Clinical ? Panel::GuiMode::Clinical : Panel::GuiMode::Research);
}

//=============================================================================================================

void PanelWidget::updateProcessingMode(ProcessingMode mode)
{
    m_panel->set_processing_mode(mode == ProcessingMode::Offline ? Panel::ProcessingMode::Offline : Panel::ProcessingMode::RealTime);
}

//=============================================================================================================

void PanelWidget::clearView()
{
}
