//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     form_view.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FormView lays out, draws and edits any SettingsForm without a toolkit.
 *
 * FormView is a SurfaceClient, so every backend that hosts a surface (Qt, SDL, ...) shows any Panel through it
 * without a view written for the panel. It draws with a Canvas in the look of a Theme and edits the form with the
 * pointer, the wheel and the keyboard.
 */

#ifndef VIZ_FORM_VIEW_H
#define VIZ_FORM_VIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../figures/display_list.h"
#include "../figures/text_metrics.h"
#include "../figures/theme.h"
#include "../forms/settings_form.h"
#include "../host/surface.h"
#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class Canvas;
class Panel;

//=============================================================================================================
/**
 * @brief Shows and edits a SettingsForm in a Surface, drawn toolkit-free in the look of a Theme.
 *
 * The title of the form comes first, then each group as a card with one row per visible field: the label on the
 * left, the editor on the right. Bool fields are switches, numbers steppers with - and + buttons (the wheel steps
 * too), choices cycle with < and > buttons, text, path and directory fields are line edits, actions buttons, colours
 * swatches and info fields toned text. Disabled fields are greyed and ignore input. The view follows the form: it
 * lays out again when fields are added, removed or relabelled and repaints when values change. Content taller than
 * the surface scrolls with the wheel.
 *
 * The form must outlive the view.
 *
 * @snippet ex_scene_sdl/main.cpp form_view_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT FormView : public SurfaceClient
{
public:
    //=========================================================================================================
    /**
     * @brief The clickable parts of an editor.
     *
     * @since 3.0.0
     */
    enum class Part
    {
        Editor,    /**< The whole editor. */
        Decrement, /**< The - button of numbers, the < button of choices. */
        Increment  /**< The + button of numbers, the > button of choices. */
    };

    //=========================================================================================================
    /**
     * Shows a form.
     *
     * @param[in] form      The form; it outlives the view.
     * @param[in] theme     The look.
     *
     * @since 3.0.0
     */
    explicit FormView(SettingsForm& form, Theme theme = Theme::light());

    //=========================================================================================================
    /**
     * Shows the form of a panel.
     *
     * @param[in] panel     The panel; it outlives the view.
     * @param[in] theme     The look.
     *
     * @since 3.0.0
     */
    explicit FormView(Panel& panel, Theme theme = Theme::light());

    //=========================================================================================================
    /**
     * Destroys the view and stops listening to the form.
     *
     * @since 3.0.0
     */
    ~FormView() override;

    FormView(const FormView&) = delete;
    FormView& operator=(const FormView&) = delete;

    //=========================================================================================================
    /**
     * @return The form shown.
     *
     * @since 3.0.0
     */
    SettingsForm& form() const
    {
        return m_form;
    }

    //=========================================================================================================
    /**
     * @return The look.
     *
     * @since 3.0.0
     */
    const Theme& theme() const
    {
        return m_theme;
    }

    //=========================================================================================================
    /**
     * Changes the look.
     *
     * @param[in] theme     The new look.
     *
     * @since 3.0.0
     */
    void set_theme(Theme theme);

    //=========================================================================================================
    /**
     * Measures text with other metrics, e.g. those of the backend's fonts.
     *
     * @param[in] metrics   The metrics, outliving the view, or nullptr for ApproximateTextMetrics.
     *
     * @since 3.0.0
     */
    void set_text_metrics(const TextMetrics* metrics);

    //=========================================================================================================
    /**
     * Sets the size of the view; resize() does so for hosted views.
     *
     * @param[in] width     Width in logical units.
     * @param[in] height    Height in logical units.
     *
     * @since 3.0.0
     */
    void set_view_size(double width, double height);

    //=========================================================================================================
    /**
     * @return Height of the laid-out content in logical units.
     *
     * @since 3.0.0
     */
    double content_height() const;

    //=========================================================================================================
    /**
     * @return How far the content is scrolled up, in logical units.
     *
     * @since 3.0.0
     */
    double scroll_offset() const
    {
        return m_scroll;
    }

    //=========================================================================================================
    /**
     * Scrolls the content, clamped so it stays in view.
     *
     * @param[in] offset    How far the content is scrolled up, in logical units.
     *
     * @since 3.0.0
     */
    void set_scroll_offset(double offset);

    //=========================================================================================================
    /**
     * @param[in] key   Key of a field.
     * @param[in] part  Part of its editor.
     *
     * @return Where the part is shown in view coordinates (scrolling applied), or nothing if the field is not shown
     *         or its editor has no such part.
     *
     * @since 3.0.0
     */
    std::optional<Rect> editor_rect(const std::string& key, Part part = Part::Editor) const;

    //=========================================================================================================
    /**
     * @return Key of the text field being edited, or an empty string.
     *
     * @since 3.0.0
     */
    const std::string& focused_key() const
    {
        return m_focus;
    }

    //=========================================================================================================
    /**
     * @return The text of the field being edited, including uncommitted changes.
     *
     * @since 3.0.0
     */
    const std::string& edit_text() const
    {
        return m_editText;
    }

    //=========================================================================================================
    /**
     * Draws the view as it is now.
     *
     * @return The figure of the view's logical size.
     *
     * @since 3.0.0
     */
    DisplayList draw() const;

    void resize(const SurfaceSize& size) override;
    bool handle_pointer(const PointerEvent& event) override;
    bool handle_wheel(const WheelEvent& event) override;
    bool handle_key(const KeyEvent& event) override;
    GpuFrame frame(const GpuFrameTarget& target) override;
    const GpuResources& resources() const override;

private:
    struct Row
    {
        enum class Type
        {
            Title, /**< The title of the form. */
            Group, /**< The title of a group. */
            Card,  /**< The background of a group's rows. */
            Field  /**< One field. */
        };
        Type type = Type::Field;
        std::string key;   /**< Field key, or the text of titles. */
        Rect rect;         /**< Row in content coordinates. */
        Rect editor;       /**< Editor in content coordinates. */
        Rect decrement;    /**< - or < button, empty if none. */
        Rect increment;    /**< + or > button, empty if none. */
        bool last = false; /**< Whether the row is the last of its card. */
    };

    void layout() const;
    void invalidate();
    const Row* row(const std::string& key) const;
    const Row* hit(double x, double y, Part* part) const;
    void click(const Row& row, Part part);
    void step(const SettingField& field, int steps);
    void focus(const std::string& key);
    void commit();
    void cancel();
    void type_text(const std::string& text);
    void draw_row(Canvas& canvas, const Row& row) const;
    void draw_editor(Canvas& canvas, const Row& row, const SettingField& field) const;
    double clamped_scroll(double offset) const;
    std::string value_text(const SettingField& field) const;
    std::string elided(const std::string& text, const Font& font, double width) const;
    void update() const;

    SettingsForm& m_form;
    Theme m_theme;
    ApproximateTextMetrics m_approximateMetrics;
    const TextMetrics* m_metrics = &m_approximateMetrics;
    int m_listener = 0;
    int m_structureListener = 0;
    double m_width = 0.0;
    double m_height = 0.0;
    double m_scroll = 0.0;
    mutable bool m_dirty = true;
    mutable std::vector<Row> m_rows;
    mutable double m_contentHeight = 0.0;
    std::string m_pressedKey;
    Part m_pressedPart = Part::Editor;
    std::string m_focus;
    std::string m_editText;
    std::string m_editOriginal;
    GpuResources m_resources;
    GpuHandle m_pipeline = 0;
    GpuHandle m_quad = 0;
    GpuHandle m_texture = 0;
};

} // namespace mne::viz

#endif // VIZ_FORM_VIEW_H
