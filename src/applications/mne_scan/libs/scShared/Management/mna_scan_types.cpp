//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_scan_types.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Conversions between plugin attributes and MNA node attributes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_scan_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QJsonDocument>
#include <QJsonObject>

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

// Attributes are a handful of settings saved with the pipeline, so going through JSON text is cheap and exact.

mne::JsonObject mne::scan::toNodeAttributes(const QVariantMap& attributes)
{
    const QByteArray text = QJsonDocument(QJsonObject::fromVariantMap(attributes)).toJson(QJsonDocument::Compact);
    return JsonDocument::from_json(std::string_view(text.constData(), static_cast<std::size_t>(text.size()))).object();
}

//=============================================================================================================

QVariantMap mne::scan::toPluginAttributes(const JsonObject& attributes)
{
    return QJsonDocument::fromJson(QByteArray::fromStdString(JsonDocument(attributes).to_json(JsonDocument::Compact))).object().toVariantMap();
}
