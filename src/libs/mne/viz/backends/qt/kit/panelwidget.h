//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     panelwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    PanelWidget shows any toolkit-free viz::Panel with Qt Widgets.
 */

#ifndef VIZ_QT_PANELWIDGET_H
#define VIZ_QT_PANELWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_qt_global.h"
#include "../viewers/abstractview.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::viz
{
class Panel;
struct Theme;
} // namespace mne::viz

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

class SceneViewport;
class SettingsPanel;

//=============================================================================================================
/**
 * @brief The Qt host of every viz::Panel: its form as a SettingsPanel, its figure in a SceneViewport, its title,
 * width, modes and settings.
 *
 * The widget shares the panel with its owner. It restores the panel from QSettings("MNECPP") under the settings
 * path when it is created and stores it when it is destroyed.
 *
 * @since 3.0.0
 */
class VIZQTSHARED_EXPORT PanelWidget : public AbstractView
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Shows a panel.
     *
     * @param[in] panel          The panel, shared with its owner.
     * @param[in] settingsPath   QSettings path of the host, e.g. "MNESCAN/NoiseReduction"; empty stores nothing.
     * @param[in] parent         Parent widget.
     */
    explicit PanelWidget(std::shared_ptr<viz::Panel> panel, const QString& settingsPath = {}, QWidget* parent = nullptr);

    ~PanelWidget() override;

    /** @return The panel. */
    viz::Panel& panel() const
    {
        return *m_panel;
    }

    /** @return The widgets of the panel's form. */
    SettingsPanel* formPanel() const
    {
        return m_formPanel;
    }

    //=========================================================================================================
    /**
     * @return The viewport of the panel's figure, or null for panels without a figure.
     *
     * @since 3.0.0
     */
    SceneViewport* figureViewport() const
    {
        return m_figureViewport;
    }

    //=========================================================================================================
    /**
     * Gives the panel a theme; PanelWidget starts with viz::Theme::light().
     *
     * @param[in] theme   Colours, fonts and metrics.
     *
     * @since 3.0.0
     */
    void setTheme(const viz::Theme& theme);

    void saveSettings() override;
    void loadSettings() override;
    void updateGuiMode(GuiMode mode) override;
    void updateProcessingMode(ProcessingMode mode) override;
    void clearView() override;

private:
    std::shared_ptr<viz::Panel> m_panel;
    SettingsPanel* m_formPanel = nullptr;
    SceneViewport* m_figureViewport = nullptr;
    int m_structureListener = 0;
};

} // namespace mne::viz::qt

#endif // VIZ_QT_PANELWIDGET_H
