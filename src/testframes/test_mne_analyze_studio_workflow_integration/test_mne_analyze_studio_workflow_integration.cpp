//=============================================================================================================
/**
 * @file     test_mne_analyze_studio_workflow_integration.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    End-to-end integration tests for the Studio workflow execution chain:
 *           PipelineParser → WorkflowManager → TemporalFilterSkill → output FIFF.
 *
 *           Requires MNE test data at the path reported by
 *           MNE_CPP_TEST_DATA_PATH (or the default relative location).
 */

#include <pipelineparser.h>
#include <workflowgraph.h>
#include <workflowmanager.h>
#include <iskilloperator.h>

// TemporalFilterSkill lives in a separate shared library that is linked at
// CMake level — include its header directly.
#include <temporalfilterskill.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>

using namespace MNEANALYZESTUDIO;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace
{

// Resolve the canonical path to mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif.
// Returns an empty string if the file cannot be found.
QString sampleRawFifPath()
{
    // Honour an explicit env-var override first.
    const QString envPath = QString::fromUtf8(qgetenv("MNE_CPP_TEST_DATA_PATH")).trimmed();
    if(!envPath.isEmpty()) {
        const QString candidate = QDir(envPath).filePath(
            QStringLiteral("MEG/sample/sample_audvis_trunc_raw.fif"));
        if(QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    // Walk up from the test binary looking for resources/data/mne-cpp-test-data.
    QDir dir(QCoreApplication::applicationDirPath());
    for(int depth = 0; depth < 10; ++depth) {
        const QString candidate = dir.filePath(
            QStringLiteral("resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif"));
        if(QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).absoluteFilePath();
        }
        if(!dir.cdUp()) {
            break;
        }
    }
    return QString();
}

// Build a minimal .mne JSON document with one temporal-filter node
// that reads from |inputUri| (file:// URI or absolute path).
QByteArray singleNodeWorkflowJson(const QString& inputUri,
                                  double          highpass,
                                  double          lowpass)
{
    const QJsonObject document{
        {"resources", QJsonArray{
            QJsonObject{
                {"uid",  "raw_in"},
                {"type", "fiff_raw"},
                {"uri",  inputUri}
            }
        }},
        {"pipeline", QJsonArray{
            QJsonObject{
                {"uid",      "filter_node"},
                {"skill_id", "mne.skills.temporal_filter"},
                {"inputs",   QJsonObject{{"raw_data", "raw_in"}}},
                {"parameters", QJsonObject{
                    {"highpass", highpass},
                    {"lowpass",  lowpass}
                }},
                {"outputs", QJsonObject{{"filtered_data", "filtered_out"}}}
            }
        }}
    };
    return QJsonDocument(document).toJson(QJsonDocument::Compact);
}

// Two-node chain: preclean (1–80 Hz) → alpha (8–13 Hz).
QByteArray twoNodeChainJson(const QString& inputUri)
{
    const QJsonObject document{
        {"resources", QJsonArray{
            QJsonObject{{"uid", "raw_in"}, {"type", "fiff_raw"}, {"uri", inputUri}}
        }},
        {"pipeline", QJsonArray{
            QJsonObject{
                {"uid",      "preclean"},
                {"skill_id", "mne.skills.temporal_filter"},
                {"inputs",   QJsonObject{{"raw_data", "raw_in"}}},
                {"parameters", QJsonObject{{"highpass", 1.0}, {"lowpass", 80.0}}},
                {"outputs",  QJsonObject{{"filtered_data", "preclean_out"}}}
            },
            QJsonObject{
                {"uid",      "alpha"},
                {"skill_id", "mne.skills.temporal_filter"},
                {"inputs",   QJsonObject{{"raw_data", "preclean_out"}}},
                {"parameters", QJsonObject{{"highpass", 8.0}, {"lowpass", 13.0}}},
                {"outputs",  QJsonObject{{"filtered_data", "alpha_out"}}}
            }
        }}
    };
    return QJsonDocument(document).toJson(QJsonDocument::Compact);
}

} // namespace


// ─────────────────────────────────────────────────────────────────────────────
// Test class
// ─────────────────────────────────────────────────────────────────────────────

class TestWorkflowIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // ── Tier 1: parser round-trip (no real FIFF needed) ─────────────────
    void testParserAcceptsValidSingleNode();
    void testParserAcceptsChainedNodes();
    void testParserRejectsMissingSkillId();
    void testParserRejectsCyclicGraph();

    // ── Tier 2: WorkflowManager operator registration ────────────────────
    void testManagerCanRegisterAndQueryOperator();
    void testManagerRejectsNullOperator();

    // ── Tier 3: end-to-end execution with real FIFF ──────────────────────
    void testSingleNodeFilterProducesOutputFile();
    void testTwoNodeChainBothFilesProduced();
    void testNodeStatusUpdatedAfterExecution();
    void testHighpassAboveLowpassReturnsError();

private:
    QString m_sampleFifPath;
    bool    m_hasFifData = false;
};

void TestWorkflowIntegration::initTestCase()
{
    m_sampleFifPath = sampleRawFifPath();
    m_hasFifData    = !m_sampleFifPath.isEmpty();
    if(!m_hasFifData) {
        qWarning("sample_audvis_trunc_raw.fif not found — execution tests will be skipped.");
        qWarning("Set MNE_CPP_TEST_DATA_PATH to the mne-cpp-test-data root to enable them.");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Tier 1 — parser
// ─────────────────────────────────────────────────────────────────────────────

void TestWorkflowIntegration::testParserAcceptsValidSingleNode()
{
    const QByteArray json = singleNodeWorkflowJson(
        QStringLiteral("file:///tmp/input.fif"), 1.0, 40.0);

    PipelineParser parser;
    const WorkflowGraph graph = parser.parseJson(json, QStringLiteral("test"));

    QVERIFY(!graph.nodes().isEmpty());
    QCOMPARE(graph.resources().size(), 1);
    QCOMPARE(graph.nodes().size(), 1);
    QCOMPARE(graph.nodes().first().skillId, QStringLiteral("mne.skills.temporal_filter"));
    QCOMPARE(graph.nodes().first().parameters.value("highpass").toDouble(), 1.0);
    QCOMPARE(graph.nodes().first().parameters.value("lowpass").toDouble(), 40.0);
}

void TestWorkflowIntegration::testParserAcceptsChainedNodes()
{
    const QByteArray json = twoNodeChainJson(QStringLiteral("file:///tmp/input.fif"));

    PipelineParser parser;
    const WorkflowGraph graph = parser.parseJson(json, QStringLiteral("test"));

    QVERIFY(!graph.nodes().isEmpty());
    QCOMPARE(graph.nodes().size(), 2);

    // Topological sort should give preclean before alpha.
    const QVector<QString> order = graph.topologicalSort();
    QCOMPARE(order.size(), 2);
    QCOMPARE(order.at(0), QStringLiteral("preclean"));
    QCOMPARE(order.at(1), QStringLiteral("alpha"));
}

void TestWorkflowIntegration::testParserRejectsMissingSkillId()
{
    const QJsonObject document{
        {"resources", QJsonArray{
            QJsonObject{{"uid","raw_in"},{"type","fiff_raw"},{"uri","file:///tmp/x.fif"}}
        }},
        {"pipeline", QJsonArray{
            QJsonObject{
                {"uid",     "bad_node"},
                // skill_id deliberately omitted
                {"inputs",  QJsonObject{{"raw_data","raw_in"}}},
                {"outputs", QJsonObject{{"filtered_data","out"}}}
            }
        }}
    };
    const QByteArray json = QJsonDocument(document).toJson(QJsonDocument::Compact);

    PipelineParser parser;
    bool threw = false;
    try {
        parser.parseJson(json, QStringLiteral("test"));
    } catch(const std::exception&) {
        threw = true;
    }
    QVERIFY2(threw, "Parser must throw for a node missing skill_id.");
}

void TestWorkflowIntegration::testParserRejectsCyclicGraph()
{
    // node A reads from output of B, node B reads from output of A → cycle.
    const QJsonObject document{
        {"resources", QJsonArray{
            QJsonObject{{"uid","raw_in"},{"type","fiff_raw"},{"uri","file:///tmp/x.fif"}}
        }},
        {"pipeline", QJsonArray{
            QJsonObject{
                {"uid","nodeA"},{"skill_id","mne.skills.temporal_filter"},
                {"inputs",  QJsonObject{{"raw_data","out_b"}}},
                {"outputs", QJsonObject{{"filtered_data","out_a"}}}
            },
            QJsonObject{
                {"uid","nodeB"},{"skill_id","mne.skills.temporal_filter"},
                {"inputs",  QJsonObject{{"raw_data","out_a"}}},
                {"outputs", QJsonObject{{"filtered_data","out_b"}}}
            }
        }}
    };
    const QByteArray json = QJsonDocument(document).toJson(QJsonDocument::Compact);

    PipelineParser parser;
    bool threw = false;
    try {
        parser.parseJson(json, QStringLiteral("test"));
    } catch(const std::exception&) {
        threw = true;
    }
    QVERIFY2(threw, "Parser must throw for a cyclic graph.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Tier 2 — WorkflowManager registration
// ─────────────────────────────────────────────────────────────────────────────

void TestWorkflowIntegration::testManagerCanRegisterAndQueryOperator()
{
    WorkflowManager manager;
    TemporalFilterSkill skill;
    manager.registerOperator(&skill);

    QVERIFY(manager.canHandleTool(QStringLiteral("apply_filter")));
    QVERIFY(!manager.canHandleTool(QStringLiteral("nonexistent_tool")));
}

void TestWorkflowIntegration::testManagerRejectsNullOperator()
{
    WorkflowManager manager;
    bool threw = false;
    try {
        manager.registerOperator(nullptr);
    } catch(const std::exception&) {
        threw = true;
    }
    QVERIFY2(threw, "registerOperator must throw for a null pointer.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Tier 3 — end-to-end execution
// ─────────────────────────────────────────────────────────────────────────────

void TestWorkflowIntegration::testSingleNodeFilterProducesOutputFile()
{
    if(!m_hasFifData) {
        QSKIP("FIFF test data not available.");
    }

    // Write the workflow JSON to a temp file so we can use loadAnalysisFile,
    // which handles resource-uid resolution and triggers execution automatically.
    QTemporaryFile tmp(QDir::tempPath() + "/tst_single_XXXXXX.mne");
    QVERIFY(tmp.open());

    const QString inputUri = QString("file://%1").arg(m_sampleFifPath);
    const QByteArray json  = singleNodeWorkflowJson(inputUri, 1.0, 40.0);
    tmp.write(json);
    tmp.flush();
    const QString tmpPath = tmp.fileName();
    tmp.close();

    WorkflowManager manager;
    TemporalFilterSkill skill;
    manager.registerOperator(&skill);

    // Capture the graph-changed signal to read the final state.
    QJsonObject finalGraph;
    connect(&manager, &WorkflowManager::activeGraphChanged, this,
            [&finalGraph](const QJsonObject& g) { finalGraph = g; });

    // loadAnalysisFile parses, sets active graph, and executes pending nodes.
    bool threw = false;
    try {
        manager.loadAnalysisFile(tmpPath);
    } catch(const std::exception& e) {
        threw = true;
        QFAIL(QString("Unexpected exception: %1").arg(e.what()).toUtf8().constData());
    }
    QVERIFY(!threw);

    // Verify at least one filtered file was produced next to the input.
    const QFileInfo inputInfo(m_sampleFifPath);
    const QDir inputDir = inputInfo.dir();
    const QStringList produced = inputDir.entryList(
        QStringList() << QStringLiteral("*_hp*Hz_lp*Hz.fif"), QDir::Files);
    QVERIFY2(!produced.isEmpty(), "Expected a filtered FIFF output file on disk.");

    // Cleanup: remove any files we created to keep the test directory tidy.
    for(const QString& fileName : produced) {
        QFile::remove(inputDir.filePath(fileName));
    }
}

void TestWorkflowIntegration::testTwoNodeChainBothFilesProduced()
{
    if(!m_hasFifData) {
        QSKIP("FIFF test data not available.");
    }

    const QString inputUri = QString("file://%1").arg(m_sampleFifPath);
    const QByteArray json  = twoNodeChainJson(inputUri);

    PipelineParser parser;
    WorkflowGraph graph = parser.parseJson(json, QStringLiteral("test"));
    QCOMPARE(graph.nodes().size(), 2);

    WorkflowManager manager;
    TemporalFilterSkill skill;
    manager.registerOperator(&skill);
    manager.setActiveGraph(graph);

    // Collect all graph-changed emissions to track node completions.
    QVector<QJsonObject> emissions;
    connect(&manager, &WorkflowManager::activeGraphChanged, this,
            [&emissions](const QJsonObject& g) { emissions.append(g); });

    bool threw = false;
    QString exceptionMessage;
    try {
        // Trigger execution by loading the already-set graph via executePendingNodes.
        // setActiveGraph does not auto-execute — call loadAnalysisFile to trigger it.
        // Instead we call a known-good path: supply the graph via a temp file.
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString analysisFilePath = tmp.path() + QStringLiteral("/chain.mne");
        QFile f(analysisFilePath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(json);
        f.close();
        manager.loadAnalysisFile(analysisFilePath);
    } catch(const std::exception& e) {
        threw = true;
        exceptionMessage = QString::fromUtf8(e.what());
    }

    if(threw) {
        QFAIL(QString("Two-node chain threw: %1").arg(exceptionMessage).toUtf8().constData());
    }

    // Two completions expected (one per node).
    QVERIFY2(emissions.size() >= 2,
             qPrintable(QString("Expected >= 2 activeGraphChanged emissions, got %1.")
                            .arg(emissions.size())));

    // Check both nodes are completed in the final graph.
    const QJsonObject finalGraph = emissions.last();
    const QJsonArray pipeline    = finalGraph.value(QStringLiteral("pipeline")).toArray();
    int completedCount = 0;
    for(const QJsonValue& v : pipeline) {
        const QString status = v.toObject()
                                   .value(QStringLiteral("runtime")).toObject()
                                   .value(QStringLiteral("status")).toString();
        if(status == QLatin1String("completed")) {
            ++completedCount;
        }
    }
    QCOMPARE(completedCount, 2);

    // Cleanup produced files.
    const QFileInfo inputInfo(m_sampleFifPath);
    const QDir inputDir = inputInfo.dir();
    const QStringList produced = inputDir.entryList(
        QStringList() << QStringLiteral("*_hp*Hz_lp*Hz.fif"), QDir::Files);
    for(const QString& fileName : produced) {
        QFile::remove(inputDir.filePath(fileName));
    }
}

void TestWorkflowIntegration::testNodeStatusUpdatedAfterExecution()
{
    if(!m_hasFifData) {
        QSKIP("FIFF test data not available.");
    }

    const QString inputUri = QString("file://%1").arg(m_sampleFifPath);
    const QByteArray json  = singleNodeWorkflowJson(inputUri, 1.0, 40.0);

    PipelineParser parser;
    WorkflowGraph graph = parser.parseJson(json, QStringLiteral("test"));

    WorkflowManager manager;
    TemporalFilterSkill skill;
    manager.registerOperator(&skill);

    QJsonObject lastGraph;
    connect(&manager, &WorkflowManager::activeGraphChanged, this,
            [&lastGraph](const QJsonObject& g) { lastGraph = g; });

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString analysisFilePath = tmp.path() + QStringLiteral("/single.mne");
    QFile f(analysisFilePath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(json);
    f.close();

    bool threw = false;
    try {
        manager.loadAnalysisFile(analysisFilePath);
    } catch(const std::exception&) {
        threw = true;
    }
    QVERIFY(!threw);
    QVERIFY2(!lastGraph.isEmpty(), "activeGraphChanged must have been emitted.");

    const QJsonArray pipeline = lastGraph.value(QStringLiteral("pipeline")).toArray();
    QCOMPARE(pipeline.size(), 1);
    const QString status = pipeline.at(0).toObject()
                               .value(QStringLiteral("runtime")).toObject()
                               .value(QStringLiteral("status")).toString();
    QCOMPARE(status, QStringLiteral("completed"));

    // Cleanup.
    const QFileInfo inputInfo(m_sampleFifPath);
    const QDir inputDir = inputInfo.dir();
    const QStringList produced = inputDir.entryList(
        QStringList() << QStringLiteral("*_hp*Hz_lp*Hz.fif"), QDir::Files);
    for(const QString& fileName : produced) {
        QFile::remove(inputDir.filePath(fileName));
    }
}

void TestWorkflowIntegration::testHighpassAboveLowpassReturnsError()
{
    if(!m_hasFifData) {
        QSKIP("FIFF test data not available.");
    }

    // highpass (50 Hz) > lowpass (10 Hz) → TemporalFilterSkill should return error
    // and WorkflowManager should propagate it as a WorkflowValidationError.
    const QString inputUri = QString("file://%1").arg(m_sampleFifPath);
    const QByteArray json  = singleNodeWorkflowJson(inputUri, 50.0, 10.0);

    PipelineParser parser;
    WorkflowGraph graph = parser.parseJson(json, QStringLiteral("test"));

    WorkflowManager manager;
    TemporalFilterSkill skill;
    manager.registerOperator(&skill);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString analysisFilePath = tmp.path() + QStringLiteral("/bad_params.mne");
    QFile f(analysisFilePath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(json);
    f.close();

    bool threw = false;
    try {
        manager.loadAnalysisFile(analysisFilePath);
    } catch(const std::exception&) {
        threw = true;
    }
    QVERIFY2(threw,
             "WorkflowManager must throw when the skill returns an error status.");
}

QTEST_GUILESS_MAIN(TestWorkflowIntegration)
#include "test_mne_analyze_studio_workflow_integration.moc"
