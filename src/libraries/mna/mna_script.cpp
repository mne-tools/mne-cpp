//=============================================================================================================
/**
 * @file     mna_script.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    MnaScript struct implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_script.h"

#include <QJsonArray>
#include <QCborArray>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QJsonObject MnaScript::toJson() const
{
    QJsonObject json;
    json[QStringLiteral("language")]    = language;

    if (!interpreter.isEmpty()) {
        json[QStringLiteral("interpreter")] = interpreter;
    }

    if (!interpreterArgs.isEmpty()) {
        QJsonArray arr;
        for (const QString& a : interpreterArgs) {
            arr.append(a);
        }
        json[QStringLiteral("interpreter_args")] = arr;
    }

    json[QStringLiteral("code")] = code;

    if (!sourceUri.isEmpty()) {
        json[QStringLiteral("source_uri")] = sourceUri;
    }

    if (!codeSha256.isEmpty()) {
        json[QStringLiteral("code_sha256")] = codeSha256;
    }

    if (keepTempFile) {
        json[QStringLiteral("keep_temp_file")] = true;
    }

    return json;
}

//=============================================================================================================

MnaScript MnaScript::fromJson(const QJsonObject& json)
{
    MnaScript s;
    s.language     = json.value(QStringLiteral("language")).toString();
    s.interpreter  = json.value(QStringLiteral("interpreter")).toString();
    s.code         = json.value(QStringLiteral("code")).toString();
    s.sourceUri    = json.value(QStringLiteral("source_uri")).toString();
    s.codeSha256   = json.value(QStringLiteral("code_sha256")).toString();
    s.keepTempFile = json.value(QStringLiteral("keep_temp_file")).toBool(false);

    const QJsonArray arr = json.value(QStringLiteral("interpreter_args")).toArray();
    for (const QJsonValue& v : arr) {
        s.interpreterArgs.append(v.toString());
    }

    return s;
}

//=============================================================================================================

QCborMap MnaScript::toCbor() const
{
    QCborMap cbor;
    cbor.insert(QStringLiteral("language"), language);

    if (!interpreter.isEmpty()) {
        cbor.insert(QStringLiteral("interpreter"), interpreter);
    }

    if (!interpreterArgs.isEmpty()) {
        QCborArray arr;
        for (const QString& a : interpreterArgs) {
            arr.append(a);
        }
        cbor.insert(QStringLiteral("interpreter_args"), arr);
    }

    cbor.insert(QStringLiteral("code"), code);

    if (!sourceUri.isEmpty()) {
        cbor.insert(QStringLiteral("source_uri"), sourceUri);
    }

    if (!codeSha256.isEmpty()) {
        cbor.insert(QStringLiteral("code_sha256"), codeSha256);
    }

    if (keepTempFile) {
        cbor.insert(QStringLiteral("keep_temp_file"), true);
    }

    return cbor;
}

//=============================================================================================================

MnaScript MnaScript::fromCbor(const QCborMap& cbor)
{
    MnaScript s;
    s.language     = cbor.value(QStringLiteral("language")).toString();
    s.interpreter  = cbor.value(QStringLiteral("interpreter")).toString();
    s.code         = cbor.value(QStringLiteral("code")).toString();
    s.sourceUri    = cbor.value(QStringLiteral("source_uri")).toString();
    s.codeSha256   = cbor.value(QStringLiteral("code_sha256")).toString();
    s.keepTempFile = cbor.value(QStringLiteral("keep_temp_file")).toBool();

    const QCborArray arr = cbor.value(QStringLiteral("interpreter_args")).toArray();
    for (const QCborValue& v : arr) {
        s.interpreterArgs.append(v.toString());
    }

    return s;
}
