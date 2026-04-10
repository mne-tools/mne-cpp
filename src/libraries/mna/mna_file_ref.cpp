//=============================================================================================================
/**
 * @file     mna_file_ref.cpp
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
 * @brief    MnaFileRef class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_file_ref.h"

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

QJsonObject MnaFileRef::toJson() const
{
    QJsonObject json;
    json[QLatin1String("role")]       = mnaFileRoleToString(role);
    json[QLatin1String("path")]       = path;
    json[QLatin1String("sha256")]     = sha256;
    json[QLatin1String("format")]     = format;
    json[QLatin1String("size_bytes")] = sizeBytes;
    json[QLatin1String("embedded")]   = embedded;
    if(embedded && !data.isEmpty()) {
        json[QLatin1String("data")] = QString::fromLatin1(data.toBase64());
    }
    return json;
}

//=============================================================================================================

MnaFileRef MnaFileRef::fromJson(const QJsonObject& json)
{
    MnaFileRef ref;
    ref.role      = mnaFileRoleFromString(json[QLatin1String("role")].toString());
    ref.path      = json[QLatin1String("path")].toString();
    ref.sha256    = json[QLatin1String("sha256")].toString();
    ref.format    = json[QLatin1String("format")].toString();
    ref.sizeBytes = static_cast<qint64>(json[QLatin1String("size_bytes")].toDouble());
    ref.embedded  = json[QLatin1String("embedded")].toBool();
    if(ref.embedded) {
        ref.data = QByteArray::fromBase64(json[QLatin1String("data")].toString().toLatin1());
    }
    return ref;
}

//=============================================================================================================

QCborMap MnaFileRef::toCbor() const
{
    QCborMap cbor;
    cbor[QLatin1String("role")]       = mnaFileRoleToString(role);
    cbor[QLatin1String("path")]       = path;
    cbor[QLatin1String("sha256")]     = sha256;
    cbor[QLatin1String("format")]     = format;
    cbor[QLatin1String("size_bytes")] = sizeBytes;
    cbor[QLatin1String("embedded")]   = embedded;
    if(embedded && !data.isEmpty()) {
        cbor[QLatin1String("data")] = QCborValue(data);
    }
    return cbor;
}

//=============================================================================================================

MnaFileRef MnaFileRef::fromCbor(const QCborMap& cbor)
{
    MnaFileRef ref;
    ref.role      = mnaFileRoleFromString(cbor[QLatin1String("role")].toString());
    ref.path      = cbor[QLatin1String("path")].toString();
    ref.sha256    = cbor[QLatin1String("sha256")].toString();
    ref.format    = cbor[QLatin1String("format")].toString();
    ref.sizeBytes = cbor[QLatin1String("size_bytes")].toInteger();
    ref.embedded  = cbor[QLatin1String("embedded")].toBool();
    if(ref.embedded) {
        ref.data = cbor[QLatin1String("data")].toByteArray();
    }
    return ref;
}
