//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     qtsettingsstore.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    QtSettingsStore stores the view state of mne_viz in a QSettings.
 */

#ifndef VIZ_QT_QTSETTINGSSTORE_H
#define VIZ_QT_QTSETTINGSSTORE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "viz_qt_global.h"

#include <mne/viz/host/settings_store.h>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QSettings;

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
/**
 * @brief viz::SettingsStore on a QSettings, which it does not own.
 *
 * Booleans, integers and numbers are written as such, so settings written before keep their types.
 *
 * @since 3.0.0
 */
class VIZQTSHARED_EXPORT QtSettingsStore : public viz::SettingsStore
{
public:
    /**
     * @param[in] settings   Settings to read and write; must outlive the store.
     */
    explicit QtSettingsStore(QSettings& settings);

    std::optional<viz::SettingValue> value(const std::string& key) const override;
    void set_value(const std::string& key, const viz::SettingValue& value) override;

private:
    QSettings& m_settings; /**< The wrapped settings. */
};

} // namespace mne::viz::qt

#endif // VIZ_QT_QTSETTINGSSTORE_H
