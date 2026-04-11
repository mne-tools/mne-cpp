//=============================================================================================================
/**
 * @file     mna_param_binding.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    MnaParamBinding struct implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_param_binding.h"

#include <QJsonArray>
#include <QCborArray>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QJsonObject MnaParamBinding::toJson() const
{
    QJsonObject json;
    json[QStringLiteral("target")]     = targetPath;
    json[QStringLiteral("expression")] = expression;
    json[QStringLiteral("trigger")]    = trigger;

    if (trigger == QStringLiteral("periodic") && periodMs > 0) {
        json[QStringLiteral("period_ms")] = periodMs;
    }

    if (!dependencies.isEmpty()) {
        QJsonArray arr;
        for (const QString& dep : dependencies) {
            arr.append(dep);
        }
        json[QStringLiteral("dependencies")] = arr;
    }

    return json;
}

//=============================================================================================================

MnaParamBinding MnaParamBinding::fromJson(const QJsonObject& json)
{
    MnaParamBinding b;
    b.targetPath  = json.value(QStringLiteral("target")).toString();
    b.expression  = json.value(QStringLiteral("expression")).toString();
    b.trigger     = json.value(QStringLiteral("trigger")).toString();
    b.periodMs    = json.value(QStringLiteral("period_ms")).toInt(0);

    const QJsonArray arr = json.value(QStringLiteral("dependencies")).toArray();
    for (const QJsonValue& v : arr) {
        b.dependencies.append(v.toString());
    }

    return b;
}

//=============================================================================================================

QCborMap MnaParamBinding::toCbor() const
{
    QCborMap cbor;
    cbor.insert(QStringLiteral("target"),     targetPath);
    cbor.insert(QStringLiteral("expression"), expression);
    cbor.insert(QStringLiteral("trigger"),    trigger);

    if (trigger == QStringLiteral("periodic") && periodMs > 0) {
        cbor.insert(QStringLiteral("period_ms"), periodMs);
    }

    if (!dependencies.isEmpty()) {
        QCborArray arr;
        for (const QString& dep : dependencies) {
            arr.append(dep);
        }
        cbor.insert(QStringLiteral("dependencies"), arr);
    }

    return cbor;
}

//=============================================================================================================

MnaParamBinding MnaParamBinding::fromCbor(const QCborMap& cbor)
{
    MnaParamBinding b;
    b.targetPath  = cbor.value(QStringLiteral("target")).toString();
    b.expression  = cbor.value(QStringLiteral("expression")).toString();
    b.trigger     = cbor.value(QStringLiteral("trigger")).toString();
    b.periodMs    = static_cast<int>(cbor.value(QStringLiteral("period_ms")).toInteger(0));

    const QCborArray arr = cbor.value(QStringLiteral("dependencies")).toArray();
    for (const QCborValue& v : arr) {
        b.dependencies.append(v.toString());
    }

    return b;
}
