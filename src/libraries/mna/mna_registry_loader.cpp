//=============================================================================================================
/**
 * @file     mna_registry_loader.cpp
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
 * @brief    MnaRegistryLoader class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_registry_loader.h"
#include "mna_op_registry.h"
#include "mna_op_schema.h"
#include "mna_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

static QMetaType::Type metaTypeFromString(const QString& str)
{
    if (str == QLatin1String("string"))  return QMetaType::QString;
    if (str == QLatin1String("double"))  return QMetaType::Double;
    if (str == QLatin1String("float"))   return QMetaType::Float;
    if (str == QLatin1String("int"))     return QMetaType::Int;
    if (str == QLatin1String("bool"))    return QMetaType::Bool;
    if (str == QLatin1String("path"))    return QMetaType::QString;
    return QMetaType::QString;
}

//=============================================================================================================

static QString metaTypeToString(QMetaType::Type type)
{
    switch (type) {
    case QMetaType::QString:  return QStringLiteral("string");
    case QMetaType::Double:   return QStringLiteral("double");
    case QMetaType::Float:    return QStringLiteral("float");
    case QMetaType::Int:      return QStringLiteral("int");
    case QMetaType::Bool:     return QStringLiteral("bool");
    default:                  return QStringLiteral("string");
    }
}

//=============================================================================================================

static MnaOpSchemaPort parsePort(const QJsonObject& obj)
{
    MnaOpSchemaPort port;
    port.name        = obj.value(QLatin1String("name")).toString();
    port.dataKind    = mnaDataKindFromString(obj.value(QLatin1String("kind")).toString());
    port.required    = obj.value(QLatin1String("required")).toBool(true);
    port.description = obj.value(QLatin1String("description")).toString();
    return port;
}

//=============================================================================================================

static QJsonObject portToJson(const MnaOpSchemaPort& port)
{
    QJsonObject obj;
    obj[QLatin1String("name")]        = port.name;
    obj[QLatin1String("kind")]        = mnaDataKindToString(port.dataKind);
    obj[QLatin1String("description")] = port.description;
    if (!port.required)
        obj[QLatin1String("required")] = false;
    return obj;
}

//=============================================================================================================

static MnaOpSchemaAttr parseAttr(const QJsonObject& obj)
{
    MnaOpSchemaAttr attr;
    attr.name         = obj.value(QLatin1String("name")).toString();
    attr.type         = metaTypeFromString(obj.value(QLatin1String("type")).toString());
    attr.required     = obj.value(QLatin1String("required")).toBool(false);
    attr.description  = obj.value(QLatin1String("description")).toString();
    if (obj.contains(QLatin1String("default")))
        attr.defaultValue = obj.value(QLatin1String("default")).toVariant();
    return attr;
}

//=============================================================================================================

static QJsonObject attrToJson(const MnaOpSchemaAttr& attr)
{
    QJsonObject obj;
    obj[QLatin1String("name")] = attr.name;
    obj[QLatin1String("type")] = metaTypeToString(attr.type);
    if (attr.required)
        obj[QLatin1String("required")] = true;
    if (!attr.description.isEmpty())
        obj[QLatin1String("description")] = attr.description;
    if (attr.defaultValue.isValid())
        obj[QLatin1String("default")] = QJsonValue::fromVariant(attr.defaultValue);
    return obj;
}

//=============================================================================================================

static MnaOpSchema parseOp(const QJsonObject& obj)
{
    MnaOpSchema schema;
    schema.opType      = obj.value(QLatin1String("type")).toString();
    schema.version     = obj.value(QLatin1String("version")).toString();
    schema.binding     = obj.value(QLatin1String("binding")).toString(QStringLiteral("internal"));
    schema.category    = obj.value(QLatin1String("category")).toString();
    schema.description = obj.value(QLatin1String("description")).toString();
    schema.library     = obj.value(QLatin1String("library")).toString();
    schema.executable  = obj.value(QLatin1String("executable")).toString();
    schema.cliTemplate = obj.value(QLatin1String("cli_template")).toString();

    const QJsonArray inputs = obj.value(QLatin1String("inputs")).toArray();
    for (const QJsonValue& v : inputs)
        schema.inputPorts.append(parsePort(v.toObject()));

    const QJsonArray outputs = obj.value(QLatin1String("outputs")).toArray();
    for (const QJsonValue& v : outputs)
        schema.outputPorts.append(parsePort(v.toObject()));

    const QJsonArray params = obj.value(QLatin1String("parameters")).toArray();
    for (const QJsonValue& v : params)
        schema.attributes.append(parseAttr(v.toObject()));

    return schema;
}

//=============================================================================================================

static QJsonObject opToJson(const MnaOpSchema& schema)
{
    QJsonObject obj;
    obj[QLatin1String("type")]        = schema.opType;
    obj[QLatin1String("description")] = schema.description;

    if (!schema.version.isEmpty())
        obj[QLatin1String("version")] = schema.version;
    if (!schema.binding.isEmpty())
        obj[QLatin1String("binding")] = schema.binding;
    if (!schema.category.isEmpty())
        obj[QLatin1String("category")] = schema.category;
    if (!schema.library.isEmpty())
        obj[QLatin1String("library")] = schema.library;
    if (!schema.executable.isEmpty())
        obj[QLatin1String("executable")] = schema.executable;
    if (!schema.cliTemplate.isEmpty())
        obj[QLatin1String("cli_template")] = schema.cliTemplate;

    QJsonArray inputs;
    for (const MnaOpSchemaPort& p : schema.inputPorts)
        inputs.append(portToJson(p));
    obj[QLatin1String("inputs")] = inputs;

    QJsonArray outputs;
    for (const MnaOpSchemaPort& p : schema.outputPorts)
        outputs.append(portToJson(p));
    obj[QLatin1String("outputs")] = outputs;

    QJsonArray params;
    for (const MnaOpSchemaAttr& a : schema.attributes)
        params.append(attrToJson(a));
    obj[QLatin1String("parameters")] = params;

    return obj;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

int MnaRegistryLoader::loadFile(const QString& path, MnaOpRegistry& registry)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "MnaRegistryLoader: Cannot open" << path;
        return -1;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "MnaRegistryLoader: JSON parse error in" << path
                    << ":" << parseError.errorString();
        return -1;
    }

    const QJsonObject root = doc.object();

    // Validate required fields
    if (!root.contains(QLatin1String("mna_registry_version"))) {
        qWarning() << "MnaRegistryLoader: Missing 'mna_registry_version' in" << path;
        return -1;
    }

    const QJsonArray ops = root.value(QLatin1String("ops")).toArray();
    int count = 0;

    for (const QJsonValue& v : ops) {
        MnaOpSchema schema = parseOp(v.toObject());
        if (!schema.opType.isEmpty()) {
            registry.registerOp(schema);
            ++count;
        }
    }

    qInfo() << "MnaRegistryLoader: Loaded" << count << "ops from" << path
            << "(provider:" << root.value(QLatin1String("provider")).toString() << ")";

    return count;
}

//=============================================================================================================

int MnaRegistryLoader::loadDirectory(const QString& registryDir, MnaOpRegistry& registry)
{
    int total = 0;

    // 1. Load master manifest
    QString masterPath = registryDir + QStringLiteral("/mna-registry.json");
    if (QFile::exists(masterPath)) {
        int n = loadFile(masterPath, registry);
        if (n > 0) total += n;
    }

    // 2. Load drop-in files from mna-registry.d/
    QDir dropInDir(registryDir + QStringLiteral("/mna-registry.d"));
    if (dropInDir.exists()) {
        QStringList filters;
        filters << QStringLiteral("*.json");
        QStringList files = dropInDir.entryList(filters, QDir::Files, QDir::Name);
        for (const QString& fileName : files) {
            int n = loadFile(dropInDir.absoluteFilePath(fileName), registry);
            if (n > 0) total += n;
        }
    }

    return total;
}

//=============================================================================================================

bool MnaRegistryLoader::saveFile(const QString& path,
                                  const QString& provider,
                                  const MnaOpRegistry& registry)
{
    QJsonObject root;
    root[QLatin1String("mna_registry_version")] = QStringLiteral("1.0");
    root[QLatin1String("provider")]             = provider;

    QJsonArray ops;
    const QStringList opTypes = registry.registeredOps();
    for (const QString& opType : opTypes)
        ops.append(opToJson(registry.schema(opType)));
    root[QLatin1String("ops")] = ops;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "MnaRegistryLoader: Cannot write" << path;
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
