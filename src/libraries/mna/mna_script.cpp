//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_script.cpp
 * @since 2026
 * @date  April 2026
 * @brief JSON and CBOR codecs for @ref MnaScript including code body, interpreter selection, integrity hash and optional source URI.
 *
 * The implementation writes the script @c code verbatim (after
 * resolving an optional @c sourceUri to the on-disk file at save
 * time), records the SHA-256 in @c codeSha256, and serialises the
 * interpreter command plus @c interpreterArgs in execution order.
 * The @c keepTempFile debug flag and the @c language /
 * @c interpreter pair round-trip symmetrically so a project can be
 * re-executed bit-for-bit later. Unknown fields are captured into
 * the standard @c extras pattern used across MNALIB structs.
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
