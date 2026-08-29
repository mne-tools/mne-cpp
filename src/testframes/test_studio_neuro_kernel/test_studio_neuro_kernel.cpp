//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_studio_neuro_kernel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     August, 2026
 * @brief    Integration tests for the local Neuro-Kernel JSON-RPC service.
 */

#include <neurokernelservice.h>

#include <jsonrpcmessage.h>

#include <QDeadlineTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QLocalSocket>
#include <QTemporaryFile>
#include <QUuid>
#include <QtTest>

using namespace MNEANALYZESTUDIO;

class TestStudioNeuroKernel : public QObject {
    Q_OBJECT

    private slots:
    void initTestCase();
    void malformedRequest();
    void listTools();
    void executeCommands_data();
    void executeCommands();
    void unknownTool();
    void missingFiles_data();
    void missingFiles();
    void malformedFile();
    void analyzeRawData_data();
    void analyzeRawData();
    void unmatchedChannels_data();
    void unmatchedChannels();

    private:
    void sendRequest(const QJsonObject& request, QJsonObject& response);

    NeuroKernelService m_service;
    QLocalSocket m_socket;
};

void TestStudioNeuroKernel::initTestCase() {
    const QString socketName = QStringLiteral("mne-nk-")
                               + QUuid::createUuid().toString(QUuid::Id128).left(12);
    QVERIFY(m_service.start(socketName));
    m_socket.connectToServer(socketName);
    QVERIFY(m_socket.waitForConnected(5000));
}

void TestStudioNeuroKernel::sendRequest(const QJsonObject& request, QJsonObject& response) {
    m_socket.write(JsonRpcMessage::serialize(request));
    QVERIFY(m_socket.flush());

    QDeadlineTimer deadline(5000);
    while (!m_socket.canReadLine()) {
        QVERIFY(m_socket.waitForReadyRead(deadline.remainingTime()));
    }

    QString errorString;
    QVERIFY2(JsonRpcMessage::deserialize(m_socket.readLine(), response, errorString),
             qPrintable(errorString));
}

void TestStudioNeuroKernel::malformedRequest() {
    m_socket.write("{bad json\n");
    QVERIFY(m_socket.flush());

    QDeadlineTimer deadline(5000);
    while (!m_socket.canReadLine()) {
        QVERIFY(m_socket.waitForReadyRead(deadline.remainingTime()));
    }

    QJsonObject response;
    QString errorString;
    QVERIFY(JsonRpcMessage::deserialize(m_socket.readLine(), response, errorString));
    QCOMPARE(response.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt(),
             -32700);
}

void TestStudioNeuroKernel::listTools() {
    const QJsonObject request = JsonRpcMessage::createRequest(QStringLiteral("list"),
                                                              QStringLiteral("tools/list"));
    QJsonObject response;
    sendRequest(request, response);
    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
    QCOMPARE(result.value(QStringLiteral("tools")).toArray().size(), 5);
    QCOMPARE(result.value(QStringLiteral("transport")).toString(), QStringLiteral("local_socket"));
}

void TestStudioNeuroKernel::executeCommands_data() {
    QTest::addColumn<QString>("command");
    QTest::addColumn<QString>("status");

    QTest::newRow("implicit help") << QString() << QStringLiteral("ok");
    QTest::newRow("help") << QStringLiteral("help") << QStringLiteral("ok");
    QTest::newRow("status") << QStringLiteral("status") << QStringLiteral("ok");
    QTest::newRow("version") << QStringLiteral("version") << QStringLiteral("ok");
    QTest::newRow("unknown") << QStringLiteral("unknown") << QStringLiteral("error");
}

void TestStudioNeuroKernel::executeCommands() {
    QFETCH(QString, command);
    QFETCH(QString, status);

    const QJsonObject params{
    {QStringLiteral("name"), QStringLiteral("neurokernel.execute")},
    {QStringLiteral("arguments"), QJsonObject{{QStringLiteral("command"), command}}}};
    const QJsonObject request = JsonRpcMessage::createRequest(QStringLiteral("execute"),
                                                              QStringLiteral("tools/call"),
                                                              params);
    QJsonObject response;
    sendRequest(request, response);
    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("status")).toString(), status);
    QCOMPARE(result.value(QStringLiteral("tool_name")).toString(),
             QStringLiteral("neurokernel.execute"));
}

void TestStudioNeuroKernel::unknownTool() {
    const QJsonObject params{
    {QStringLiteral("name"), QStringLiteral("neurokernel.unknown")},
    {QStringLiteral("arguments"), QJsonObject()}};
    const QJsonObject request = JsonRpcMessage::createRequest(QStringLiteral("unknown"),
                                                              QStringLiteral("tools/call"),
                                                              params);
    QJsonObject response;
    sendRequest(request, response);
    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("status")).toString(), QStringLiteral("ignored"));
}

void TestStudioNeuroKernel::missingFiles_data() {
    QTest::addColumn<QString>("toolName");

    QTest::newRow("raw stats") << QStringLiteral("neurokernel.raw_stats");
    QTest::newRow("channel stats") << QStringLiteral("neurokernel.channel_stats");
    QTest::newRow("peak window") << QStringLiteral("neurokernel.find_peak_window");
    QTest::newRow("psd") << QStringLiteral("neurokernel.psd_summary");
}

void TestStudioNeuroKernel::missingFiles() {
    QFETCH(QString, toolName);

    const QJsonObject arguments{{QStringLiteral("file"), QStringLiteral("missing-raw.fif")}};
    const QJsonObject params{
    {QStringLiteral("name"), toolName},
    {QStringLiteral("arguments"), arguments}};
    const QJsonObject request = JsonRpcMessage::createRequest(QStringLiteral("missing"),
                                                              QStringLiteral("tools/call"),
                                                              params);
    QJsonObject response;
    sendRequest(request, response);
    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("status")).toString(), QStringLiteral("error"));
    QVERIFY(result.value(QStringLiteral("message")).toString().contains(QStringLiteral("could not find")));
}

void TestStudioNeuroKernel::malformedFile() {
    QTemporaryFile malformedFile;
    QVERIFY(malformedFile.open());
    QCOMPARE(malformedFile.write("not a fiff file"), 15);
    QVERIFY(malformedFile.flush());

    const QJsonObject arguments{{QStringLiteral("file"), malformedFile.fileName()}};
    const QJsonObject params{
    {QStringLiteral("name"), QStringLiteral("neurokernel.raw_stats")},
    {QStringLiteral("arguments"), arguments}};
    const QJsonObject request = JsonRpcMessage::createRequest(QStringLiteral("malformed"),
                                                              QStringLiteral("tools/call"),
                                                              params);
    QJsonObject response;
    sendRequest(request, response);
    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("status")).toString(), QStringLiteral("error"));
    QVERIFY(result.value(QStringLiteral("message")).toString().contains(QStringLiteral("failed")));
}

void TestStudioNeuroKernel::analyzeRawData_data() {
    QTest::addColumn<QString>("toolName");
    QTest::addColumn<QJsonObject>("extraArguments");

    QTest::newRow("raw stats")
    << QStringLiteral("neurokernel.raw_stats")
    << QJsonObject();
    QTest::newRow("channel stats")
    << QStringLiteral("neurokernel.channel_stats")
    << QJsonObject{{QStringLiteral("match"), QStringLiteral("EEG")},
                   {QStringLiteral("limit"), 2}};
    QTest::newRow("peak window")
    << QStringLiteral("neurokernel.find_peak_window")
    << QJsonObject{{QStringLiteral("match"), QStringLiteral("MEG")}};
    QTest::newRow("psd")
    << QStringLiteral("neurokernel.psd_summary")
    << QJsonObject{{QStringLiteral("match"), QStringLiteral("EEG")},
                   {QStringLiteral("nfft"), 64}};
}

void TestStudioNeuroKernel::analyzeRawData() {
    QFETCH(QString, toolName);
    QFETCH(QJsonObject, extraArguments);

    const QString rawPath = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/MEG/sample/"
                                                                                    "sample_audvis_trunc_raw.fif");
    if (!QFileInfo::exists(rawPath)) {
        QSKIP("Sample raw data not available");
    }

    QJsonObject arguments = extraArguments;
    arguments.insert(QStringLiteral("file"), rawPath);
    arguments.insert(QStringLiteral("from_sample"), 12900);
    arguments.insert(QStringLiteral("to_sample"), 13155);
    const QJsonObject params{
    {QStringLiteral("name"), toolName},
    {QStringLiteral("arguments"), arguments}};
    const QJsonObject request = JsonRpcMessage::createRequest(QStringLiteral("analysis"),
                                                              QStringLiteral("tools/call"),
                                                              params);
    QJsonObject response;
    sendRequest(request, response);
    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
    QCOMPARE(result.value(QStringLiteral("tool_name")).toString(), toolName);
    QCOMPARE(result.value(QStringLiteral("from_sample")).toInt(), 12900);
    QCOMPARE(result.value(QStringLiteral("to_sample")).toInt(), 13155);

    if (toolName == QStringLiteral("neurokernel.raw_stats")) {
        QVERIFY(result.value(QStringLiteral("channel_count")).toInt() > 0);
        QCOMPARE(result.value(QStringLiteral("sample_count")).toInt(), 256);
        QCOMPARE(result.value(QStringLiteral("top_channels")).toArray().size(), 3);
    } else if (toolName == QStringLiteral("neurokernel.channel_stats")) {
        QCOMPARE(result.value(QStringLiteral("channel_count")).toInt(), 2);
        QCOMPARE(result.value(QStringLiteral("channels")).toArray().size(), 2);
    } else if (toolName == QStringLiteral("neurokernel.find_peak_window")) {
        QVERIFY(result.value(QStringLiteral("peak_channel")).toString().contains(QStringLiteral("MEG")));
        QVERIFY(result.value(QStringLiteral("peak_abs")).toDouble() > 0.0);
    } else {
        QCOMPARE(result.value(QStringLiteral("nfft")).toInt(), 64);
        QVERIFY(!result.value(QStringLiteral("frequencies")).toArray().isEmpty());
        QCOMPARE(result.value(QStringLiteral("frequencies")).toArray().size(),
                 result.value(QStringLiteral("psd")).toArray().size());
    }
}

void TestStudioNeuroKernel::unmatchedChannels_data() {
    QTest::addColumn<QString>("toolName");

    QTest::newRow("peak window") << QStringLiteral("neurokernel.find_peak_window");
    QTest::newRow("psd") << QStringLiteral("neurokernel.psd_summary");
}

void TestStudioNeuroKernel::unmatchedChannels() {
    QFETCH(QString, toolName);

    const QString rawPath = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/MEG/sample/"
                                                                                    "sample_audvis_trunc_raw.fif");
    if (!QFileInfo::exists(rawPath)) {
        QSKIP("Sample raw data not available");
    }

    const QJsonObject arguments{
    {QStringLiteral("file"), rawPath},
    {QStringLiteral("from_sample"), 12900},
    {QStringLiteral("to_sample"), 13155},
    {QStringLiteral("match"), QStringLiteral("NOT_A_CHANNEL")}};
    const QJsonObject params{
    {QStringLiteral("name"), toolName},
    {QStringLiteral("arguments"), arguments}};
    const QJsonObject request = JsonRpcMessage::createRequest(QStringLiteral("unmatched"),
                                                              QStringLiteral("tools/call"),
                                                              params);
    QJsonObject response;
    sendRequest(request, response);
    const QJsonObject result = response.value(QStringLiteral("result")).toObject();
    QCOMPARE(result.value(QStringLiteral("status")).toString(), QStringLiteral("error"));
    QVERIFY(result.value(QStringLiteral("message")).toString().contains(QStringLiteral("could not find")));
}

QTEST_GUILESS_MAIN(TestStudioNeuroKernel)
#include "test_studio_neuro_kernel.moc"