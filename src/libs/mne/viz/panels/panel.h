//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Panel: a toolkit-free tool panel of the applications, shown by any backend.
 *
 * A panel (compensators, projectors, forward model settings, ...) holds the state and rules of one tool and
 * describes its user interface with the generic building blocks of mne_viz, such as a SettingsForm. Each backend
 * implements one generic host for panels instead of a hand-written view per tool, so the tools look the same
 * everywhere and a new backend shows all of them.
 */

#ifndef VIZ_PANEL_H
#define VIZ_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "../forms/settings_form.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <memory>
#include <string>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class ItemList;
class SceneFigure;
class SettingsStore;

//=============================================================================================================
/**
 * @brief Base of the toolkit-free tool panels: a title, a settings form with item lists, an optional figure,
 * modes and persistence.
 *
 * @snippet ex_scene/main.cpp panel_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT Panel
{
public:
    /** Who uses the application. */
    enum class GuiMode
    {
        Clinical, /**< Fewer, safer options. */
        Research  /**< All options. */
    };

    /** How the data arrive. */
    enum class ProcessingMode
    {
        RealTime, /**< Streamed while recorded. */
        Offline   /**< From a file. */
    };

    //=========================================================================================================
    /**
     * @param[in] id      Settings group of the panel, e.g. "CompensatorView".
     * @param[in] title   Window title.
     */
    Panel(std::string id, std::string title);

    virtual ~Panel();

    Panel(const Panel&) = delete;
    Panel& operator=(const Panel&) = delete;

    /** @return The settings group of the panel. */
    const std::string& id() const
    {
        return m_id;
    }

    /** @return The window title. */
    const std::string& title() const
    {
        return m_title;
    }

    /** @return The controls of the panel. */
    SettingsForm& form()
    {
        return *m_form;
    }

    /** @return The controls of the panel. */
    const SettingsForm& form() const
    {
        return *m_form;
    }

    /** @return The form, shared with hosts that may outlive the panel's owner. */
    const std::shared_ptr<SettingsForm>& shared_form() const
    {
        return m_form;
    }

    /** @return The width hosts give the panel in logical pixels; 0 for any. */
    int width() const
    {
        return m_width;
    }

    //=========================================================================================================
    /**
     * Sets who uses the application; panels hide or show options.
     *
     * @param[in] mode   New mode.
     */
    void set_gui_mode(GuiMode mode);

    /** @return Who uses the application. */
    GuiMode gui_mode() const
    {
        return m_guiMode;
    }

    //=========================================================================================================
    /**
     * Sets how the data arrive; panels hide or show options.
     *
     * @param[in] mode   New mode.
     */
    void set_processing_mode(ProcessingMode mode);

    /** @return How the data arrive. */
    ProcessingMode processing_mode() const
    {
        return m_processingMode;
    }

    //=========================================================================================================
    /**
     * @return The interactive figure that hosts show below the form, or null for panels with only a form.
     */
    const std::shared_ptr<SceneFigure>& figure() const
    {
        return m_figure;
    }

    //=========================================================================================================
    /**
     * @return The item lists by key; each sits in the custom field of its key, where hosts show it.
     */
    const std::map<std::string, std::shared_ptr<ItemList>>& lists() const
    {
        return m_lists;
    }

    //=========================================================================================================
    /**
     * @param[in] key   Key of a list.
     *
     * @return The list, or null.
     */
    ItemList* list(const std::string& key) const;

    //=========================================================================================================
    /**
     * Restores the panel from @p store. By default the persistent form fields are read from
     * "<path>/<id>/<field key>".
     *
     * @param[in] store   Stored settings.
     * @param[in] path    Settings path of the host, e.g. "MNESCAN/NoiseReduction"; empty restores nothing.
     */
    virtual void load(const SettingsStore& store, const std::string& path);

    //=========================================================================================================
    /**
     * Stores the panel in @p store, under the keys load() reads.
     *
     * @param[in] store   Stored settings.
     * @param[in] path    Settings path of the host; empty stores nothing.
     */
    virtual void save(SettingsStore& store, const std::string& path) const;

protected:
    //=========================================================================================================
    /**
     * Sets the width hosts give the panel.
     *
     * @param[in] width   Width in logical pixels; 0 for any.
     */
    void set_width(int width)
    {
        m_width = width;
    }

    //=========================================================================================================
    /**
     * Sets the figure hosts show below the form; call it in the constructor.
     *
     * @param[in] figure   The figure.
     */
    void set_figure(std::shared_ptr<SceneFigure> figure)
    {
        m_figure = std::move(figure);
    }

    //=========================================================================================================
    /**
     * Adds an item list in a custom field of the form; call it in the constructor.
     *
     * @param[in] key     Key of the field and the list.
     * @param[in] label   Label above the list; empty for none.
     * @param[in] list    The list.
     * @param[in] group   Group of the field.
     *
     * @return The list.
     */
    ItemList& add_list(const std::string& key, const std::string& label, std::shared_ptr<ItemList> list,
                       const std::string& group = {});

    //=========================================================================================================
    /**
     * @param[in] path   Settings path of the host.
     * @param[in] name   Name below the panel's group.
     *
     * @return "<path>/<id>/<name>".
     */
    std::string settings_key(const std::string& path, const std::string& name) const;

    /** Called after the GUI mode changed. */
    virtual void gui_mode_changed();

    /** Called after the processing mode changed. */
    virtual void processing_mode_changed();

private:
    std::string m_id;
    std::string m_title;
    std::shared_ptr<SettingsForm> m_form;
    std::shared_ptr<SceneFigure> m_figure;
    std::map<std::string, std::shared_ptr<ItemList>> m_lists;
    int m_width = 0;
    GuiMode m_guiMode = GuiMode::Research;
    ProcessingMode m_processingMode = ProcessingMode::RealTime;
};

} // namespace mne::viz

#endif // VIZ_PANEL_H
