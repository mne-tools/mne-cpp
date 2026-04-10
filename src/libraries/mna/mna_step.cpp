//=============================================================================================================
/**
 * @file     mna_step.cpp
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
 * @brief    MnaStep class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_step.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QCborValue>
#include <QCborArray>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QJsonObject MnaStep::toJson() const
{
    QJsonObject json;
    json[QLatin1String("id")]           = id;
    json[QLatin1String("tool")]         = tool;
    json[QLatin1String("tool_version")] = toolVersion;
    json[QLatin1String("parameters")]   = QJsonObject::fromVariantMap(parameters);
    json[QLatin1String("inputs")]       = QJsonArray::fromStringList(inputs);
    json[QLatin1String("outputs")]      = QJsonArray::fromStringList(outputs);
    json[QLatin1String("timestamp")]    = timestamp.toString(Qt::ISODate);
    return json;
}

//=============================================================================================================

MnaStep MnaStep::fromJson(const QJsonObject& json)
{
    MnaStep step;
    step.id          = json[QLatin1String("id")].toString();
    step.tool        = json[QLatin1String("tool")].toString();
    step.toolVersion = json[QLatin1String("tool_version")].toString();
    step.parameters  = json[QLatin1String("parameters")].toObject().toVariantMap();
    step.timestamp   = QDateTime::fromString(json[QLatin1String("timestamp")].toString(), Qt::ISODate);

    const QJsonArray inputsArr = json[QLatin1String("inputs")].toArray();
    for(const QJsonValue& v : inputsArr) {
        step.inputs.append(v.toString());
    }

    const QJsonArray outputsArr = json[QLatin1String("outputs")].toArray();
    for(const QJsonValue& v : outputsArr) {
        step.outputs.append(v.toString());
    }

    return step;
}

//=============================================================================================================

QCborMap MnaStep::toCbor() const
{
    QCborMap cbor;
    cbor[QLatin1String("id")]           = id;
    cbor[QLatin1String("tool")]         = tool;
    cbor[QLatin1String("tool_version")] = toolVersion;
    cbor[QLatin1String("parameters")]   = QCborMap::fromVariantMap(parameters);
    cbor[QLatin1String("timestamp")]    = QCborValue(timestamp);

    QCborArray inputsArr;
    for(const QString& s : inputs) {
        inputsArr.append(s);
    }
    cbor[QLatin1String("inputs")] = inputsArr;

    QCborArray outputsArr;
    for(const QString& s : outputs) {
        outputsArr.append(s);
    }
    cbor[QLatin1String("outputs")] = outputsArr;

    return cbor;
}

//=============================================================================================================

MnaStep MnaStep::fromCbor(const QCborMap& cbor)
{
    MnaStep step;
    step.id          = cbor[QLatin1String("id")].toString();
    step.tool        = cbor[QLatin1String("tool")].toString();
    step.toolVersion = cbor[QLatin1String("tool_version")].toString();
    step.parameters  = cbor[QLatin1String("parameters")].toMap().toVariantMap();
    step.timestamp   = cbor[QLatin1String("timestamp")].toDateTime();

    const QCborArray inputsArr = cbor[QLatin1String("inputs")].toArray();
    for(const QCborValue& v : inputsArr) {
        step.inputs.append(v.toString());
    }

    const QCborArray outputsArr = cbor[QLatin1String("outputs")].toArray();
    for(const QCborValue& v : outputsArr) {
        step.outputs.append(v.toString());
    }

    return step;
}
