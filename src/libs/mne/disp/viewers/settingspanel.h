//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     settingspanel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SettingsPanel builds Qt Widgets for any scene::SettingsForm.
 *
 * The one Qt builder of the toolkit-neutral settings description: check boxes, spin boxes, sliders, combo boxes,
 * line edits, file choosers and buttons in group boxes, kept in step with the form both ways, with the enabled and
 * visible conditions of the fields applied. Values persist in QSettings.
 */

#ifndef DISP_SETTINGSPANEL_H
#define DISP_SETTINGSPANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include <mne/scene/settings_form.h>

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
class QLabel;

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

/**
 * @brief Qt Widgets for a scene::SettingsForm.
 *
 * The panel shares the form with its host, so it outlives the host's members when Qt deletes it as a child. Fields are laid out in the order of the form, one group box per group (fields
 * without a group come first, without a box). Custom fields get an empty container the host fills with its own
 * widget (customWidget()).
 *
 * @since 3.0.0
 */
class DISPSHARED_EXPORT SettingsPanel : public QWidget
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
    explicit SettingsPanel(std::shared_ptr<scene::SettingsForm> form, QWidget* parent = nullptr);

    ~SettingsPanel() override;

    /** @return The form. */
    scene::SettingsForm& form() const
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
     * Reads the values stored under @p path; missing ones keep their current value.
     *
     * @param[in] path   QSettings group of the panel, e.g. "MNESCAN/Averaging"; empty does nothing.
     */
    void loadSettings(const QString& path);

    //=========================================================================================================
    /**
     * Stores the values under @p path, one QSettings key per field.
     *
     * @param[in] path   QSettings group of the panel; empty does nothing.
     */
    void saveSettings(const QString& path) const;

private:
    struct Row
    {
        QWidget* editor = nullptr; /**< Editor, button or container. */
        QLabel* label = nullptr;   /**< Label in front of the editor, if any. */
    };

    void buildField(const scene::SettingField& field, QFormLayout* layout);
    void showValue(const std::string& key);
    void updateField(const std::string& key);
    void updateConditions();

    std::shared_ptr<scene::SettingsForm> m_pForm;
    scene::SettingsForm& m_form;
    std::map<std::string, Row> m_rows;
    int m_listener = 0;
    int m_structureListener = 0;
};

} // namespace mne::disp

#endif // DISP_SETTINGSPANEL_H
