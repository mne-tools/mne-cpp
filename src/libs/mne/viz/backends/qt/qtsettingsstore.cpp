//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     qtsettingsstore.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    QtSettingsStore converts between viz::SettingValue and QVariant.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "qtsettingsstore.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>
#include <QString>
#include <QVariant>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <type_traits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QtSettingsStore::QtSettingsStore(QSettings& settings)
: m_settings(settings)
{
}

//=============================================================================================================

std::optional<SettingValue> QtSettingsStore::value(const std::string& key) const
{
    const QString qKey = QString::fromStdString(key);
    if (!m_settings.contains(qKey)) {
        return std::nullopt;
    }
    const QVariant stored = m_settings.value(qKey);
    switch (stored.typeId()) {
        case QMetaType::Bool:
            return stored.toBool();
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
            return stored.toInt();
        case QMetaType::Float:
        case QMetaType::Double:
            return stored.toDouble();
        default:
            return stored.toString().toStdString();
    }
}

//=============================================================================================================

void QtSettingsStore::set_value(const std::string& key, const SettingValue& value)
{
    const QVariant stored = std::visit(
        [](const auto& v) -> QVariant {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                return QString::fromStdString(v);
            } else {
                return QVariant(v);
            }
        },
        value);
    m_settings.setValue(QString::fromStdString(key), stored);
}
