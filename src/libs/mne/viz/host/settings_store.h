//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     settings_store.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SettingsStore persists view state as key/value pairs in whatever storage a backend provides.
 *
 * Views remember their state (camera, visibility, shaders) between runs. They read and write it through
 * SettingsStore, so the storage is the backend's choice: QSettings in the Qt backend, a file or memory elsewhere.
 * MemorySettingsStore keeps the values in memory, for tests and hosts without persistent settings.
 */

#ifndef VIZ_SETTINGS_STORE_H
#define VIZ_SETTINGS_STORE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <optional>
#include <string>
#include <variant>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * A stored value. Stores may hand back another alternative than was written (an INI file returns text), so
 * readers convert with the typed accessors of SettingsStore.
 *
 * @since 3.0.0
 */
using SettingValue = std::variant<bool, int, double, std::string>;

//=============================================================================================================
/**
 * @brief Key/value storage for the persistent state of views.
 *
 * @snippet ex_disp3d_scene/main.cpp view_state_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT SettingsStore
{
public:
    virtual ~SettingsStore() = default;

    /**
     * @param[in] key   Key.
     * @return The stored value, or nothing when @p key is absent.
     */
    virtual std::optional<SettingValue> value(const std::string& key) const = 0;

    /**
     * Store @p value under @p key, replacing any previous value.
     *
     * @param[in] key     Key.
     * @param[in] value   Value.
     */
    virtual void set_value(const std::string& key, const SettingValue& value) = 0;

    /**
     * @param[in] key   Key.
     * @return Whether a value is stored under @p key.
     */
    bool contains(const std::string& key) const;

    /**
     * @param[in] key        Key.
     * @param[in] fallback   Result when @p key is absent or not a boolean ("true"/"false", "1"/"0" and numbers count).
     * @return The stored boolean.
     */
    bool read_bool(const std::string& key, bool fallback) const;

    /**
     * @param[in] key        Key.
     * @param[in] fallback   Result when @p key is absent or not a number.
     * @return The stored integer; floating-point values are rounded.
     */
    int read_int(const std::string& key, int fallback) const;

    /**
     * @param[in] key        Key.
     * @param[in] fallback   Result when @p key is absent or not a number.
     * @return The stored number.
     */
    double read_double(const std::string& key, double fallback) const;

    /**
     * @param[in] key        Key.
     * @param[in] fallback   Result when @p key is absent.
     * @return The stored text; other values are formatted as text.
     */
    std::string read_string(const std::string& key, const std::string& fallback) const;
};

//=============================================================================================================
/**
 * @brief SettingsStore that keeps the values in memory.
 *
 * @snippet ex_disp3d_scene/main.cpp view_state_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT MemorySettingsStore : public SettingsStore
{
public:
    std::optional<SettingValue> value(const std::string& key) const override;
    void set_value(const std::string& key, const SettingValue& value) override;

    /**
     * @return All stored values by key.
     */
    const std::map<std::string, SettingValue>& values() const
    {
        return m_values;
    }

private:
    std::map<std::string, SettingValue> m_values; /**< Stored values by key. */
};

} // namespace mne::viz

#endif // VIZ_SETTINGS_STORE_H
