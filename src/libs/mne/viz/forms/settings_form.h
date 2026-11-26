//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     settings_form.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SettingsForm describes a settings panel (fields, ranges, choices, groups) and holds its values.
 *
 * A panel is described once without a GUI toolkit: its fields with their kind, label, range, unit and choices, in
 * groups, and when a field is enabled or shown. The form holds the values, keeps them valid and tells listeners
 * about changes. A toolkit needs one generic builder that turns any form into widgets (viz::qt::SettingsPanel for Qt
 * Widgets) instead of one hand-written view per panel.
 */

#ifndef VIZ_SETTINGS_FORM_H
#define VIZ_SETTINGS_FORM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/** The value of a settings field. */
using SettingValue = std::variant<bool, int, double, std::string>;

/**
 * @brief One option of a choice field.
 *
 * @since 3.0.0
 */
struct SettingChoice
{
    std::string value; /**< Stored value. */
    std::string label; /**< Shown text. */

    friend bool operator==(const SettingChoice&, const SettingChoice&) = default;
};

/**
 * @brief A field of a settings panel.
 *
 * @since 3.0.0
 */
struct SettingField
{
    /** What the field edits. */
    enum class Kind
    {
        Bool,   /**< On or off (a check box). */
        Int,    /**< A whole number in [minimum, maximum] (a spin box or slider). */
        Double, /**< A number in [minimum, maximum] with decimals (a spin box or slider). */
        Choice, /**< One of the choices (a combo box). */
        Text,   /**< Free text (a line edit). */
        Path,   /**< A file (a line edit with a browse button). */
        Action, /**< No value: a button that triggers listeners. */
        Custom  /**< No value: room for a toolkit-specific widget of the host. */
    };

    /** How a number is edited. */
    enum class Style
    {
        Default, /**< A spin box. */
        Slider   /**< A slider. */
    };

    /** When an editor hands a typed value to the form. */
    enum class Commit
    {
        Immediately, /**< On every change, e.g. each spin box step. */
        WhenFinished /**< When editing finishes, e.g. on Enter or when the spin box loses the focus. */
    };

    std::string key;                     /**< Unique key; also the persisted name. */
    Kind kind = Kind::Bool;              /**< What the field edits. */
    std::string label;                   /**< Label in front of the editor, or the text of check boxes and buttons. */
    std::string tooltip;                 /**< Longer explanation. */
    SettingValue default_value;          /**< Value before anything is set. */
    double minimum = 0.0;                /**< Lowest number. */
    double maximum = 100.0;              /**< Highest number. */
    double step = 1.0;                   /**< Step of spin boxes and sliders. */
    int decimals = 2;                    /**< Decimals of Double fields. */
    std::string prefix;                  /**< Text before a number, e.g. "-". */
    std::string unit;                    /**< Text after a number, e.g. " Hz". */
    Style style = Style::Default;        /**< How a number is edited. */
    Commit commit = Commit::Immediately; /**< When spin boxes and line edits hand their value to the form. */
    std::vector<SettingChoice> choices;  /**< Options of Choice fields. */
    std::string file_filter;             /**< File dialog filter of Path fields, e.g. "FIFF files (*.fif)". */
    std::string group;                   /**< Title of the group the field is shown in; empty for none. */
    std::string enabled_if;              /**< Key of a Bool field the field needs on ("!key": off); empty for always. */
    std::string visible_if;              /**< Key of a Bool field the field is shown with ("!key": without). */
    bool hidden = false;                 /**< Never shown, e.g. a mode flag of the host that conditions refer to. */
    bool persistent = true;              /**< Whether hosts store the value with the other settings. */
};

/**
 * @brief A settings panel: its fields in order and their values.
 *
 * Numbers are clamped to their range and choices must be one of the field's, so the values are always valid.
 * Listeners hear about every value that changes and every triggered action; structure listeners hear when fields
 * change their range, choices, label or presence.
 *
 * @snippet ex_scene/main.cpp settings_form_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT SettingsForm
{
public:
    /** Called with the key and the new value of a changed field, or the key and no value of a triggered action. */
    using Listener = std::function<void(const std::string& key, const std::optional<SettingValue>& value)>;

    //=========================================================================================================
    /**
     * Creates an empty form.
     *
     * @param[in] title   Title of the panel.
     */
    explicit SettingsForm(std::string title = {});

    /** @return The title of the panel. */
    const std::string& title() const
    {
        return m_title;
    }

    //=========================================================================================================
    /**
     * Adds a field; its value starts at the (clamped) default.
     *
     * @param[in] field   Field; a field with the same key replaces the old one.
     *
     * @return The stored field.
     */
    const SettingField& add(SettingField field);

    //=========================================================================================================
    /**
     * Adds a Bool field.
     *
     * @param[in] key     Field key.
     * @param[in] label   Text of the check box.
     * @param[in] value   Default value.
     * @param[in] group   Group title; empty for none.
     *
     * @return The stored field.
     */
    const SettingField& add_bool(const std::string& key, const std::string& label, bool value, const std::string& group = {});

    //=========================================================================================================
    /**
     * Adds an Int field.
     *
     * @param[in] key       Field key.
     * @param[in] label     Label.
     * @param[in] value     Default value.
     * @param[in] minimum   Lowest number.
     * @param[in] maximum   Highest number.
     * @param[in] step      Step of the spin box.
     * @param[in] unit      Text after the number, e.g. " ms".
     * @param[in] group     Group title; empty for none.
     *
     * @return The stored field.
     */
    const SettingField& add_int(const std::string& key, const std::string& label, int value, int minimum, int maximum, int step = 1,
                                const std::string& unit = {}, const std::string& group = {});

    //=========================================================================================================
    /**
     * Adds a Double field.
     *
     * @param[in] key        Field key.
     * @param[in] label      Label.
     * @param[in] value      Default value.
     * @param[in] minimum    Lowest number.
     * @param[in] maximum    Highest number.
     * @param[in] step       Step of the spin box.
     * @param[in] decimals   Decimals shown.
     * @param[in] unit       Text after the number, e.g. " Hz".
     * @param[in] group      Group title; empty for none.
     *
     * @return The stored field.
     */
    const SettingField& add_double(const std::string& key, const std::string& label, double value, double minimum, double maximum,
                                   double step, int decimals, const std::string& unit = {}, const std::string& group = {});

    //=========================================================================================================
    /**
     * Adds a Choice field.
     *
     * @param[in] key       Field key.
     * @param[in] label     Label.
     * @param[in] choices   Options.
     * @param[in] value     Default choice; the first one if it is none of them.
     * @param[in] group     Group title; empty for none.
     *
     * @return The stored field.
     */
    const SettingField& add_choice(const std::string& key, const std::string& label, std::vector<SettingChoice> choices,
                                   const std::string& value, const std::string& group = {});

    //=========================================================================================================
    /**
     * Adds a Text field.
     *
     * @param[in] key     Field key.
     * @param[in] label   Label.
     * @param[in] value   Default text.
     * @param[in] group   Group title; empty for none.
     *
     * @return The stored field.
     */
    const SettingField& add_text(const std::string& key, const std::string& label, const std::string& value, const std::string& group = {});

    //=========================================================================================================
    /**
     * Adds a flag: a Bool field that is never shown or stored, e.g. a mode of the host that conditions refer to.
     *
     * @param[in] key     Field key.
     * @param[in] value   Default value.
     *
     * @return The stored field.
     */
    const SettingField& add_flag(const std::string& key, bool value = false);

    //=========================================================================================================
    /**
     * Adds an Action field, a button that triggers listeners.
     *
     * @param[in] key     Field key.
     * @param[in] label   Text of the button.
     * @param[in] group   Group title; empty for none.
     *
     * @return The stored field.
     */
    const SettingField& add_action(const std::string& key, const std::string& label, const std::string& group = {});

    //=========================================================================================================
    /**
     * Adds a Custom field the host fills with its own widget.
     *
     * @param[in] key     Field key.
     * @param[in] label   Label in front of the widget; empty for none.
     * @param[in] group   Group title; empty for none.
     *
     * @return The stored field.
     */
    const SettingField& add_custom(const std::string& key, const std::string& label = {}, const std::string& group = {});

    //=========================================================================================================
    /**
     * @param[in] key   Field key.
     *
     * @return The field, or nullptr.
     */
    const SettingField* field(const std::string& key) const;

    /** @return The fields in order. */
    const std::vector<SettingField>& fields() const
    {
        return m_fields;
    }

    /** @return The group titles in the order of their first field; the fields without group form "". */
    std::vector<std::string> groups() const;

    //=========================================================================================================
    /**
     * Changes the range of a number field, clamping its value.
     *
     * @param[in] key       Field key.
     * @param[in] minimum   Lowest number.
     * @param[in] maximum   Highest number; below @p minimum it becomes @p minimum.
     */
    void set_range(const std::string& key, double minimum, double maximum);

    /**
     * Replaces the choices of a Choice field; a value that is no longer a choice becomes the first one.
     *
     * @param[in] key       Field key.
     * @param[in] choices   New options.
     */
    void set_choices(const std::string& key, std::vector<SettingChoice> choices);

    /**
     * Changes the label of a field.
     *
     * @param[in] key     Field key.
     * @param[in] label   New label.
     */
    void set_label(const std::string& key, const std::string& label);

    //=========================================================================================================
    /**
     * @param[in] key   Field key.
     *
     * @return The value, or none for unknown keys, actions and custom fields.
     */
    std::optional<SettingValue> value(const std::string& key) const;

    /**
     * @param[in] key        Field key.
     * @param[in] fallback   Returned if the field has no value of type @p T.
     *
     * @return The value as @p T; Int values also convert to double.
     */
    template<typename T>
    T get(const std::string& key, T fallback = T{}) const
    {
        const std::optional<SettingValue> stored = value(key);
        if (!stored) {
            return fallback;
        }
        if (const T* typed = std::get_if<T>(&*stored)) {
            return *typed;
        }
        if constexpr (std::is_same_v<T, double>) {
            if (const int* number = std::get_if<int>(&*stored)) {
                return static_cast<double>(*number);
            }
        }
        return fallback;
    }

    //=========================================================================================================
    /**
     * Sets a value, made valid for the field (clamped, rounded to an int, or rejected if no choice).
     *
     * @param[in] key     Field key.
     * @param[in] value   New value; numbers convert between int and double.
     *
     * @return Whether the stored value changed; listeners are told then.
     */
    bool set_value(const std::string& key, const SettingValue& value);

    /** @return All values by key. */
    std::map<std::string, SettingValue> values() const
    {
        return m_values;
    }

    /**
     * Sets several values, e.g. persisted ones; unknown keys are ignored.
     *
     * @param[in] values   Values by key.
     */
    void set_values(const std::map<std::string, SettingValue>& values);

    /** Sets every field back to its default. */
    void reset();

    //=========================================================================================================
    /**
     * Triggers an action: listeners hear its key without a value.
     *
     * @param[in] key   Key of an Action field; other keys are ignored.
     */
    void trigger(const std::string& key);

    /**
     * @param[in] key   Field key.
     *
     * @return Whether the field can be edited, from its enabled_if condition.
     */
    bool is_enabled(const std::string& key) const;

    /**
     * @param[in] key   Field key.
     *
     * @return Whether the field is shown: not hidden and its visible_if condition holds.
     */
    bool is_visible(const std::string& key) const;

    //=========================================================================================================
    /**
     * Registers a listener for value changes and actions.
     *
     * @param[in] listener   Listener.
     *
     * @return An id for remove_listener().
     */
    int add_listener(Listener listener);

    /**
     * Registers a listener for changes of the fields themselves (range, choices, label, added fields).
     *
     * @param[in] listener   Called with the key of the changed field.
     *
     * @return An id for remove_listener().
     */
    int add_structure_listener(std::function<void(const std::string& key)> listener);

    /**
     * Removes a listener.
     *
     * @param[in] id   Id from add_listener() or add_structure_listener().
     */
    void remove_listener(int id);

private:
    SettingField* find(const std::string& key);
    std::optional<SettingValue> valid_value(const SettingField& field, const SettingValue& value) const;
    bool condition_holds(const std::string& condition) const;
    void store(const std::string& key, const SettingValue& value);
    void notify_structure(const std::string& key);

    std::string m_title;
    std::vector<SettingField> m_fields;
    std::map<std::string, SettingValue> m_values;
    std::map<int, Listener> m_listeners;
    std::map<int, std::function<void(const std::string&)>> m_structureListeners;
    int m_nextListener = 1;
};

} // namespace mne::viz

#endif // VIZ_SETTINGS_FORM_H
