//=============================================================================================================
/**
 * @file     mna_verification.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    MnaVerification, MnaVerificationCheck, MnaVerificationResult, MnaProvenance implementations.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_verification.h"

#include <QJsonArray>
#include <QCborArray>
#include <QSysInfo>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// MnaVerificationCheck
//=============================================================================================================

QJsonObject MnaVerificationCheck::toJson() const
{
    QJsonObject json;
    json[QStringLiteral("id")]          = id;
    json[QStringLiteral("description")] = description;
    json[QStringLiteral("phase")]       = phase;
    json[QStringLiteral("expression")]  = expression;
    if (!script.code.isEmpty()) {
        json[QStringLiteral("script")] = script.toJson();
    }
    json[QStringLiteral("severity")]    = severity;
    if (!onFail.isEmpty()) {
        json[QStringLiteral("on_fail")] = onFail;
    }
    return json;
}

//=============================================================================================================

MnaVerificationCheck MnaVerificationCheck::fromJson(const QJsonObject& json)
{
    MnaVerificationCheck c;
    c.id          = json.value(QStringLiteral("id")).toString();
    c.description = json.value(QStringLiteral("description")).toString();
    c.phase       = json.value(QStringLiteral("phase")).toString();
    c.expression  = json.value(QStringLiteral("expression")).toString();
    if (json.contains(QStringLiteral("script"))) {
        c.script = MnaScript::fromJson(json.value(QStringLiteral("script")).toObject());
    }
    c.severity    = json.value(QStringLiteral("severity")).toString();
    c.onFail      = json.value(QStringLiteral("on_fail")).toString();
    return c;
}

//=============================================================================================================

QCborMap MnaVerificationCheck::toCbor() const
{
    QCborMap cbor;
    cbor.insert(QStringLiteral("id"),          id);
    cbor.insert(QStringLiteral("description"), description);
    cbor.insert(QStringLiteral("phase"),       phase);
    cbor.insert(QStringLiteral("expression"),  expression);
    if (!script.code.isEmpty()) {
        cbor.insert(QStringLiteral("script"), script.toCbor());
    }
    cbor.insert(QStringLiteral("severity"),    severity);
    if (!onFail.isEmpty()) {
        cbor.insert(QStringLiteral("on_fail"), onFail);
    }
    return cbor;
}

//=============================================================================================================

MnaVerificationCheck MnaVerificationCheck::fromCbor(const QCborMap& cbor)
{
    MnaVerificationCheck c;
    c.id          = cbor.value(QStringLiteral("id")).toString();
    c.description = cbor.value(QStringLiteral("description")).toString();
    c.phase       = cbor.value(QStringLiteral("phase")).toString();
    c.expression  = cbor.value(QStringLiteral("expression")).toString();
    if (cbor.contains(QStringLiteral("script"))) {
        c.script = MnaScript::fromCbor(cbor.value(QStringLiteral("script")).toMap());
    }
    c.severity    = cbor.value(QStringLiteral("severity")).toString();
    c.onFail      = cbor.value(QStringLiteral("on_fail")).toString();
    return c;
}

//=============================================================================================================
// MnaVerificationResult
//=============================================================================================================

QJsonObject MnaVerificationResult::toJson() const
{
    QJsonObject json;
    json[QStringLiteral("check_id")]     = checkId;
    json[QStringLiteral("passed")]       = passed;
    json[QStringLiteral("severity")]     = severity;
    json[QStringLiteral("message")]      = message;
    json[QStringLiteral("actual_value")] = QJsonValue::fromVariant(actualValue);
    if (evaluatedAt.isValid()) {
        json[QStringLiteral("evaluated_at")] = evaluatedAt.toString(Qt::ISODateWithMs);
    }
    return json;
}

//=============================================================================================================

MnaVerificationResult MnaVerificationResult::fromJson(const QJsonObject& json)
{
    MnaVerificationResult r;
    r.checkId     = json.value(QStringLiteral("check_id")).toString();
    r.passed      = json.value(QStringLiteral("passed")).toBool(false);
    r.severity    = json.value(QStringLiteral("severity")).toString();
    r.message     = json.value(QStringLiteral("message")).toString();
    r.actualValue = json.value(QStringLiteral("actual_value")).toVariant();
    r.evaluatedAt = QDateTime::fromString(json.value(QStringLiteral("evaluated_at")).toString(), Qt::ISODateWithMs);
    return r;
}

//=============================================================================================================

QCborMap MnaVerificationResult::toCbor() const
{
    QCborMap cbor;
    cbor.insert(QStringLiteral("check_id"),     checkId);
    cbor.insert(QStringLiteral("passed"),       passed);
    cbor.insert(QStringLiteral("severity"),     severity);
    cbor.insert(QStringLiteral("message"),      message);
    cbor.insert(QStringLiteral("actual_value"), QCborValue::fromVariant(actualValue));
    if (evaluatedAt.isValid()) {
        cbor.insert(QStringLiteral("evaluated_at"), evaluatedAt.toString(Qt::ISODateWithMs));
    }
    return cbor;
}

//=============================================================================================================

MnaVerificationResult MnaVerificationResult::fromCbor(const QCborMap& cbor)
{
    MnaVerificationResult r;
    r.checkId     = cbor.value(QStringLiteral("check_id")).toString();
    r.passed      = cbor.value(QStringLiteral("passed")).toBool();
    r.severity    = cbor.value(QStringLiteral("severity")).toString();
    r.message     = cbor.value(QStringLiteral("message")).toString();
    r.actualValue = cbor.value(QStringLiteral("actual_value")).toVariant();
    r.evaluatedAt = QDateTime::fromString(cbor.value(QStringLiteral("evaluated_at")).toString(), Qt::ISODateWithMs);
    return r;
}

//=============================================================================================================
// MnaProvenance
//=============================================================================================================

QJsonObject MnaProvenance::toJson() const
{
    QJsonObject json;

    if (!inputHashes.isEmpty()) {
        QJsonObject hashObj;
        for (auto it = inputHashes.constBegin(); it != inputHashes.constEnd(); ++it) {
            hashObj.insert(it.key(), it.value());
        }
        json[QStringLiteral("input_hashes")] = hashObj;
    }

    if (!resolvedAttributes.isEmpty()) {
        QJsonObject attrObj;
        for (auto it = resolvedAttributes.constBegin(); it != resolvedAttributes.constEnd(); ++it) {
            attrObj.insert(it.key(), QJsonValue::fromVariant(it.value()));
        }
        json[QStringLiteral("resolved_attributes")] = attrObj;
    }

    if (!mneCppVersion.isEmpty()) json[QStringLiteral("mne_cpp_version")]  = mneCppVersion;
    if (!qtVersion.isEmpty())     json[QStringLiteral("qt_version")]       = qtVersion;
    if (!compilerInfo.isEmpty())  json[QStringLiteral("compiler_info")]    = compilerInfo;
    if (!osInfo.isEmpty())        json[QStringLiteral("os_info")]          = osInfo;
    if (!hostName.isEmpty())      json[QStringLiteral("host_name")]        = hostName;
    if (!externalToolVersion.isEmpty()) json[QStringLiteral("external_tool_version")] = externalToolVersion;

    if (startedAt.isValid())  json[QStringLiteral("started_at")]  = startedAt.toString(Qt::ISODateWithMs);
    if (finishedAt.isValid()) json[QStringLiteral("finished_at")] = finishedAt.toString(Qt::ISODateWithMs);
    if (wallTimeMs > 0)       json[QStringLiteral("wall_time_ms")]       = wallTimeMs;
    if (peakMemoryBytes > 0)  json[QStringLiteral("peak_memory_bytes")]  = peakMemoryBytes;
    if (randomSeed >= 0)      json[QStringLiteral("random_seed")]        = randomSeed;

    return json;
}

//=============================================================================================================

MnaProvenance MnaProvenance::fromJson(const QJsonObject& json)
{
    MnaProvenance p;

    const QJsonObject hashObj = json.value(QStringLiteral("input_hashes")).toObject();
    for (auto it = hashObj.constBegin(); it != hashObj.constEnd(); ++it) {
        p.inputHashes.insert(it.key(), it.value().toString());
    }

    const QJsonObject attrObj = json.value(QStringLiteral("resolved_attributes")).toObject();
    for (auto it = attrObj.constBegin(); it != attrObj.constEnd(); ++it) {
        p.resolvedAttributes.insert(it.key(), it.value().toVariant());
    }

    p.mneCppVersion       = json.value(QStringLiteral("mne_cpp_version")).toString();
    p.qtVersion           = json.value(QStringLiteral("qt_version")).toString();
    p.compilerInfo        = json.value(QStringLiteral("compiler_info")).toString();
    p.osInfo              = json.value(QStringLiteral("os_info")).toString();
    p.hostName            = json.value(QStringLiteral("host_name")).toString();
    p.externalToolVersion = json.value(QStringLiteral("external_tool_version")).toString();

    p.startedAt       = QDateTime::fromString(json.value(QStringLiteral("started_at")).toString(), Qt::ISODateWithMs);
    p.finishedAt      = QDateTime::fromString(json.value(QStringLiteral("finished_at")).toString(), Qt::ISODateWithMs);
    p.wallTimeMs      = static_cast<qint64>(json.value(QStringLiteral("wall_time_ms")).toDouble(0));
    p.peakMemoryBytes = static_cast<qint64>(json.value(QStringLiteral("peak_memory_bytes")).toDouble(0));
    p.randomSeed      = static_cast<qint64>(json.value(QStringLiteral("random_seed")).toDouble(-1));

    return p;
}

//=============================================================================================================

QCborMap MnaProvenance::toCbor() const
{
    QCborMap cbor;

    if (!inputHashes.isEmpty()) {
        QCborMap hashMap;
        for (auto it = inputHashes.constBegin(); it != inputHashes.constEnd(); ++it) {
            hashMap.insert(it.key(), it.value());
        }
        cbor.insert(QStringLiteral("input_hashes"), hashMap);
    }

    if (!resolvedAttributes.isEmpty()) {
        QCborMap attrMap;
        for (auto it = resolvedAttributes.constBegin(); it != resolvedAttributes.constEnd(); ++it) {
            attrMap.insert(it.key(), QCborValue::fromVariant(it.value()));
        }
        cbor.insert(QStringLiteral("resolved_attributes"), attrMap);
    }

    if (!mneCppVersion.isEmpty()) cbor.insert(QStringLiteral("mne_cpp_version"),  mneCppVersion);
    if (!qtVersion.isEmpty())     cbor.insert(QStringLiteral("qt_version"),       qtVersion);
    if (!compilerInfo.isEmpty())  cbor.insert(QStringLiteral("compiler_info"),    compilerInfo);
    if (!osInfo.isEmpty())        cbor.insert(QStringLiteral("os_info"),          osInfo);
    if (!hostName.isEmpty())      cbor.insert(QStringLiteral("host_name"),        hostName);
    if (!externalToolVersion.isEmpty()) cbor.insert(QStringLiteral("external_tool_version"), externalToolVersion);

    if (startedAt.isValid())  cbor.insert(QStringLiteral("started_at"),  startedAt.toString(Qt::ISODateWithMs));
    if (finishedAt.isValid()) cbor.insert(QStringLiteral("finished_at"), finishedAt.toString(Qt::ISODateWithMs));
    if (wallTimeMs > 0)       cbor.insert(QStringLiteral("wall_time_ms"),       wallTimeMs);
    if (peakMemoryBytes > 0)  cbor.insert(QStringLiteral("peak_memory_bytes"),  peakMemoryBytes);
    if (randomSeed >= 0)      cbor.insert(QStringLiteral("random_seed"),        randomSeed);

    return cbor;
}

//=============================================================================================================

MnaProvenance MnaProvenance::fromCbor(const QCborMap& cbor)
{
    MnaProvenance p;

    const QCborMap hashMap = cbor.value(QStringLiteral("input_hashes")).toMap();
    for (auto it = hashMap.constBegin(); it != hashMap.constEnd(); ++it) {
        p.inputHashes.insert(it.key().toString(), it.value().toString());
    }

    const QCborMap attrMap = cbor.value(QStringLiteral("resolved_attributes")).toMap();
    for (auto it = attrMap.constBegin(); it != attrMap.constEnd(); ++it) {
        p.resolvedAttributes.insert(it.key().toString(), it.value().toVariant());
    }

    p.mneCppVersion       = cbor.value(QStringLiteral("mne_cpp_version")).toString();
    p.qtVersion           = cbor.value(QStringLiteral("qt_version")).toString();
    p.compilerInfo        = cbor.value(QStringLiteral("compiler_info")).toString();
    p.osInfo              = cbor.value(QStringLiteral("os_info")).toString();
    p.hostName            = cbor.value(QStringLiteral("host_name")).toString();
    p.externalToolVersion = cbor.value(QStringLiteral("external_tool_version")).toString();

    p.startedAt       = QDateTime::fromString(cbor.value(QStringLiteral("started_at")).toString(), Qt::ISODateWithMs);
    p.finishedAt      = QDateTime::fromString(cbor.value(QStringLiteral("finished_at")).toString(), Qt::ISODateWithMs);
    p.wallTimeMs      = cbor.value(QStringLiteral("wall_time_ms")).toInteger(0);
    p.peakMemoryBytes = cbor.value(QStringLiteral("peak_memory_bytes")).toInteger(0);
    p.randomSeed      = cbor.value(QStringLiteral("random_seed")).toInteger(-1);

    return p;
}

//=============================================================================================================
// MnaVerification
//=============================================================================================================

QJsonObject MnaVerification::toJson() const
{
    QJsonObject json;

    if (!explanation.isEmpty()) {
        json[QStringLiteral("explanation")] = explanation;
    }

    if (!checks.isEmpty()) {
        QJsonArray arr;
        for (const MnaVerificationCheck& c : checks) {
            arr.append(c.toJson());
        }
        json[QStringLiteral("checks")] = arr;
    }

    if (!preResults.isEmpty()) {
        QJsonArray arr;
        for (const MnaVerificationResult& r : preResults) {
            arr.append(r.toJson());
        }
        json[QStringLiteral("pre_results")] = arr;
    }

    if (!postResults.isEmpty()) {
        QJsonArray arr;
        for (const MnaVerificationResult& r : postResults) {
            arr.append(r.toJson());
        }
        json[QStringLiteral("post_results")] = arr;
    }

    QJsonObject provObj = provenance.toJson();
    if (!provObj.isEmpty()) {
        json[QStringLiteral("provenance")] = provObj;
    }

    return json;
}

//=============================================================================================================

MnaVerification MnaVerification::fromJson(const QJsonObject& json)
{
    MnaVerification v;

    v.explanation = json.value(QStringLiteral("explanation")).toString();

    const QJsonArray checksArr = json.value(QStringLiteral("checks")).toArray();
    for (const QJsonValue& val : checksArr) {
        v.checks.append(MnaVerificationCheck::fromJson(val.toObject()));
    }

    const QJsonArray preArr = json.value(QStringLiteral("pre_results")).toArray();
    for (const QJsonValue& val : preArr) {
        v.preResults.append(MnaVerificationResult::fromJson(val.toObject()));
    }

    const QJsonArray postArr = json.value(QStringLiteral("post_results")).toArray();
    for (const QJsonValue& val : postArr) {
        v.postResults.append(MnaVerificationResult::fromJson(val.toObject()));
    }

    if (json.contains(QStringLiteral("provenance"))) {
        v.provenance = MnaProvenance::fromJson(json.value(QStringLiteral("provenance")).toObject());
    }

    return v;
}

//=============================================================================================================

QCborMap MnaVerification::toCbor() const
{
    QCborMap cbor;

    if (!explanation.isEmpty()) {
        cbor.insert(QStringLiteral("explanation"), explanation);
    }

    if (!checks.isEmpty()) {
        QCborArray arr;
        for (const MnaVerificationCheck& c : checks) {
            arr.append(c.toCbor());
        }
        cbor.insert(QStringLiteral("checks"), arr);
    }

    if (!preResults.isEmpty()) {
        QCborArray arr;
        for (const MnaVerificationResult& r : preResults) {
            arr.append(r.toCbor());
        }
        cbor.insert(QStringLiteral("pre_results"), arr);
    }

    if (!postResults.isEmpty()) {
        QCborArray arr;
        for (const MnaVerificationResult& r : postResults) {
            arr.append(r.toCbor());
        }
        cbor.insert(QStringLiteral("post_results"), arr);
    }

    QCborMap provMap = provenance.toCbor();
    if (!provMap.isEmpty()) {
        cbor.insert(QStringLiteral("provenance"), provMap);
    }

    return cbor;
}

//=============================================================================================================

MnaVerification MnaVerification::fromCbor(const QCborMap& cbor)
{
    MnaVerification v;

    v.explanation = cbor.value(QStringLiteral("explanation")).toString();

    const QCborArray checksArr = cbor.value(QStringLiteral("checks")).toArray();
    for (const QCborValue& val : checksArr) {
        v.checks.append(MnaVerificationCheck::fromCbor(val.toMap()));
    }

    const QCborArray preArr = cbor.value(QStringLiteral("pre_results")).toArray();
    for (const QCborValue& val : preArr) {
        v.preResults.append(MnaVerificationResult::fromCbor(val.toMap()));
    }

    const QCborArray postArr = cbor.value(QStringLiteral("post_results")).toArray();
    for (const QCborValue& val : postArr) {
        v.postResults.append(MnaVerificationResult::fromCbor(val.toMap()));
    }

    if (cbor.contains(QStringLiteral("provenance"))) {
        v.provenance = MnaProvenance::fromCbor(cbor.value(QStringLiteral("provenance")).toMap());
    }

    return v;
}
