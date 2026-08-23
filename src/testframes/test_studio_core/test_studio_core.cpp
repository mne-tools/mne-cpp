//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_studio_core.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Checks the mne_analyze_studio core logic.
 *
 * These classes are plain logic rather than widgets, so unlike the display
 * smoke tests this asserts behaviour: which provider a configuration resolves
 * to, whether a request round trips through serialisation, what a buffer
 * reports for a file that does not exist.
 *
 * The planner is only exercised in mock mode and through its configuration
 * accessors. Nothing here performs a network request, and the environment
 * variables the planner reads are cleared in initTestCase so the result does
 * not depend on whatever happens to be set in the shell that runs the suite.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <core/llmtoolplanner.h>
#include <core/jsonrpcmessage.h>
#include <core/fiffbuffer.h>
#include <core/capabilitycatalog.h>
#include <core/capabilityutils.h>
#include <workbench/analysisresultwidget.h>
#include <workbench/agentchatdockwidget.h>
#include <workbench/extensionhostedviewwidget.h>
#include <workbench/llmsettingsdialog.h>
#include <workbench/pillselectorwidget.h>
#include <workbench/workflowminimapwidget.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTreeWidget>

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZESTUDIO;

//=============================================================================================================
/**
 * DECLARE CLASS TestStudioCore
 *
 * @brief Checks the mne_analyze_studio core logic.
 */
class TestStudioCore: public QObject
{
    Q_OBJECT

public:
    TestStudioCore() = default;

private:
    static QString rawPath();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void jsonRpc_requestRoundTrips();
    void jsonRpc_responseAndErrorAreDistinct();
    void jsonRpc_rejectsMalformedPayload();

    void planner_modeSelectsProvider_data();
    void planner_modeSelectsProvider();
    void planner_mockModeIsConfiguredWithoutCredentials();
    void planner_httpModeNeedsEndpointAndModel();
    void planner_statusSummaryMentionsMode();
    void agentChatDockWidget_conversationAndConfirmations();
    void llmSettingsDialog_ruleBasedAndProfileFlow();
    void workflowMiniMapWidget_graphRenderingAndActivation();
    void analysisResultWidget_jsonAndStatisticsViews();
    void extensionHostedViewWidget_descriptorCommandsAndUpdates();

    void fiffBuffer_reportsUriAndKind();
    void fiffBuffer_missingFileFailsToOpen();
    void fiffBuffer_realFileOpensAndDescribesItself();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestStudioCore::rawPath()
{
    return QCoreApplication::applicationDirPath()
           + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
}

//=============================================================================================================

void TestStudioCore::initTestCase()
{
    // The planner falls back to these when its configuration leaves a field
    // empty. Clearing them keeps the test reading the configuration under test
    // rather than whatever the developer has exported.
    qunsetenv("MNE_ANALYZE_STUDIO_LLM_MODE");
    qunsetenv("MNE_ANALYZE_STUDIO_LLM_PROVIDER");
    qunsetenv("MNE_ANALYZE_STUDIO_LLM_ENDPOINT");
    qunsetenv("MNE_ANALYZE_STUDIO_LLM_API_KEY");
    qunsetenv("MNE_ANALYZE_STUDIO_LLM_MODEL");
}

//=============================================================================================================

void TestStudioCore::cleanupTestCase()
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    settings.remove("agent/profiles/coverage-test-profile");
    if(settings.value("agent/selected_profile").toString() == QLatin1String("coverage-test-profile")) {
        settings.remove("agent/selected_profile");
    }
}

//=============================================================================================================

void TestStudioCore::jsonRpc_requestRoundTrips()
{
    QJsonObject params;
    params.insert("path", QString("/tmp/example.fif"));
    params.insert("count", 3);

    const QJsonObject request = JsonRpcMessage::createRequest("id-1", "tools/call", params);

    QVERIFY(JsonRpcMessage::isValid(request));
    QCOMPARE(request.value("method").toString(), QString("tools/call"));
    QCOMPARE(request.value("id").toString(), QString("id-1"));

    // Serialising and reading back has to preserve the payload exactly, since
    // this is the wire format the skill host and the workbench talk over.
    const QByteArray wire = JsonRpcMessage::serialize(request);
    QVERIFY(!wire.isEmpty());

    QJsonObject parsed;
    QString error;
    QVERIFY2(JsonRpcMessage::deserialize(wire, parsed, error),
             qPrintable(QString("round trip failed: %1").arg(error)));

    QCOMPARE(parsed.value("method").toString(), QString("tools/call"));
    QCOMPARE(parsed.value("id").toString(), QString("id-1"));
    QCOMPARE(parsed.value("params").toObject().value("path").toString(), QString("/tmp/example.fif"));
    QCOMPARE(parsed.value("params").toObject().value("count").toInt(), 3);
}

//=============================================================================================================

void TestStudioCore::jsonRpc_responseAndErrorAreDistinct()
{
    QJsonObject result;
    result.insert("ok", true);

    const QJsonObject response = JsonRpcMessage::createResponse(QJsonValue("id-2"), result);
    const QJsonObject failure  = JsonRpcMessage::createError(QJsonValue("id-2"), -32601, "method not found");

    QVERIFY(JsonRpcMessage::isValid(response));
    QVERIFY(JsonRpcMessage::isValid(failure));

    // A caller decides success by which key is present, so a message must not
    // carry both. If it did, a failure could be read as a result.
    QVERIFY(response.contains("result"));
    QVERIFY(!response.contains("error"));

    QVERIFY(failure.contains("error"));
    QVERIFY(!failure.contains("result"));

    QCOMPARE(failure.value("error").toObject().value("code").toInt(), -32601);
    QCOMPARE(failure.value("error").toObject().value("message").toString(), QString("method not found"));
}

//=============================================================================================================

void TestStudioCore::jsonRpc_rejectsMalformedPayload()
{
    QJsonObject parsed;
    QString error;

    // Truncated JSON is what a dropped connection leaves behind, so it has to
    // be reported rather than silently yielding an empty message.
    QVERIFY(!JsonRpcMessage::deserialize(QByteArray("{\"jsonrpc\":\"2.0\","), parsed, error));
    QVERIFY2(!error.isEmpty(), "malformed payload was rejected without saying why");

    QVERIFY(!JsonRpcMessage::deserialize(QByteArray(""), parsed, error));

    // A well formed JSON document that is not a JSON-RPC message must not pass
    // validation just because it parses.
    QJsonObject notRpc;
    notRpc.insert("hello", QString("world"));
    QVERIFY(!JsonRpcMessage::isValid(notRpc));
}

//=============================================================================================================

void TestStudioCore::planner_modeSelectsProvider_data()
{
    QTest::addColumn<QString>("mode");
    QTest::addColumn<QString>("expectedProvider");

    // The mode string is what picks the request shape and the endpoint, so a
    // mode that resolved to the wrong provider would send a payload the remote
    // service cannot read. The names are exact, not prefixes: "openai" alone
    // is not the OpenAI mode, it falls through to the generic compatible path.
    QTest::newRow("openai responses")    << "openai_responses"    << "OpenAI";
    QTest::newRow("gemini openai compat") << "gemini_openai"      << "Google Gemini";
    QTest::newRow("anthropic messages")  << "anthropic_messages"  << "Anthropic";

    // Anything unrecognised has to land on the generic compatible provider
    // rather than on one of the specific ones, which is what stops an unknown
    // mode silently borrowing another provider's request format.
    QTest::newRow("unrecognised")        << "something-else"      << "openai-compatible";
}

//=============================================================================================================

void TestStudioCore::planner_modeSelectsProvider()
{
    QFETCH(QString, mode);
    QFETCH(QString, expectedProvider);

    LlmToolPlanner planner;

    LlmPlannerConfig config;
    config.mode = mode;
    config.model = "some-model";
    config.apiKey = "not-a-real-key";
    planner.setConfiguration(config);

    QCOMPARE(planner.configuration().mode, mode);
    QCOMPARE(planner.providerName(), expectedProvider);

    // None of these are mock mode, so the workbench would really issue a
    // request for any of them.
    QVERIFY(!planner.isMockMode());
}

//=============================================================================================================

void TestStudioCore::planner_mockModeIsConfiguredWithoutCredentials()
{
    LlmToolPlanner planner;

    LlmPlannerConfig config;
    config.mode = "mock";
    planner.setConfiguration(config);

    QVERIFY(planner.isMockMode());

    // Mock mode exists so the workbench is usable with no credentials, so it
    // has to report itself configured even with an empty key and model.
    QVERIFY2(planner.isConfigured(),
             "mock mode should not require credentials, that is its purpose");

    // Case should not matter, since this arrives from a settings file a person edited.
    LlmPlannerConfig upper;
    upper.mode = "MOCK";
    planner.setConfiguration(upper);
    QVERIFY(planner.isMockMode());
}

//=============================================================================================================

void TestStudioCore::planner_httpModeNeedsEndpointAndModel()
{
    LlmToolPlanner planner;

    // A plain HTTP mode with nothing filled in is not usable, and saying so is
    // what stops the workbench issuing a request that cannot succeed.
    LlmPlannerConfig empty;
    empty.mode = "http";
    planner.setConfiguration(empty);
    QVERIFY(!planner.isConfigured());

    // An endpoint alone is still not enough without a model.
    LlmPlannerConfig endpointOnly;
    endpointOnly.mode = "http";
    endpointOnly.endpoint = "http://localhost:1234/v1/chat";
    planner.setConfiguration(endpointOnly);
    QVERIFY(!planner.isConfigured());

    LlmPlannerConfig complete;
    complete.mode = "http";
    complete.endpoint = "http://localhost:1234/v1/chat";
    complete.model = "local-model";
    planner.setConfiguration(complete);
    QVERIFY(planner.isConfigured());
}

//=============================================================================================================

void TestStudioCore::planner_statusSummaryMentionsMode()
{
    LlmToolPlanner planner;

    LlmPlannerConfig config;
    config.mode = "mock";
    planner.setConfiguration(config);

    // The summary is shown in the UI, so an empty one would leave a user with
    // no way to tell which provider is in use.
    const QString summary = planner.statusSummary();
    QVERIFY2(!summary.isEmpty(), "status summary is empty");
    QVERIFY2(summary.contains("Mock", Qt::CaseInsensitive),
             qPrintable(QString("summary does not mention the mode: %1").arg(summary)));
}

//=============================================================================================================

void TestStudioCore::agentChatDockWidget_conversationAndConfirmations()
{
    AgentChatDockWidget widget;
    widget.resize(480, 720);

    auto* input = widget.findChild<QLineEdit*>("agentInput");
    auto* sendButton = widget.findChild<QPushButton*>("agentSendBtn");
    auto* connectionButton = widget.findChild<QPushButton*>("agentConnectBtn");
    auto* statusLabel = widget.findChild<QLabel*>("agentStatusLabel");
    auto* validationLabel = widget.findChild<QLabel*>("agentValidationLabel");
    auto* mainStack = widget.findChild<QStackedWidget*>();
    QVERIFY(input != nullptr);
    QVERIFY(sendButton != nullptr);
    QVERIFY(connectionButton != nullptr);
    QVERIFY(statusLabel != nullptr);
    QVERIFY(validationLabel != nullptr);
    QVERIFY(mainStack != nullptr);

    QSignalSpy commandSpy(&widget, &AgentChatDockWidget::commandSubmitted);
    input->setText("  inspect active data  ");
    QTest::mouseClick(sendButton, Qt::LeftButton);
    QCOMPARE(commandSpy.size(), 1);
    QCOMPARE(commandSpy.takeFirst().at(0).toString(), QString("inspect active data"));
    QVERIFY(input->text().isEmpty());
    QCOMPARE(widget.currentConversationEntries().size(), 1);

    widget.appendTranscript("Assistant> analysis complete");
    QCOMPARE(widget.currentConversationEntries().size(), 2);
    widget.appendTranscript("You> start another task");
    QCOMPARE(widget.archivedConversationSessions().size(), 1);
    QCOMPARE(widget.currentConversationEntries().size(), 1);
    QCOMPARE(widget.archivedConversationSessions().first().toObject().value("title").toString(),
             QString("inspect active data"));
    widget.appendTranscript("   ");
    QCOMPARE(widget.currentConversationEntries().size(), 1);

    QSignalSpy approveSpy(&widget, &AgentChatDockWidget::confirmationRequested);
    QSignalSpy dismissSpy(&widget, &AgentChatDockWidget::confirmationDismissed);
    widget.setPendingConfirmations({QJsonObject{{"command", "apply-filter"},
                                                {"title", "Apply filter"},
                                                {"reason", "changes data"},
                                                {"stale", true},
                                                {"stale_reason", "view changed"}},
                                    QJsonObject{{"command", ""}}});
    auto* approveButton = widget.findChild<QPushButton*>("agentApproveBtn");
    auto* dismissButton = widget.findChild<QPushButton*>("agentDismissBtn");
    QVERIFY(approveButton != nullptr);
    QVERIFY(dismissButton != nullptr);
    QCOMPARE(approveButton->text(), QString("Approve Anyway"));
    QTest::mouseClick(approveButton, Qt::LeftButton);
    QTest::mouseClick(dismissButton, Qt::LeftButton);
    QCOMPARE(approveSpy.takeFirst().at(0).toString(), QString("apply-filter"));
    QCOMPARE(dismissSpy.takeFirst().at(0).toString(), QString("apply-filter"));
    widget.setPendingConfirmations({});

    widget.setPlannerStatus("LLM: Connected | Provider: OpenAI | Model: gpt-test");
    QVERIFY(statusLabel->text().contains("OpenAI"));
    QVERIFY(statusLabel->text().contains("gpt-test"));
    widget.setPlannerStatus("Deterministic fallback only");
    QCOMPARE(statusLabel->text(), QString("Rule-based planning active"));

    widget.setConnectionState("Remote", true, "API key missing");
    QCOMPARE(connectionButton->text(), QString("!"));
    QVERIFY(validationLabel->isVisibleTo(&widget));
    QCOMPARE(validationLabel->text(), QString("API key missing"));
    widget.setConnectionState("Rule-based", false);
    QVERIFY(!connectionButton->isVisibleTo(&widget));

    QSignalSpy settingsSpy(&widget, &AgentChatDockWidget::openConnectionSettingsRequested);
    widget.setConnectionState("Remote", false);
    QTest::mouseClick(connectionButton, Qt::LeftButton);
    QCOMPARE(settingsSpy.size(), 1);

    widget.setConnectionProfiles({"work", "local"}, "work");
    widget.setConnectionModes({qMakePair(QString("OpenAI"), QString("openai_responses")),
                               qMakePair(QString("Rule-Based"), QString("disabled"))},
                              "openai_responses");
    widget.setSuggestedModels({"model-a", "model-b"}, "model-b");
    widget.setPlannerSafetyLevel("safe");
    widget.setPlannerSafetyLevel("unsupported");

    const QJsonArray restoredCurrent{QJsonObject{{"text", "Assistant> restored"},
                                                 {"timestamp", "2026-08-23T10:00:00Z"}}};
    const QJsonArray restoredArchive{QJsonObject{{"title", "Prior session"},
                                                 {"preview", "prior preview"},
                                                 {"timestamp", "2026-08-22T10:00:00Z"},
                                                 {"entries", restoredCurrent}}};
    widget.restoreConversationState(restoredCurrent, restoredArchive);
    QCOMPARE(widget.currentConversationEntries(), restoredCurrent);
    QCOMPARE(widget.archivedConversationSessions(), restoredArchive);

    QPushButton* historyButton = nullptr;
    for(QPushButton* button : widget.findChildren<QPushButton*>("agentHeaderBtn")) {
        if(button->text() == QLatin1String("History")) {
            historyButton = button;
            break;
        }
    }
    QVERIFY(historyButton != nullptr);
    QTest::mouseClick(historyButton, Qt::LeftButton);
    QCOMPARE(mainStack->currentIndex(), 1);
    QTest::mouseClick(historyButton, Qt::LeftButton);
    QCOMPARE(mainStack->currentIndex(), 0);

    QApplication::processEvents();
}

//=============================================================================================================

void TestStudioCore::llmSettingsDialog_ruleBasedAndProfileFlow()
{
    QSettings settings("MNE-CPP", "MNEAnalyzeStudio");
    const QString profileName = "coverage-test-profile";
    const QString profileKey = QString("agent/profiles/%1").arg(profileName);
    settings.setValue(QString("%1/mode").arg(profileKey), "openai_responses");
    settings.setValue(QString("%1/model").arg(profileKey), "coverage-model");
    settings.setValue(QString("%1/api_key").arg(profileKey), "coverage-key");
    settings.setValue("agent/selected_profile", profileName);

    LlmPlannerConfig config;
    config.mode = "disabled";
    LlmSettingsDialog dialog(config);
    QCOMPARE(dialog.configuration().mode, QString("disabled"));
    QVERIFY(!dialog.hasValidationResult());

    dialog.setTestScenario("show the active raw view", QJsonArray(), QJsonObject());
    QVERIFY(QMetaObject::invokeMethod(&dialog, "runPlannerTest", Qt::DirectConnection));
    QVERIFY(dialog.hasValidationResult());
    QVERIFY(!dialog.lastValidationSucceeded());
    QVERIFY(!dialog.lastValidationMessage().isEmpty());

    QVERIFY(QMetaObject::invokeMethod(&dialog,
                                      "applySelectedProfile",
                                      Qt::DirectConnection,
                                      Q_ARG(QString, profileName)));
    const LlmPlannerConfig profileConfig = dialog.configuration();
    QCOMPARE(profileConfig.mode, QString("openai_responses"));
    QCOMPARE(profileConfig.model, QString("coverage-model"));
    QCOMPARE(profileConfig.apiKey, QString("coverage-key"));

    const auto selectors = dialog.findChildren<PillSelectorWidget*>();
    PillSelectorWidget* modeSelector = nullptr;
    PillSelectorWidget* suggestedModelSelector = nullptr;
    for(PillSelectorWidget* selector : selectors) {
        if(selector->currentValue() == QLatin1String("openai_responses")) {
            modeSelector = selector;
        } else if(selector->currentValue().isEmpty()) {
            suggestedModelSelector = selector;
        }
    }
    QVERIFY(modeSelector != nullptr);
    QVERIFY(suggestedModelSelector != nullptr);

    modeSelector->setCurrentValue("gemini_openai");
    QVERIFY(QMetaObject::invokeMethod(&dialog, "updateModeDefaults", Qt::DirectConnection));
    QCOMPARE(dialog.configuration().mode, QString("gemini_openai"));
    QVERIFY(dialog.configuration().model.isEmpty());
    QVERIFY(!dialog.hasValidationResult());

    suggestedModelSelector->setItems({qMakePair(QString("Gemini test"), QString("gemini-test"))});
    suggestedModelSelector->setCurrentValue("gemini-test");
    QVERIFY(QMetaObject::invokeMethod(&dialog, "applySuggestedModel", Qt::DirectConnection));
    QCOMPARE(dialog.configuration().model, QString("gemini-test"));

    modeSelector->setCurrentValue("disabled");
    QVERIFY(QMetaObject::invokeMethod(&dialog, "updateModeDefaults", Qt::DirectConnection));
    QVERIFY(QMetaObject::invokeMethod(&dialog, "browseModels", Qt::DirectConnection));
    const auto statusLabels = dialog.findChildren<QLabel*>();
    QVERIFY(std::any_of(statusLabels.cbegin(), statusLabels.cend(), [](const QLabel* label) {
        return label->text().contains("No model catalog endpoint");
    }));

}

//=============================================================================================================

void TestStudioCore::workflowMiniMapWidget_graphRenderingAndActivation()
{
    WorkflowMiniMapWidget widget;
    widget.resize(600, 300);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.minimumSizeHint(), QSize(280, 180));
    QCOMPARE(widget.sizeHint(), QSize(420, 220));

    QImage image(widget.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    widget.render(&image);
    QVERIFY(!image.isNull());

    const QJsonObject sourceNode{
        {"uid", "source"},
        {"label", "Load Raw"},
        {"stage", "input"},
        {"outputs", QJsonObject{{"raw", "raw-output"}}},
        {"runtime", QJsonObject{{"status", "completed"}}}
    };
    const QJsonObject processNode{
        {"uid", "process"},
        {"label", "Filter"},
        {"skill_id", "filter-skill"},
        {"inputs", QJsonObject{{"raw", "raw-output"}}},
        {"outputs", QJsonObject{{"filtered", "filtered-output"}}},
        {"runtime", QJsonObject{{"status", "running"}}}
    };
    const QJsonObject resultNode{
        {"uid", "result"},
        {"inputs", QJsonObject{{"data", "filtered-output"}}},
        {"runtime", QJsonObject{{"status", "failed"}}}
    };
    widget.setWorkflowGraph(QJsonObject{{"pipeline", QJsonArray{resultNode, sourceNode, processNode}}});
    widget.setFocusNodeUid("process");

    image.fill(Qt::transparent);
    widget.render(&image);
    QVERIFY(image.pixelColor(image.width() / 2, image.height() / 2).alpha() > 0);

    QSignalSpy activationSpy(&widget, &WorkflowMiniMapWidget::nodeActivated);
    QTest::mouseMove(&widget, QPoint(300, 150));
    QCOMPARE(widget.cursor().shape(), Qt::PointingHandCursor);
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, QPoint(300, 150));
    QCOMPARE(activationSpy.size(), 1);
    QCOMPARE(activationSpy.takeFirst().at(0).toString(), QString("process"));
    QTest::mouseClick(&widget, Qt::RightButton, Qt::NoModifier, QPoint(300, 150));
    QCOMPARE(activationSpy.size(), 0);
    QTest::mouseMove(&widget, QPoint(5, 5));
    QCOMPARE(widget.cursor().shape(), Qt::ArrowCursor);

    QEvent leaveEvent(QEvent::Leave);
    QApplication::sendEvent(&widget, &leaveEvent);
    QCOMPARE(widget.cursor().shape(), Qt::ArrowCursor);

    widget.setFocusNodeUid("missing");
    widget.setFocusNodeUid("source");
    widget.resize(20, 20);
    image = QImage(widget.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    widget.render(&image);

    const QJsonObject cycleA{{"uid", "a"},
                             {"inputs", QJsonObject{{"input", "b-output"}}},
                             {"outputs", QJsonObject{{"output", "a-output"}}}};
    const QJsonObject cycleB{{"uid", "b"},
                             {"inputs", QJsonObject{{"input", "a-output"}}},
                             {"outputs", QJsonObject{{"output", "b-output"}}}};
    widget.resize(400, 220);
    widget.setWorkflowGraph(QJsonObject{{"pipeline", QJsonArray{cycleB, QJsonObject(), cycleA}}});
    image = QImage(widget.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    widget.render(&image);

    widget.setWorkflowGraph(QJsonObject{{"pipeline", QJsonArray()}});
    image.fill(Qt::transparent);
    widget.render(&image);
}

//=============================================================================================================

void TestStudioCore::analysisResultWidget_jsonAndStatisticsViews()
{
    AnalysisResultWidget widget;
    QTreeWidget* tree = widget.findChild<QTreeWidget*>();
    QTableWidget* table = widget.findChild<QTableWidget*>();
    QStackedWidget* stack = widget.findChild<QStackedWidget*>();
    QVERIFY(tree);
    QVERIFY(table);
    QVERIFY(stack);

    const QJsonObject genericResult{
        {"message", "Generic output"},
        {"object", QJsonObject{{"enabled", true}, {"missing", QJsonValue::Null}}},
        {"array", QJsonArray{"entry", 2.5}},
        {"count", 7}
    };
    widget.setResult("custom.summary", genericResult);
    QCOMPARE(widget.toolName(), QString("custom.summary"));
    QCOMPARE(widget.result(), genericResult);
    QCOMPARE(stack->currentWidget(), tree);
    QCOMPARE(tree->topLevelItemCount(), genericResult.size());
    QVERIFY(tree->findItems("object", Qt::MatchExactly).constFirst()->childCount() == 2);
    widget.setResultHistory(QJsonArray{genericResult});
    widget.setRuntimeContext(QJsonObject{{"selected_channel", "MEG 0111"}});

    const QJsonArray channelStats{
        QJsonObject{{"name", "MEG 0111"}, {"rms", 1.25}, {"mean_abs", 0.75}, {"peak_abs", 3.5}},
        QJsonObject{{"name", "EEG 001"}, {"rms", 2.0}, {"mean_abs", 1.0}, {"peak_abs", 4.0}}
    };
    widget.setResult("neurokernel.channel_stats",
                     QJsonObject{{"message", "Channel statistics"}, {"channels", channelStats}});
    QCOMPARE(stack->currentWidget(), table);
    QCOMPARE(table->rowCount(), 2);
    QCOMPARE(table->item(0, 0)->text(), QString("MEG 0111"));
    QCOMPARE(table->item(0, 1)->text(), QString("1.25"));
    QCOMPARE(table->item(0, 2)->text(), QString("0.75"));
    QCOMPARE(table->item(0, 3)->text(), QString("3.5"));

    const QJsonArray rawStats{
        QJsonObject{{"name", "MEG 0121"}, {"rms", 9.5}}
    };
    widget.setResult("neurokernel.raw_stats", QJsonObject{{"top_channels", rawStats}});
    QCOMPARE(stack->currentWidget(), table);
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QString("MEG 0121"));
    QCOMPARE(table->item(0, 1)->text(), QString("9.5"));
    QCOMPARE(table->item(0, 2)->text(), QString("-"));
    QCOMPARE(table->item(0, 3)->text(), QString("-"));

    widget.setResult("neurokernel.raw_stats", QJsonObject{{"top_channels", "invalid"}});
    QCOMPARE(stack->currentWidget(), tree);
    QCOMPARE(table->rowCount(), 0);
}

//=============================================================================================================

void TestStudioCore::extensionHostedViewWidget_descriptorCommandsAndUpdates()
{
    ExtensionHostedViewWidget widget;
    QSlider* opacitySlider = widget.findChild<QSlider*>();
    QVERIFY(opacitySlider);

    QSignalSpy commandSpy(&widget, &ExtensionHostedViewWidget::viewCommandRequested);
    QVERIFY(QMetaObject::invokeMethod(opacitySlider, "sliderReleased"));
    QCOMPARE(commandSpy.size(), 0);
    QCOMPARE(widget.sessionId(), QString());
    QCOMPARE(widget.filePath(), QString());

    const QJsonObject descriptor{
        {"session_id", "session-42"},
        {"file", "/tmp/sample-raw.fif"},
        {"provider_display_name", "Surface Viewer"},
        {"extension_display_name", "Dummy 3D"},
        {"slot", "center"},
        {"scene_id", "scene-7"},
        {"message", "Rendering surface"},
        {"state", QJsonObject{{"hemisphere", "left"}, {"camera", "lateral"}, {"opacity", 0.65}}},
        {"controls", QJsonObject{{"opacity", QJsonObject{{"command", "change_opacity"},
                                                          {"target_argument", "alpha"}}}}},
        {"actions", QJsonArray{
            QJsonObject{{"command", "reset_camera"},
                        {"label", "Reset Camera"},
                        {"description", "Restore the default view"},
                        {"arguments", QJsonObject{{"camera", "lateral"}}}},
            QJsonObject{{"label", "Ignored"}}
        }}
    };
    widget.setSessionDescriptor(descriptor);
    QCOMPARE(widget.sessionId(), QString("session-42"));
    QCOMPARE(widget.filePath(), QString("/tmp/sample-raw.fif"));
    QCOMPARE(opacitySlider->value(), 65);

    opacitySlider->setValue(40);
    QVERIFY(QMetaObject::invokeMethod(opacitySlider, "sliderReleased"));
    QCOMPARE(commandSpy.size(), 1);
    QList<QVariant> arguments = commandSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("session-42"));
    QCOMPARE(arguments.at(1).toString(), QString("change_opacity"));
    const QJsonObject expectedOpacityArguments{{"alpha", 0.4}};
    QCOMPARE(arguments.at(2).toJsonObject(), expectedOpacityArguments);

    QPushButton* resetButton = nullptr;
    for(QPushButton* button : widget.findChildren<QPushButton*>()) {
        if(button->text() == QLatin1String("Reset Camera")) {
            resetButton = button;
            break;
        }
    }
    QVERIFY(resetButton);
    resetButton->click();
    QCOMPARE(commandSpy.size(), 1);
    arguments = commandSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("session-42"));
    QCOMPARE(arguments.at(1).toString(), QString("reset_camera"));
    const QJsonObject expectedCameraArguments{{"camera", "lateral"}};
    QCOMPARE(arguments.at(2).toJsonObject(), expectedCameraArguments);

    widget.applySessionUpdate(QJsonObject{{"file", "/tmp/updated.fif"},
                                          {"state", QJsonObject{{"opacity", 0.25}}},
                                          {"actions", QJsonArray()}});
    QCOMPARE(widget.sessionId(), QString("session-42"));
    QCOMPARE(widget.filePath(), QString("/tmp/updated.fif"));
    QCOMPARE(opacitySlider->value(), 25);

    widget.setSessionDescriptor(QJsonObject{{"actions", QJsonArray{QJsonObject{{"command", "refresh"}}}}});
    QPushButton* refreshButton = nullptr;
    for(QPushButton* button : widget.findChildren<QPushButton*>()) {
        if(button->text() == QLatin1String("refresh")) {
            refreshButton = button;
            break;
        }
    }
    QVERIFY(refreshButton);
    refreshButton->click();
    QCOMPARE(commandSpy.size(), 0);
}

//=============================================================================================================

void TestStudioCore::fiffBuffer_reportsUriAndKind()
{
    FiffBuffer buffer("/some/where/example.fif");

    // The uri identifies the buffer to the skill host, so it has to reflect the
    // path it was built with rather than being empty or generic.
    QVERIFY2(!buffer.uri().isEmpty(), "buffer reports no uri");
    QVERIFY2(buffer.uri().contains("example.fif"),
             qPrintable(QString("uri does not identify the file: %1").arg(buffer.uri())));
}

//=============================================================================================================

void TestStudioCore::fiffBuffer_missingFileFailsToOpen()
{
    FiffBuffer buffer("/no/such/directory/missing.fif");

    // Opening something that is not there has to fail and stay closed. Anything
    // else and the skill host would go on to read from an unopened buffer.
    QVERIFY2(!buffer.open(), "opening a missing file reported success");
    QVERIFY2(!buffer.isOpen(), "buffer claims to be open after a failed open");

    // Metadata has to be answerable in that state rather than faulting.
    const QJsonObject metadata = buffer.getMetadata();
    Q_UNUSED(metadata)
}

//=============================================================================================================

void TestStudioCore::fiffBuffer_realFileOpensAndDescribesItself()
{
    if(!QFile::exists(rawPath())) {
        QSKIP("Raw test data not found");
    }

    FiffBuffer buffer(rawPath());

    QVERIFY2(buffer.open(), "failed to open a real fiff file");
    QVERIFY(buffer.isOpen());

    // The metadata is what a skill reads to decide whether it can act on the
    // buffer, so an empty object would make every skill decline.
    const QJsonObject metadata = buffer.getMetadata();
    QVERIFY2(!metadata.isEmpty(), "an opened fiff buffer described itself with nothing");
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestStudioCore)
#include "test_studio_core.moc"
