//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *

 * @file     writemnaskill.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    WriteMnaSkill implementation.
 */

#include "writemnaskill.h"

#include <workflowgraph.h>

#include <mne/mna/mna_io.h>
#include <mne/mna/mna_project.h>

#include <mne/base/string_utils.h>

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

#include <chrono>

using namespace MNEANALYZESTUDIO;
using namespace mne;

namespace
{

QJsonObject objectSchema(const QJsonObject& properties, const QJsonArray& required = QJsonArray())
{
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}};
}

QString resolveUri(const QString& uri)
{
    return ISkillOperator::localPathFromUri(uri);
}

} // namespace

WriteMnaSkill::WriteMnaSkill(QObject* parent)
: ISkillOperator(parent)
{
}

QJsonObject WriteMnaSkill::getOperatorDefinition() const
{
    return QJsonObject{
        {"skill_id", "mne.skills.write_mna"},
        {"tool_name", "write_mna_project"},
        {"display_name", "Write MNA Project"},
        {"description", "Persist a (possibly newly created) MNA project to disk as .mna (JSON) or .mnx (CBOR)."},
        {"extension_id", "mna-skills"},
        {"extension_display_name", "MNA Project Skills"},
        {"inputs_schema", objectSchema(QJsonObject{{"source_project", QJsonObject{{"type", "string"}, {"title", "Source project path"}, {"description", "Optional input project to round-trip; if omitted a new empty project is created."}}}})},
        {"parameters_schema", objectSchema(QJsonObject{{"output_path", QJsonObject{{"type", "string"}, {"title", "Output path"}}}, {"format", QJsonObject{{"type", "string"}, {"title", "Format"}, {"enum", QJsonArray{"mna", "mnx", "auto"}}, {"default", "auto"}}}, {"project_name", QJsonObject{{"type", "string"}, {"title", "Project name (when creating new)"}}}}, QJsonArray{"output_path"})},
        {"outputs_schema", objectSchema(QJsonObject{{"output_path", QJsonObject{{"type", "string"}, {"title", "Written file path"}}}}, QJsonArray{"output_path"})}};
}

QJsonObject WriteMnaSkill::executeSkill(const WorkflowNode& nodeState)
{
    const QString outputPath = nodeState.parameters.value("output_path").toString();
    if (outputPath.isEmpty()) {
        return QJsonObject{
            {"status", "error"},
            {"message", QStringLiteral("Write MNA node `%1`: missing `output_path` parameter.").arg(nodeState.uid)}};
    }

    MnaProject project;

    const QJsonObject sourceInput = nodeState.resolvedInputs.value("source_project").toObject();
    QString sourceUri = sourceInput.value("uri").toString();
    if (sourceUri.isEmpty()) {
        sourceUri = nodeState.resolvedInputs.value("source_project").toString();
    }
    if (!sourceUri.isEmpty()) {
        const QString path = resolveUri(sourceUri);
        if (!QFileInfo::exists(path)) {
            return QJsonObject{
                {"status", "error"},
                {"message", QStringLiteral("Write MNA node `%1`: source `%2` not found.").arg(nodeState.uid, path)}};
        }
        Expected<MnaProject> read = MnaIO::read(path_from_utf8(path.toStdString()));
        if (!read) {
            return QJsonObject{
                {"status", "error"},
                {"message", QStringLiteral("Write MNA node `%1`: %2").arg(nodeState.uid, QString::fromUtf8(read.error().what()))}};
        }
        project = std::move(*read);
    } else {
        project.name = nodeState.parameters.value("project_name").toString(QStringLiteral("Untitled MNA Project")).toStdString();
    }

    project.modified = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());

    // MnaIO::write dispatches by file suffix (.mna -> JSON, .mnx -> CBOR).
    // The optional "format" parameter is honoured by ensuring the output
    // path carries the matching extension.
    const QString format = nodeState.parameters.value("format").toString(QStringLiteral("auto"));
    QString writePath = outputPath;
    if (format == QLatin1String("mna") && !writePath.endsWith(QLatin1String(".mna"), Qt::CaseInsensitive)) {
        writePath += QLatin1String(".mna");
    } else if (format == QLatin1String("mnx") && !writePath.endsWith(QLatin1String(".mnx"), Qt::CaseInsensitive)) {
        writePath += QLatin1String(".mnx");
    }
    if (const Expected<void> written = MnaIO::write(project, path_from_utf8(writePath.toStdString())); !written) {
        return QJsonObject{
            {"status", "error"},
            {"message", QStringLiteral("Write MNA node `%1`: %2").arg(nodeState.uid, QString::fromUtf8(written.error().what()))}};
    }

    return QJsonObject{
        {"status", "completed"},
        {"message", QStringLiteral("Wrote MNA project to `%1`.").arg(writePath)},
        {"outputs", QJsonObject{{"output_path", writePath}}}};
}
