//=============================================================================================================
/**
 * @file     pipelineparser.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the .mne parser and validator.
 */

#include "pipelineparser.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonParseError>

using namespace MNEANALYZESTUDIO;

namespace
{

QString describeEntry(const QString& sourceName, const QString& sectionName, int index)
{
    return QStringLiteral("%1 %2[%3]").arg(sourceName, sectionName).arg(index);
}

QJsonObject ensureObject(const QJsonValue& value, const QString& fieldName, const QString& context)
{
    if(!value.isObject()) {
        throw WorkflowValidationError(QStringLiteral("%1 must define `%2` as an object.").arg(context, fieldName));
    }

    return value.toObject();
}

}

PipelineParser::PipelineParser(QObject* parent)
: QObject(parent)
{
}

WorkflowGraph PipelineParser::parseFile(const QString& filePath) const
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw WorkflowValidationError(QStringLiteral("Could not open workflow file `%1`.").arg(filePath));
    }

    return parseJson(file.readAll(), filePath);
}

WorkflowGraph PipelineParser::parseJson(const QByteArray& jsonPayload, const QString& sourceName) const
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(jsonPayload, &parseError);
    if(parseError.error != QJsonParseError::NoError || !document.isObject()) {
        throw WorkflowValidationError(QStringLiteral("Invalid .mne JSON in `%1`: %2")
                                          .arg(sourceName, parseError.errorString()));
    }

    return parseDocument(document, sourceName);
}

WorkflowGraph PipelineParser::parseDocument(const QJsonDocument& document, const QString& sourceName) const
{
    if(!document.isObject()) {
        throw WorkflowValidationError(QStringLiteral("Workflow document `%1` must be a JSON object.").arg(sourceName));
    }

    const QJsonObject root = document.object();
    const QJsonArray resources = root.value(QStringLiteral("resources")).toArray();
    const QJsonArray pipeline = root.value(QStringLiteral("pipeline")).toArray();

    WorkflowGraph graph;
    for(int i = 0; i < resources.size(); ++i) {
        graph.addResource(parseResource(resources.at(i).toObject(), i, sourceName));
    }

    for(int i = 0; i < pipeline.size(); ++i) {
        graph.addNode(parseNode(pipeline.at(i).toObject(), i, sourceName));
    }

    graph.validateReferences();
    graph.topologicalSort();

    return graph;
}

WorkflowResource PipelineParser::parseResource(const QJsonObject& object, int index, const QString& sourceName) const
{
    const QString context = describeEntry(sourceName, QStringLiteral("resources"), index);

    WorkflowResource resource;
    resource.uid = object.value(QStringLiteral("uid")).toString().trimmed();
    resource.type = object.value(QStringLiteral("type")).toString().trimmed();
    resource.uri = object.value(QStringLiteral("uri")).toString().trimmed();
    resource.metadata = object.value(QStringLiteral("metadata")).toObject();

    if(resource.uid.isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("%1 is missing `uid`.").arg(context));
    }

    if(resource.type.isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("%1 is missing `type`.").arg(context));
    }

    if(resource.uri.isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("%1 is missing `uri`.").arg(context));
    }

    return resource;
}

WorkflowNode PipelineParser::parseNode(const QJsonObject& object, int index, const QString& sourceName) const
{
    const QString context = describeEntry(sourceName, QStringLiteral("pipeline"), index);

    WorkflowNode node;
    node.uid = object.value(QStringLiteral("uid")).toString().trimmed();
    node.skillId = object.value(QStringLiteral("skill_id")).toString().trimmed();
    node.label = object.value(QStringLiteral("label")).toString().trimmed();
    node.stage = object.value(QStringLiteral("stage")).toString().trimmed();
    node.description = object.value(QStringLiteral("description")).toString().trimmed();
    node.inputs = ensureObject(object.value(QStringLiteral("inputs")), QStringLiteral("inputs"), context);
    node.parameters = object.value(QStringLiteral("parameters")).toObject();
    node.outputs = ensureObject(object.value(QStringLiteral("outputs")), QStringLiteral("outputs"), context);

    if(node.uid.isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("%1 is missing `uid`.").arg(context));
    }

    if(node.skillId.isEmpty()) {
        throw WorkflowValidationError(QStringLiteral("%1 is missing `skill_id`.").arg(context));
    }

    return node;
}
