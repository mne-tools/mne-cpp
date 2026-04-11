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
