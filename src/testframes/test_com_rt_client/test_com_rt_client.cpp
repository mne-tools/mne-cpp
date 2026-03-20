//=============================================================================================================
/**
 * @file     test_com_rt_client.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Tests for com rt_client classes — RtCmdClient, RtDataClient,
 *           and RtClient offline construction and state management
 *           (no network server required).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <com/rt_client/rt_cmd_client.h>
#include <com/rt_client/rt_data_client.h>
#include <com/rt_client/rt_client.h>
#include <com/rt_command/command.h>
#include <com/rt_command/command_manager.h>
#include <com/rt_command/command_parser.h>
#include <com/rt_command/raw_command.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QAbstractSocket>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMLIB;

//=============================================================================================================
/**
 * @brief Offline tests for RtCmdClient, RtDataClient, RtClient construction
 *        and state management; additional Command/CommandManager edge cases.
 */
class TestComRtClient : public QObject
{
    Q_OBJECT

private slots:
    // ── RtCmdClient (offline) ─────────────────────────────────────────
    void testRtCmdClientConstruction();
    void testRtCmdClientHasCommandEmpty();
    void testRtCmdClientReadAvailableDataEmpty();
    void testRtCmdClientResponseSignal();
    void testRtCmdClientDisconnectedState();

    // ── RtDataClient (offline) ────────────────────────────────────────
    void testRtDataClientConstruction();
    void testRtDataClientDisconnectedState();
    void testRtDataClientSetClientAlias();

    // ── RtClient (offline) ────────────────────────────────────────────
    void testRtClientConstruction();
    void testRtClientConnectionStatusOffline();
    void testRtClientStopBeforeStart();
    void testRtClientConnectionChangedSignal();

    // ── MetaData struct ───────────────────────────────────────────────
    void testMetaDataConstruction();

    // ── Command edge cases ────────────────────────────────────────────
    void testCommandDirectMemberAccess();
    void testCommandReserializeRoundTrip();
    void testCommandEmptyParamsToJson();
    void testCommandMultipleParamTypes();
    void testCommandExecuteSignalPayload();

    // ── RawCommand edge cases ─────────────────────────────────────────
    void testRawCommandMultipleParams();
    void testRawCommandExecuteSignalPayload();
    void testRawCommandCopyIndependence();

    // ── CommandManager edge cases ─────────────────────────────────────
    void testCommandManagerOverwriteCommand();
    void testCommandManagerInsertMultipleFromJson();
    void testCommandManagerInactiveSkipsParsing();
    void testCommandManagerCommandMapAccess();

    // ── CommandParser edge cases ──────────────────────────────────────
    void testCommandParserMultipleSequentialParses();
    void testCommandParserCLIWithMultipleParams();
    void testCommandParserJsonWithMultipleCommands();
};

//=============================================================================================================
// RtCmdClient (offline)
//=============================================================================================================

void TestComRtClient::testRtCmdClientConstruction()
{
    RtCmdClient client;
    // Should be disconnected, not connected to any server
    QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);
}

//=============================================================================================================

void TestComRtClient::testRtCmdClientHasCommandEmpty()
{
    RtCmdClient client;
    // No commands loaded
    QVERIFY(!client.hasCommand("help"));
    QVERIFY(!client.hasCommand("stop"));
    QVERIFY(!client.hasCommand(""));
}

//=============================================================================================================

void TestComRtClient::testRtCmdClientReadAvailableDataEmpty()
{
    RtCmdClient client;
    // No data available before any connection
    QString data = client.readAvailableData();
    QVERIFY(data.isEmpty());
}

//=============================================================================================================

void TestComRtClient::testRtCmdClientResponseSignal()
{
    RtCmdClient client;
    // Verify the response signal is connectable
    QSignalSpy spy(&client, &RtCmdClient::response);
    QVERIFY(spy.isValid());
}

//=============================================================================================================

void TestComRtClient::testRtCmdClientDisconnectedState()
{
    RtCmdClient client;
    QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);
    QVERIFY(!client.isOpen());
    QCOMPARE(client.peerPort(), quint16(0));
}

//=============================================================================================================
// RtDataClient (offline)
//=============================================================================================================

void TestComRtClient::testRtDataClientConstruction()
{
    RtDataClient client;
    QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);
}

//=============================================================================================================

void TestComRtClient::testRtDataClientDisconnectedState()
{
    RtDataClient client;
    QVERIFY(!client.isOpen());
    QCOMPARE(client.peerPort(), quint16(0));
    // disconnectFromHost on already disconnected socket should not crash
    client.disconnectFromHost();
    QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);
}

//=============================================================================================================

void TestComRtClient::testRtDataClientSetClientAlias()
{
    RtDataClient client;
    // setClientAlias should not crash when disconnected
    client.setClientAlias("test_alias");
    QVERIFY(true); // No crash
}

//=============================================================================================================
// RtClient (offline)
//=============================================================================================================

void TestComRtClient::testRtClientConstruction()
{
    RtClient client("127.0.0.1", "test_client");
    // Should not be running
    QVERIFY(!client.isRunning());
}

//=============================================================================================================

void TestComRtClient::testRtClientConnectionStatusOffline()
{
    RtClient client("127.0.0.1", "test_client");
    QVERIFY(!client.getConnectionStatus());
}

//=============================================================================================================

void TestComRtClient::testRtClientStopBeforeStart()
{
    RtClient client("127.0.0.1", "test_client");
    // Stopping a thread that was never started should be safe
    bool stopped = client.stop();
    QVERIFY(stopped);
    QVERIFY(!client.isRunning());
}

//=============================================================================================================

void TestComRtClient::testRtClientConnectionChangedSignal()
{
    RtClient client("127.0.0.1", "test_client");
    QSignalSpy spy(&client, &RtClient::connectionChanged);
    QVERIFY(spy.isValid());
}

//=============================================================================================================
// MetaData struct
//=============================================================================================================

void TestComRtClient::testMetaDataConstruction()
{
    FIFFLIB::FiffInfo::SPtr pInfo = FIFFLIB::FiffInfo::SPtr::create();
    FIFFLIB::FiffDigitizerData::SPtr pDigData = FIFFLIB::FiffDigitizerData::SPtr::create();

    MetaData meta(pInfo, pDigData);
    QVERIFY(meta.m_pInfo != nullptr);
    QVERIFY(meta.m_pDigitizerData != nullptr);
    QCOMPARE(meta.m_pInfo, pInfo);
    QCOMPARE(meta.m_pDigitizerData, pDigData);
}

//=============================================================================================================
// Command edge cases
//=============================================================================================================

void TestComRtClient::testCommandDirectMemberAccess()
{
    Command cmd("test", "A test command");
    // Public members should be directly accessible
    QCOMPARE(cmd.m_sCommand, QString("test"));
    QCOMPARE(cmd.m_sDescription, QString("A test command"));
    QVERIFY(cmd.m_qListParamNames.isEmpty());
    QVERIFY(cmd.m_qListParamValues.isEmpty());
    QVERIFY(cmd.m_qListParamDescriptions.isEmpty());
    QVERIFY(cmd.m_bIsJson);
}

//=============================================================================================================

void TestComRtClient::testCommandReserializeRoundTrip()
{
    // Build a command with params
    QStringList names = {"id", "value"};
    QList<QVariant> values = {QVariant(42), QVariant(QString("hello"))};
    QStringList descs = {"Client ID", "Setting value"};

    Command original("set", "Set a value", names, values, descs, true);

    // Serialize to JSON
    QJsonObject json = original.toJsonObject();

    // Reconstruct from JSON
    Command reconstructed("set", json, true);

    QCOMPARE(reconstructed.command(), original.command());
    QCOMPARE(reconstructed.description(), original.description());
    QCOMPARE(reconstructed.count(), original.count());
    QCOMPARE(reconstructed.pNames().size(), original.pNames().size());
    QCOMPARE(reconstructed.pDescriptions().at(0), QString("Client ID"));
}

//=============================================================================================================

void TestComRtClient::testCommandEmptyParamsToJson()
{
    Command cmd("help", "Show help", true);
    QJsonObject json = cmd.toJsonObject();

    QCOMPARE(json.value("description").toString(), QString("Show help"));
    QJsonObject params = json.value("parameters").toObject();
    QVERIFY(params.isEmpty());
}

//=============================================================================================================

void TestComRtClient::testCommandMultipleParamTypes()
{
    QStringList names = {"intParam", "floatParam", "stringParam", "boolParam"};
    QList<QVariant> values = {QVariant(100), QVariant(3.14), QVariant(QString("text")), QVariant(true)};

    Command cmd("multi", "Multi-type params", names, values, true);
    QCOMPARE(cmd.count(), quint32(4));
    QCOMPARE(cmd["intParam"].toInt(), 100);
    QVERIFY(qAbs(cmd["floatParam"].toDouble() - 3.14) < 0.001);
    QCOMPARE(cmd["stringParam"].toString(), QString("text"));
    QCOMPARE(cmd["boolParam"].toBool(), true);

    // toStringReadySend should include all params
    QString ready = cmd.toStringReadySend();
    QVERIFY(ready.contains("multi"));
    QVERIFY(ready.contains("intParam"));
    QVERIFY(ready.contains("100"));
}

//=============================================================================================================

void TestComRtClient::testCommandExecuteSignalPayload()
{
    Command cmd("test", "Test");
    QSignalSpy spy(&cmd, &Command::executed);
    cmd.execute();
    QCOMPARE(spy.count(), 1);

    // The signal carries the command itself
    Command signalCmd = spy.at(0).at(0).value<Command>();
    QCOMPARE(signalCmd.command(), QString("test"));
}

//=============================================================================================================
// RawCommand edge cases
//=============================================================================================================

void TestComRtClient::testRawCommandMultipleParams()
{
    RawCommand raw("set", false);
    raw.pValues().append("100");
    raw.pValues().append("200");
    raw.pValues().append("300");
    QCOMPARE(raw.count(), quint32(3));
    QCOMPARE(raw.pValues().at(0), QString("100"));
    QCOMPARE(raw.pValues().at(2), QString("300"));
}

//=============================================================================================================

void TestComRtClient::testRawCommandExecuteSignalPayload()
{
    RawCommand raw("test", false);
    raw.pValues().append("param1");
    raw.pValues().append("param2");

    QSignalSpy spy(&raw, &RawCommand::executed);
    raw.execute();
    QCOMPARE(spy.count(), 1);

    // The signal carries parameter list
    QList<QString> params = spy.at(0).at(0).value<QList<QString>>();
    QCOMPARE(params.size(), 2);
    QCOMPARE(params.at(0), QString("param1"));
}

//=============================================================================================================

void TestComRtClient::testRawCommandCopyIndependence()
{
    RawCommand original("test", true);
    original.pValues().append("val");

    RawCommand copy(original);
    copy.pValues().append("extra");

    // Original should be unaffected
    QCOMPARE(original.count(), quint32(1));
    QCOMPARE(copy.count(), quint32(2));
}

//=============================================================================================================
// CommandManager edge cases
//=============================================================================================================

void TestComRtClient::testCommandManagerOverwriteCommand()
{
    CommandManager mgr;
    mgr.insert("test", "First description");
    QCOMPARE(mgr["test"].description(), QString("First description"));

    // Overwrite with new description
    mgr.insert("test", "Second description");
    QCOMPARE(mgr["test"].description(), QString("Second description"));

    // Should still be only 1 command
    QCOMPARE(mgr.commandMap().size(), 1);
}

//=============================================================================================================

void TestComRtClient::testCommandManagerInsertMultipleFromJson()
{
    // Build JSON doc with multiple commands
    QJsonObject cmd1Content;
    cmd1Content.insert("description", QString("Start acquisition"));
    cmd1Content.insert("parameters", QJsonObject());

    QJsonObject cmd2Content;
    cmd2Content.insert("description", QString("Stop acquisition"));
    cmd2Content.insert("parameters", QJsonObject());

    QJsonObject cmd3Content;
    cmd3Content.insert("description", QString("Show help"));
    cmd3Content.insert("parameters", QJsonObject());

    QJsonObject cmds;
    cmds.insert("start", cmd1Content);
    cmds.insert("stop", cmd2Content);
    cmds.insert("help", cmd3Content);

    QJsonObject root;
    root.insert("commands", cmds);
    QJsonDocument doc(root);

    CommandManager mgr(doc);
    QCOMPARE(mgr.commandMap().size(), 3);
    QVERIFY(mgr.hasCommand("start"));
    QVERIFY(mgr.hasCommand("stop"));
    QVERIFY(mgr.hasCommand("help"));
    QCOMPARE(mgr["start"].description(), QString("Start acquisition"));
}

//=============================================================================================================

void TestComRtClient::testCommandManagerInactiveSkipsParsing()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");

    CommandParser parser;
    parser.attach(&mgr);

    // Deactivate the manager
    mgr.setStatus(false);
    QVERIFY(!mgr.isActive());

    QSignalSpy spy(&mgr["help"], &Command::executed);

    // Parse should succeed (parser finds command) but manager won't execute
    QStringList parsed;
    parser.parse("help", parsed);
    // Command should NOT have been executed because manager is inactive
    QCOMPARE(spy.count(), 0);
}

//=============================================================================================================

void TestComRtClient::testCommandManagerCommandMapAccess()
{
    CommandManager mgr;
    mgr.insert("alpha", "First");
    mgr.insert("beta", "Second");
    mgr.insert("gamma", "Third");

    QMap<QString, Command>& map = mgr.commandMap();
    QCOMPARE(map.size(), 3);
    QVERIFY(map.contains("alpha"));
    QVERIFY(map.contains("beta"));
    QVERIFY(map.contains("gamma"));
    QCOMPARE(map["alpha"].description(), QString("First"));
}

//=============================================================================================================
// CommandParser edge cases
//=============================================================================================================

void TestComRtClient::testCommandParserMultipleSequentialParses()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");
    mgr.insert("stop", "Stop");

    CommandParser parser;
    parser.attach(&mgr);

    QStringList parsed;

    // Parse help
    QVERIFY(parser.parse("help", parsed));
    QCOMPARE(parsed.size(), 1);
    QCOMPARE(parsed.at(0), QString("help"));

    // Parse stop
    parsed.clear();
    QVERIFY(parser.parse("stop", parsed));
    QCOMPARE(parsed.size(), 1);
    QCOMPARE(parsed.at(0), QString("stop"));

    // Parse help again
    parsed.clear();
    QVERIFY(parser.parse("help", parsed));
    QCOMPARE(parsed.at(0), QString("help"));
}

//=============================================================================================================

void TestComRtClient::testCommandParserCLIWithMultipleParams()
{
    QStringList names = {"x", "y", "z"};
    QList<QVariant> values = {QVariant(int(0)), QVariant(int(0)), QVariant(int(0))};

    CommandManager mgr;
    Command cmd("move", "Move to position", names, values, false);
    mgr.insert("move", cmd);

    CommandParser parser;
    parser.attach(&mgr);

    QStringList parsed;
    QVERIFY(parser.parse("move 10 20 30", parsed));
    QCOMPARE(parsed.at(0), QString("move"));
    QCOMPARE(mgr["move"][qint32(0)].toInt(), 10);
    QCOMPARE(mgr["move"][qint32(1)].toInt(), 20);
    QCOMPARE(mgr["move"][qint32(2)].toInt(), 30);
}

//=============================================================================================================

void TestComRtClient::testCommandParserJsonWithMultipleCommands()
{
    CommandManager mgr;
    mgr.insert("start", "Start");
    mgr.insert("stop", "Stop");

    CommandParser parser;
    parser.attach(&mgr);

    // JSON with multiple commands
    QString jsonInput = "{\"commands\":{\"start\":{},\"stop\":{}}}";
    QStringList parsed;
    QVERIFY(parser.parse(jsonInput, parsed));
    QCOMPARE(parsed.size(), 2);
    QVERIFY(parsed.contains("start"));
    QVERIFY(parsed.contains("stop"));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestComRtClient)
#include "test_com_rt_client.moc"
