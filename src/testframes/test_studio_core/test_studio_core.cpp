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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>

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

    void jsonRpc_requestRoundTrips();
    void jsonRpc_responseAndErrorAreDistinct();
    void jsonRpc_rejectsMalformedPayload();

    void planner_modeSelectsProvider_data();
    void planner_modeSelectsProvider();
    void planner_mockModeIsConfiguredWithoutCredentials();
    void planner_httpModeNeedsEndpointAndModel();
    void planner_statusSummaryMentionsMode();

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

QTEST_GUILESS_MAIN(TestStudioCore)
#include "test_studio_core.moc"
