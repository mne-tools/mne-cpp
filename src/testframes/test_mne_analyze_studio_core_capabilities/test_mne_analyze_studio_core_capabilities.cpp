//=============================================================================================================
/**
 * @file     test_mne_analyze_studio_core_capabilities.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Tests shared Studio capability metadata and extension pipeline validation behavior.
 */

#include <capabilitycatalog.h>
#include <capabilityutils.h>
#include <viewproviderregistry.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

using namespace MNEANALYZESTUDIO;

namespace
{

bool jsonArrayContainsString(const QJsonArray& values, const QString& expected)
{
    for(const QJsonValue& value : values) {
        if(value.toString() == expected) {
            return true;
        }
    }

    return false;
}

QJsonObject objectSchema(const QJsonObject& properties = QJsonObject(),
                         const QJsonArray& required = QJsonArray())
{
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

bool writeJsonFile(const QString& filePath, const QJsonObject& document)
{
    const QFileInfo info(filePath);
    if(!QDir().mkpath(info.absolutePath())) {
        return false;
    }

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(document).toJson(QJsonDocument::Indented));
    return file.flush();
}

QJsonObject pipelineManifest(const QString& extensionId,
                             const QString& pipelineId,
                             const QString& displayName)
{
    QJsonArray requiredInputs;
    requiredInputs.append(QStringLiteral("subject_id"));

    return QJsonObject{
        {"id", extensionId},
        {"display_name", displayName},
        {"contributes", QJsonObject{
             {"analysis_pipelines", QJsonArray{
                  QJsonObject{
                      {"id", pipelineId},
                      {"display_name", QStringLiteral("Duplicate Pipeline")},
                      {"description", QStringLiteral("Fixture pipeline for registry duplicate tests.")},
                      {"input_schema", objectSchema(QJsonObject{
                           {"subject_id", QJsonObject{
                                {"type", "string"},
                                {"title", "Subject"}
                            }}
                       }, requiredInputs)},
                      {"output_schema", objectSchema(QJsonObject{
                           {"report_path", QJsonObject{
                                {"type", "string"},
                                {"title", "Report Path"}
                            }}
                       })},
                      {"steps", QJsonArray()}
                  }
              }}
         }}
    };
}

} // namespace

class TestMneAnalyzeStudioCoreCapabilities : public QObject
{
    Q_OBJECT

private slots:
    void testPipelineAliasRoundTrip();
    void testCapabilityCatalogLookupAndSharedAliases();
    void testAnnotateWorkflowCapability();
    void testAnnotateNeuroKernelAndPipelineCapabilities();
    void testRegistryRejectsDuplicatePipelineIds();
    void testRegistryBuildsPipelineToolDefinitions();
};

void TestMneAnalyzeStudioCoreCapabilities::testPipelineAliasRoundTrip()
{
    const QString pipelineId = QStringLiteral("analysis.find_peak");
    const QString aliasToolName = pipelineRunAliasToolName(pipelineId);

    QCOMPARE(aliasToolName, QStringLiteral("studio.pipeline.run::analysis.find_peak"));
    QCOMPARE(pipelineIdFromPipelineRunAliasToolName(aliasToolName), pipelineId);
    QCOMPARE(normalizedPlannerToolName(aliasToolName), QStringLiteral("studio.pipeline.run"));
    QCOMPARE(normalizedPlannerToolName(QStringLiteral(" neurokernel.raw_stats ")),
             QStringLiteral("neurokernel.raw_stats"));
}

void TestMneAnalyzeStudioCoreCapabilities::testCapabilityCatalogLookupAndSharedAliases()
{
    const QString pipelineId = QStringLiteral("analysis.psd_summary");
    const QString pipelineToolName = pipelineRunAliasToolName(pipelineId);

    QVector<CapabilityCatalogSource> sources;
    sources.append(CapabilityCatalogSource{
        QStringLiteral("workbench_local"),
        QStringLiteral("Workbench Tools"),
        QJsonArray{
            QJsonObject{
                {"name", "studio.pipeline.run"},
                {"description", "Generic pipeline runner."}
            }
        }
    });
    sources.append(CapabilityCatalogSource{
        QStringLiteral("analysis_pipeline"),
        QStringLiteral("Analysis Pipelines"),
        QJsonArray{
            QJsonObject{
                {"name", pipelineToolName},
                {"pipeline_id", pipelineId},
                {"description", "Run the PSD summary pipeline."},
                {"capability_aliases", QJsonArray{QStringLiteral("studio.pipeline.run")}}
            }
        }
    });

    QStringList warnings;
    const QJsonArray catalog = buildCapabilityCatalog(sources, &warnings);
    QCOMPARE(catalog.size(), 2);
    QVERIFY(warnings.isEmpty());

    const QJsonObject genericRunner = capabilityFromCatalog(catalog, QStringLiteral("studio.pipeline.run"));
    QCOMPARE(genericRunner.value("name").toString(), QStringLiteral("studio.pipeline.run"));
    QCOMPARE(genericRunner.value("capability_source_id").toString(), QStringLiteral("workbench_local"));

    const QJsonObject pipelineTool = capabilityFromCatalog(catalog, QStringLiteral("pipeline:analysis.psd_summary"));
    QCOMPARE(pipelineTool.value("name").toString(), pipelineToolName);
    QCOMPARE(pipelineTool.value("capability_source_id").toString(), QStringLiteral("analysis_pipeline"));
    QVERIFY(jsonArrayContainsString(pipelineTool.value("capability_lookup_aliases").toArray(), pipelineId));
    QVERIFY(jsonArrayContainsString(pipelineTool.value("capability_shared_aliases").toArray(),
                                    QStringLiteral("studio.pipeline.run")));

    const QJsonArray pipelineTools = capabilitiesFromCatalogBySource(catalog, QStringLiteral("analysis_pipeline"));
    QCOMPARE(pipelineTools.size(), 1);
    QCOMPARE(capabilityFromCatalog(catalog, pipelineId).value("name").toString(), pipelineToolName);
}

void TestMneAnalyzeStudioCoreCapabilities::testAnnotateWorkflowCapability()
{
    const QJsonObject annotated = annotateCapabilityMetadata(QJsonObject{
        {"name", "workflow.temporal_filter"},
        {"skill_id", "temporal.filter"},
        {"workflow_operator", true}
    });

    QCOMPARE(annotated.value("capability_id").toString(), QStringLiteral("workflow_skill:temporal.filter"));
    QCOMPARE(annotated.value("capability_kind").toString(), QStringLiteral("workflow_skill"));

    const QJsonArray aliases = annotated.value("capability_aliases").toArray();
    QVERIFY(jsonArrayContainsString(aliases, QStringLiteral("temporal.filter")));
    QVERIFY(jsonArrayContainsString(aliases, QStringLiteral("workflow.temporal_filter")));
}

void TestMneAnalyzeStudioCoreCapabilities::testAnnotateNeuroKernelAndPipelineCapabilities()
{
    const QJsonObject kernelTool = annotateCapabilityMetadata(QJsonObject{
        {"name", "neurokernel.raw_stats"}
    });

    QCOMPARE(kernelTool.value("capability_id").toString(), QStringLiteral("tool:neurokernel.raw_stats"));
    QCOMPARE(kernelTool.value("capability_kind").toString(), QStringLiteral("neurokernel_tool"));
    QVERIFY(jsonArrayContainsString(kernelTool.value("capability_aliases").toArray(),
                                    QStringLiteral("neurokernel.raw_stats")));

    const QJsonObject workflowIoTool = annotateCapabilityMetadata(QJsonObject{
        {"name", "studio.workflow.load"},
        {"capability_kind", "workflow_io"}
    });

    QCOMPARE(workflowIoTool.value("capability_id").toString(), QStringLiteral("tool:studio.workflow.load"));
    QCOMPARE(workflowIoTool.value("capability_kind").toString(), QStringLiteral("workflow_io"));
    QVERIFY(jsonArrayContainsString(workflowIoTool.value("capability_aliases").toArray(),
                                    QStringLiteral("studio.workflow.load")));

    const QString pipelineId = QStringLiteral("analysis.psd_summary");
    const QString aliasToolName = pipelineRunAliasToolName(pipelineId);
    const QJsonObject pipelineTool = annotateCapabilityMetadata(QJsonObject{
        {"name", aliasToolName},
        {"pipeline_id", pipelineId},
        {"capability_aliases", QJsonArray{QStringLiteral("legacy.pipeline.alias")}}
    });

    QCOMPARE(pipelineTool.value("capability_id").toString(), QStringLiteral("pipeline:analysis.psd_summary"));
    QCOMPARE(pipelineTool.value("capability_kind").toString(), QStringLiteral("analysis_pipeline"));
    QCOMPARE(pipelineTool.value("pipeline_run_tool").toString(), QStringLiteral("studio.pipeline.run"));

    const QJsonArray aliases = pipelineTool.value("capability_aliases").toArray();
    QVERIFY(jsonArrayContainsString(aliases, QStringLiteral("legacy.pipeline.alias")));
    QVERIFY(jsonArrayContainsString(aliases, pipelineId));
    QVERIFY(jsonArrayContainsString(aliases, QStringLiteral("studio.pipeline.run")));
    QVERIFY(jsonArrayContainsString(aliases, aliasToolName));
}

void TestMneAnalyzeStudioCoreCapabilities::testRegistryRejectsDuplicatePipelineIds()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY2(temporaryDirectory.isValid(), "Temporary test directory should be created.");

    const QString firstManifestPath = temporaryDirectory.path()
                                      + QStringLiteral("/ext_one/manifest.json");
    const QString secondManifestPath = temporaryDirectory.path()
                                       + QStringLiteral("/ext_two/manifest.json");
    QVERIFY(writeJsonFile(firstManifestPath,
                          pipelineManifest(QStringLiteral("ext.one"),
                                           QStringLiteral("duplicate.pipeline"),
                                           QStringLiteral("Extension One"))));
    QVERIFY(writeJsonFile(secondManifestPath,
                          pipelineManifest(QStringLiteral("ext.two"),
                                           QStringLiteral("duplicate.pipeline"),
                                           QStringLiteral("Extension Two"))));

    ViewProviderRegistry registry;
    QString errorMessage;
    QVERIFY(registry.loadFromDirectory(temporaryDirectory.path(), &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("Duplicate analysis pipeline id `duplicate.pipeline`")));
    QCOMPARE(registry.allManifests().size(), 1);

    const QJsonArray pipelines = registry.analysisPipelineDefinitions();
    QCOMPARE(pipelines.size(), 1);

    const QJsonObject pipeline = pipelines.at(0).toObject();
    QCOMPARE(pipeline.value("id").toString(), QStringLiteral("duplicate.pipeline"));
    QCOMPARE(pipeline.value("capability_id").toString(), QStringLiteral("pipeline:duplicate.pipeline"));
    QCOMPARE(pipeline.value("capability_kind").toString(), QStringLiteral("analysis_pipeline"));
    QVERIFY(jsonArrayContainsString(pipeline.value("capability_aliases").toArray(),
                                    QStringLiteral("duplicate.pipeline")));
    QVERIFY(jsonArrayContainsString(pipeline.value("capability_aliases").toArray(),
                                    QStringLiteral("studio.pipeline.run")));
}

void TestMneAnalyzeStudioCoreCapabilities::testRegistryBuildsPipelineToolDefinitions()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY2(temporaryDirectory.isValid(), "Temporary test directory should be created.");

    const QString manifestPath = temporaryDirectory.path()
                                 + QStringLiteral("/ext_pipeline/manifest.json");
    QVERIFY(writeJsonFile(manifestPath,
                          pipelineManifest(QStringLiteral("ext.pipeline"),
                                           QStringLiteral("analysis.psd_summary"),
                                           QStringLiteral("Pipeline Extension"))));

    ViewProviderRegistry registry;
    QString errorMessage;
    QVERIFY(registry.loadFromDirectory(temporaryDirectory.path(), &errorMessage));
    QVERIFY(errorMessage.isEmpty());

    const QJsonArray tools = registry.analysisPipelineToolDefinitions();
    QCOMPARE(tools.size(), 1);

    const QJsonObject tool = tools.at(0).toObject();
    QCOMPARE(tool.value("name").toString(), QStringLiteral("studio.pipeline.run::analysis.psd_summary"));
    QCOMPARE(tool.value("pipeline_id").toString(), QStringLiteral("analysis.psd_summary"));
    QCOMPARE(tool.value("pipeline_run_tool").toString(), QStringLiteral("studio.pipeline.run"));
    QCOMPARE(tool.value("capability_kind").toString(), QStringLiteral("analysis_pipeline"));
    QCOMPARE(tool.value("capability_id").toString(), QStringLiteral("pipeline:analysis.psd_summary"));
    QCOMPARE(tool.value("extension_id").toString(), QStringLiteral("ext.pipeline"));
    QVERIFY(jsonArrayContainsString(tool.value("capability_aliases").toArray(),
                                    QStringLiteral("analysis.psd_summary")));
    QVERIFY(jsonArrayContainsString(tool.value("capability_aliases").toArray(),
                                    QStringLiteral("studio.pipeline.run")));
}

QTEST_GUILESS_MAIN(TestMneAnalyzeStudioCoreCapabilities)

#include "test_mne_analyze_studio_core_capabilities.moc"
