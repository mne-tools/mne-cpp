//=============================================================================================================
/**
 * @file     mna_node.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MnaNode struct implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_node.h"

#include <QJsonArray>
#include <QCborArray>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

static QString execModeToString(MnaNodeExecMode mode)
{
    switch (mode) {
    case MnaNodeExecMode::Batch:  return QStringLiteral("batch");
    case MnaNodeExecMode::Stream: return QStringLiteral("stream");
    case MnaNodeExecMode::Ipc:    return QStringLiteral("ipc");
    case MnaNodeExecMode::Script: return QStringLiteral("script");
    }
    return QStringLiteral("batch");
}

//=============================================================================================================

static MnaNodeExecMode execModeFromString(const QString& str)
{
    if (str == QLatin1String("stream")) return MnaNodeExecMode::Stream;
    if (str == QLatin1String("ipc"))    return MnaNodeExecMode::Ipc;
    if (str == QLatin1String("script")) return MnaNodeExecMode::Script;
    return MnaNodeExecMode::Batch;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QJsonObject MnaNode::toJson() const
{
    QJsonObject json;
    json[QLatin1String("id")]        = id;
    json[QLatin1String("op_type")]   = opType;
    json[QLatin1String("exec_mode")] = execModeToString(execMode);

    // Attributes
    json[QLatin1String("attributes")] = QJsonObject::fromVariantMap(attributes);

    // Ports
    QJsonArray inputArr;
    for (const MnaPort& p : inputs)
        inputArr.append(p.toJson());
    json[QLatin1String("inputs")] = inputArr;

    QJsonArray outputArr;
    for (const MnaPort& p : outputs)
        outputArr.append(p.toJson());
    json[QLatin1String("outputs")] = outputArr;

    // IPC config
    if (execMode == MnaNodeExecMode::Ipc) {
        json[QLatin1String("ipc_command")]   = ipcCommand;
        QJsonArray argsArr;
        for (const QString& arg : ipcArgs)
            argsArr.append(arg);
        json[QLatin1String("ipc_args")]      = argsArr;
        json[QLatin1String("ipc_work_dir")]  = ipcWorkDir;
        json[QLatin1String("ipc_transport")] = ipcTransport;
    }

    // Script config
    if (execMode == MnaNodeExecMode::Script) {
        json[QLatin1String("script")] = script.toJson();
    }

    // Verification & provenance
    QJsonObject verObj = verification.toJson();
    if (!verObj.isEmpty()) {
        json[QLatin1String("verification")] = verObj;
    }

    // Metadata
    if (!toolVersion.isEmpty())
        json[QLatin1String("tool_version")] = toolVersion;
    if (executedAt.isValid())
        json[QLatin1String("executed_at")] = executedAt.toString(Qt::ISODate);
    json[QLatin1String("dirty")] = dirty;

    return json;
}

//=============================================================================================================

MnaNode MnaNode::fromJson(const QJsonObject& json)
{
    MnaNode node;
    node.id       = json[QLatin1String("id")].toString();
    node.opType   = json[QLatin1String("op_type")].toString();
    node.execMode = execModeFromString(json[QLatin1String("exec_mode")].toString());

    node.attributes = json[QLatin1String("attributes")].toObject().toVariantMap();

    // Ports
    QJsonArray inputArr = json[QLatin1String("inputs")].toArray();
    for (const QJsonValue& v : inputArr)
        node.inputs.append(MnaPort::fromJson(v.toObject()));

    QJsonArray outputArr = json[QLatin1String("outputs")].toArray();
    for (const QJsonValue& v : outputArr)
        node.outputs.append(MnaPort::fromJson(v.toObject()));

    // IPC
    node.ipcCommand   = json[QLatin1String("ipc_command")].toString();
    QJsonArray argsArr = json[QLatin1String("ipc_args")].toArray();
    for (const QJsonValue& v : argsArr)
        node.ipcArgs.append(v.toString());
    node.ipcWorkDir   = json[QLatin1String("ipc_work_dir")].toString();
    node.ipcTransport = json[QLatin1String("ipc_transport")].toString();

    // Script
    if (json.contains(QLatin1String("script"))) {
        node.script = MnaScript::fromJson(json[QLatin1String("script")].toObject());
    }

    // Verification
    if (json.contains(QLatin1String("verification"))) {
        node.verification = MnaVerification::fromJson(json[QLatin1String("verification")].toObject());
    }

    // Metadata
    node.toolVersion = json[QLatin1String("tool_version")].toString();
    QString execStr  = json[QLatin1String("executed_at")].toString();
    if (!execStr.isEmpty())
        node.executedAt = QDateTime::fromString(execStr, Qt::ISODate);
    node.dirty = json[QLatin1String("dirty")].toBool(true);

    return node;
}

//=============================================================================================================

QCborMap MnaNode::toCbor() const
{
    QCborMap cbor;
    cbor[QLatin1String("id")]        = id;
    cbor[QLatin1String("op_type")]   = opType;
    cbor[QLatin1String("exec_mode")] = execModeToString(execMode);

    cbor[QLatin1String("attributes")] = QCborMap::fromJsonObject(QJsonObject::fromVariantMap(attributes));

    QCborArray inputArr;
    for (const MnaPort& p : inputs)
        inputArr.append(p.toCbor());
    cbor[QLatin1String("inputs")] = inputArr;

    QCborArray outputArr;
    for (const MnaPort& p : outputs)
        outputArr.append(p.toCbor());
    cbor[QLatin1String("outputs")] = outputArr;

    if (execMode == MnaNodeExecMode::Ipc) {
        cbor[QLatin1String("ipc_command")]   = ipcCommand;
        QCborArray argsArr;
        for (const QString& arg : ipcArgs)
            argsArr.append(arg);
        cbor[QLatin1String("ipc_args")]      = argsArr;
        cbor[QLatin1String("ipc_work_dir")]  = ipcWorkDir;
        cbor[QLatin1String("ipc_transport")] = ipcTransport;
    }

    if (execMode == MnaNodeExecMode::Script) {
        cbor[QLatin1String("script")] = script.toCbor();
    }

    QCborMap verCbor = verification.toCbor();
    if (!verCbor.isEmpty()) {
        cbor[QLatin1String("verification")] = verCbor;
    }

    if (!toolVersion.isEmpty())
        cbor[QLatin1String("tool_version")] = toolVersion;
    if (executedAt.isValid())
        cbor[QLatin1String("executed_at")] = executedAt.toString(Qt::ISODate);
    cbor[QLatin1String("dirty")] = dirty;

    return cbor;
}

//=============================================================================================================

MnaNode MnaNode::fromCbor(const QCborMap& cbor)
{
    MnaNode node;
    node.id       = cbor[QLatin1String("id")].toString();
    node.opType   = cbor[QLatin1String("op_type")].toString();
    node.execMode = execModeFromString(cbor[QLatin1String("exec_mode")].toString());

    QCborMap attrCbor = cbor[QLatin1String("attributes")].toMap();
    node.attributes = attrCbor.toJsonObject().toVariantMap();

    QCborArray inputArr = cbor[QLatin1String("inputs")].toArray();
    for (const QCborValue& v : inputArr)
        node.inputs.append(MnaPort::fromCbor(v.toMap()));

    QCborArray outputArr = cbor[QLatin1String("outputs")].toArray();
    for (const QCborValue& v : outputArr)
        node.outputs.append(MnaPort::fromCbor(v.toMap()));

    node.ipcCommand   = cbor[QLatin1String("ipc_command")].toString();
    QCborArray argsArr = cbor[QLatin1String("ipc_args")].toArray();
    for (const QCborValue& v : argsArr)
        node.ipcArgs.append(v.toString());
    node.ipcWorkDir   = cbor[QLatin1String("ipc_work_dir")].toString();
    node.ipcTransport = cbor[QLatin1String("ipc_transport")].toString();

    if (cbor.contains(QLatin1String("script"))) {
        node.script = MnaScript::fromCbor(cbor[QLatin1String("script")].toMap());
    }
    if (cbor.contains(QLatin1String("verification"))) {
        node.verification = MnaVerification::fromCbor(cbor[QLatin1String("verification")].toMap());
    }

    node.toolVersion = cbor[QLatin1String("tool_version")].toString();
    QString execStr  = cbor[QLatin1String("executed_at")].toString();
    if (!execStr.isEmpty())
        node.executedAt = QDateTime::fromString(execStr, Qt::ISODate);
    node.dirty = cbor[QLatin1String("dirty")].toBool(true);

    return node;
}
