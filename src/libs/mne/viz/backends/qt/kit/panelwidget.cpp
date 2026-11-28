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
#include "itemlistview.h"
#include "qttheme.h"
#include "sceneviewport.h"
#include "settingspanel.h"
#include "../qtsettingsstore.h"
#include <mne/viz/figures/scene_figure.h>
#include <mne/viz/panels/panel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>
#include <QLayout>
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
    // The item lists fill the custom fields of their keys, again whenever the form rebuilds its widgets
    fillLists();
    connect(m_formPanel, &SettingsPanel::rebuilt, this, &PanelWidget::fillLists);
    if (m_panel->figure()) {
        // The figure takes the room; the form keeps its height and shows only with fields
        m_figureViewport = new SceneViewport(m_panel->figure(), this);
        layout->addWidget(m_figureViewport, 1);
        m_formPanel->setVisible(!m_panel->form().fields().empty());
        m_structureListener = m_panel->form().add_structure_listener([this](const std::string&) {
            m_formPanel->setVisible(!m_panel->form().fields().empty());
        });
    }
    // The look of viz::FormView, so a panel looks alike in every backend
    setTheme(viz::Theme::light());
}

//=============================================================================================================

PanelWidget::~PanelWidget()
{
    if (m_structureListener != 0) {
        m_panel->form().remove_listener(m_structureListener);
    }
    saveSettings();
}

//=============================================================================================================

void PanelWidget::setTheme(const viz::Theme& theme)
{
    applyTheme(this, theme);
    m_formPanel->setTheme(theme);
    if (m_figureViewport) {
        m_figureViewport->setTheme(theme);
    }
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

//=============================================================================================================

void PanelWidget::fillLists()
{
    for (const auto& [key, list] : m_panel->lists()) {
        QWidget* container = m_formPanel->customWidget(key);
        if (!container || !container->layout() || container->findChild<ItemListView*>()) {
            continue;
        }
        container->layout()->addWidget(new ItemListView(list, container));
    }
}
