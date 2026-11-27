//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     settingspanel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SettingsPanel builds Qt Widgets for any viz::SettingsForm.
 *
 * The one Qt builder of the toolkit-neutral settings description: check boxes, spin boxes, sliders, combo boxes,
 * line edits, file and directory choosers, colour buttons, read-only texts and buttons in group boxes, kept in step
 * with the form both ways, with the enabled and visible conditions of the fields applied. Values persist in QSettings.
 */

#ifndef DISP_SETTINGSPANEL_H
#define DISP_SETTINGSPANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_qt_global.h"
#include <mne/viz/forms/settings_form.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QWidget>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <memory>
#include <string>

class QFormLayout;
class QGroupBox;
class QLabel;

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

/**
 * @brief Qt Widgets for a viz::SettingsForm.
 *
 * The panel shares the form with its host, so it outlives the host's members when Qt deletes it as a child. Fields are laid out in the order of the form, one group box per group (fields
 * without a group come first, without a box); a group box hides while none of its fields shows, and hidden fields
 * get no widget. Custom fields get an empty container the host fills with its own widget (customWidget()). Adding or
 * removing fields rebuilds the widgets, emptying the containers of custom fields again; rebuilt() tells the host.
 *
 * @since 3.0.0
 */
class VIZQTSHARED_EXPORT SettingsPanel : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Builds the widgets of a form.
     *
     * @param[in] form     Form, shared with the host.
     * @param[in] parent   Parent widget.
     */
    explicit SettingsPanel(std::shared_ptr<viz::SettingsForm> form, QWidget* parent = nullptr);

    ~SettingsPanel() override;

    /** @return The form. */
    viz::SettingsForm& form() const
    {
        return *m_pForm;
    }

    //=========================================================================================================
    /**
     * @param[in] key   Field key.
     *
     * @return The editor of a field (or the button of an action, or the container of a custom field), or nullptr.
     */
    QWidget* editor(const std::string& key) const;

    //=========================================================================================================
    /**
     * @param[in] key   Key of a Custom field.
     *
     * @return The container to put the host's widget in, with an empty vertical layout, or nullptr.
     */
    QWidget* customWidget(const std::string& key) const;

    //=========================================================================================================
    /**
     * Reads the values stored under @p path; missing ones and fields that are not persistent keep their current value.
     *
     * @param[in] path   QSettings group of the panel, e.g. "MNESCAN/Averaging"; empty does nothing.
     */
    void loadSettings(const QString& path);

    //=========================================================================================================
    /**
     * Stores the values under @p path, one QSettings key per persistent field.
     *
     * @param[in] path   QSettings group of the panel; empty does nothing.
     */
    void saveSettings(const QString& path) const;

signals:
    //=========================================================================================================
    /**
     * Emitted after fields were added or removed and the widgets were built again.
     *
     * @since 3.0.0
     */
    void rebuilt();

private:
    struct Row
    {
        QWidget* editor = nullptr; /**< Editor, button or container. */
        QLabel* label = nullptr;   /**< Label in front of the editor, if any. */
    };

    void build();
    void rebuild();
    void buildField(const viz::SettingField& field, QFormLayout* layout);
    QWidget* buildChooser(const viz::SettingField& field);
    void showValue(const std::string& key);
    void updateField(const std::string& key);
    void updateConditions();

    std::shared_ptr<viz::SettingsForm> m_pForm;
    viz::SettingsForm& m_form;
    std::map<std::string, Row> m_rows;
    std::map<std::string, QGroupBox*> m_groups;
    QWidget* m_content = nullptr; /**< Holds the rows; replaced on rebuild(). */
    int m_listener = 0;
    int m_structureListener = 0;
};

} // namespace mne::viz::qt

#endif // DISP_SETTINGSPANEL_H
