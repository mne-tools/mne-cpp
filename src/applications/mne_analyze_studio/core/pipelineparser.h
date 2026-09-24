//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     pipelineparser.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares the .mne parser and validator.
 */

#ifndef MNE_ANALYZE_STUDIO_PIPELINEPARSER_H
#define MNE_ANALYZE_STUDIO_PIPELINEPARSER_H

#include "studio_core_global.h"
#include "workflowgraph.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QObject>
#include <QString>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Parser that converts .mne JSON files into validated workflow DAGs.
 */
class STUDIOCORESHARED_EXPORT PipelineParser : public QObject
{
    Q_OBJECT

public:
    explicit PipelineParser(QObject* parent = nullptr);

    WorkflowGraph parseFile(const QString& filePath) const;
    WorkflowGraph parseJson(const QByteArray& jsonPayload,
                            const QString& sourceName = QStringLiteral("<memory>")) const;
    WorkflowGraph parseDocument(const QJsonDocument& document,
                                const QString& sourceName = QStringLiteral("<document>")) const;

private:
    WorkflowResource parseResource(const QJsonObject& object, int index, const QString& sourceName) const;
    WorkflowNode parseNode(const QJsonObject& object, int index, const QString& sourceName) const;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_PIPELINEPARSER_H
