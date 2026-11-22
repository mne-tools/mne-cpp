//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2010-2026 MNE-CPP Authors
 *

 * @file     test_rtserver_commands.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 *
 * @brief    Tests for the command vocabulary of mne_rt_server: Command, RawCommand, CommandParser, CommandManager
 */
//=============================================================================================================

#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QVariant>

#include <rtserver_connector/command.h>
#include <rtserver_connector/raw_command.h>
#include <rtserver_connector/command_parser.h>
#include <rtserver_connector/command_manager.h>

using namespace RTSERVER;

//=============================================================================================================

class TestRtServerCommands : public QObject
{
    Q_OBJECT

private slots:
    // ---- Command tests ----
    void testCommandDefaultConstruction();
    void testCommandNameDescription();
    void testCommandWithParams();
    void testCommandWithParamsAndDescriptions();
    void testCommandFromJson();
    void testCommandCopy();
    void testCommandAssignment();
    void testCommandToJsonObject();
    void testCommandToStringList();
    void testCommandToStringReadySend();
    void testCommandSubscriptByName();
    void testCommandSubscriptByIndex();
    void testCommandConstSubscript();
    void testCommandExecuteSignal();
    void testCommandIsJson();

    // ---- RawCommand tests ----
    void testRawCommandDefault();
    void testRawCommandFromString_CLI();
    void testRawCommandFromString_JSON();
    void testRawCommandCopy();
    void testRawCommandAssignment();
    void testRawCommandExecuteSignal();

    // ---- CommandParser tests ----
    void testCommandParserCLI();
    void testCommandParserJSON();
    void testCommandParserEmpty();
    void testCommandParserUnknown();
    void testCommandParserExists();

    // ---- CommandManager tests ----
    void testCommandManagerDefault();
    void testCommandManagerInsertKeyDesc();
    void testCommandManagerInsertCommand();
    void testCommandManagerInsertJsonDoc();
    void testCommandManagerHasCommand();
    void testCommandManagerClear();
    void testCommandManagerSubscript();
    void testCommandManagerActiveToggle();
    void testCommandManagerCommandMapChanged();
    void testCommandManagerUpdate();

    // ---- Gap-fill tests ----
    void testCommandReply();
    void testCommandSend();
    void testCommandParserGetRawCommand();
    void testCommandManagerFromByteArray();
    void testCommandManagerTriggeredSignal();
    void testCommandManagerResponseSignal();
    void testCommandParserResponseSignal();

    // ---- Integration: Parser + Manager ----
    void testParserManagerIntegration_CLI();
    void testParserManagerIntegration_JSON();

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
// Command tests
//=============================================================================================================

void TestRtServerCommands::testCommandDefaultConstruction()
{
    Command cmd;
    QCOMPARE(cmd.command(), QString(""));
    QCOMPARE(cmd.description(), QString(""));
    QCOMPARE(cmd.count(), quint32(0));
    QVERIFY(cmd.isJson());
}

void TestRtServerCommands::testCommandNameDescription()
{
    Command cmd("help", "Show help text", false);
    QCOMPARE(cmd.command(), QString("help"));
    QCOMPARE(cmd.description(), QString("Show help text"));
    QCOMPARE(cmd.count(), quint32(0));
    QVERIFY(!cmd.isJson());
}

void TestRtServerCommands::testCommandWithParams()
{
    QStringList names;
    names << "id" << "value";
    QList<QVariant> values;
    values << QVariant(int(42)) << QVariant(QString("test"));

    Command cmd("set", "Set a value", names, values, true);
    QCOMPARE(cmd.count(), quint32(2));
    QCOMPARE(cmd.pNames().size(), 2);
    QCOMPARE(cmd.pNames().at(0), QString("id"));
    QCOMPARE(cmd.pNames().at(1), QString("value"));
    QCOMPARE(cmd.pValues().at(0).toInt(), 42);
    QCOMPARE(cmd.pValues().at(1).toString(), QString("test"));
    // Auto-generated descriptions should be empty strings
    QCOMPARE(cmd.pDescriptions().size(), 2);
    QCOMPARE(cmd.pDescriptions().at(0), QString(""));
}

void TestRtServerCommands::testCommandWithParamsAndDescriptions()
{
    QStringList names;
    names << "id";
    QList<QVariant> values;
    values << QVariant(int(0));
    QStringList descs;
    descs << "Client ID";

    Command cmd("get", "Get info", names, values, descs, true);
    QCOMPARE(cmd.count(), quint32(1));
    QCOMPARE(cmd.pDescriptions().at(0), QString("Client ID"));
}

void TestRtServerCommands::testCommandFromJson()
{
    QJsonObject paramObj;
    QJsonObject idParam;
    idParam.insert("type", QString("int"));
    idParam.insert("description", QString("Client ID"));
    QJsonObject params;
    params.insert("id", idParam);

    QJsonObject content;
    content.insert("description", QString("Get measurement info"));
    content.insert("parameters", params);

    Command cmd("measinfo", content, true);
    QCOMPARE(cmd.command(), QString("measinfo"));
    QCOMPARE(cmd.description(), QString("Get measurement info"));
    QCOMPARE(cmd.count(), quint32(1));
    QCOMPARE(cmd.pNames().at(0), QString("id"));
    QCOMPARE(cmd.pDescriptions().at(0), QString("Client ID"));
}

void TestRtServerCommands::testCommandCopy()
{
    QStringList names;
    names << "x";
    QList<QVariant> values;
    values << QVariant(3.14);

    Command original("test", "A test", names, values, true);
    Command copy(original);

    QCOMPARE(copy.command(), original.command());
    QCOMPARE(copy.description(), original.description());
    QCOMPARE(copy.count(), original.count());
    QCOMPARE(copy.pValues().at(0).toDouble(), 3.14);
}

void TestRtServerCommands::testCommandAssignment()
{
    Command a("alpha", "First");
    Command b("beta", "Second");
    b = a;
    QCOMPARE(b.command(), QString("alpha"));
    QCOMPARE(b.description(), QString("First"));
}

void TestRtServerCommands::testCommandToJsonObject()
{
    QStringList names;
    names << "id";
    QList<QVariant> values;
    values << QVariant(int(0));
    QStringList descs;
    descs << "Client ID";

    Command cmd("measinfo", "Get info", names, values, descs, true);
    QJsonObject json = cmd.toJsonObject();

    QCOMPARE(json.value("description").toString(), QString("Get info"));
    QVERIFY(json.contains("parameters"));

    QJsonObject params = json.value("parameters").toObject();
    QVERIFY(params.contains("id"));
    QCOMPARE(params.value("id").toObject().value("description").toString(), QString("Client ID"));
}

void TestRtServerCommands::testCommandToStringList()
{
    QStringList names;
    names << "id";
    QList<QVariant> values;
    values << QVariant(int(0));
    QStringList descs;
    descs << "Client ID";

    Command cmd("measinfo", "Get info", names, values, descs, true);
    QStringList sl = cmd.toStringList();

    QCOMPARE(sl.size(), 3);
    QCOMPARE(sl.at(0), QString("measinfo"));
    QVERIFY(sl.at(1).contains("Client ID"));
    QCOMPARE(sl.at(2), QString("Get info"));
}

void TestRtServerCommands::testCommandToStringReadySend()
{
    QStringList names;
    names << "id";
    QList<QVariant> values;
    values << QVariant(int(42));

    Command cmd("measinfo", "Get info", names, values, true);
    QString ready = cmd.toStringReadySend();

    // Format: "measinfo":{"id":"42"}
    QVERIFY(ready.contains("measinfo"));
    QVERIFY(ready.contains("id"));
    QVERIFY(ready.contains("42"));
}

void TestRtServerCommands::testCommandSubscriptByName()
{
    QStringList names;
    names << "id" << "value";
    QList<QVariant> values;
    values << QVariant(int(1)) << QVariant(QString("abc"));

    Command cmd("set", "Set", names, values, true);
    QCOMPARE(cmd["id"].toInt(), 1);
    QCOMPARE(cmd["value"].toString(), QString("abc"));

    // Modify through subscript
    cmd["id"].setValue(99);
    QCOMPARE(cmd["id"].toInt(), 99);
}

void TestRtServerCommands::testCommandSubscriptByIndex()
{
    QStringList names;
    names << "x";
    QList<QVariant> values;
    values << QVariant(int(5));

    Command cmd("get", "Get", names, values, true);
    QCOMPARE(cmd[qint32(0)].toInt(), 5);

    cmd[qint32(0)].setValue(10);
    QCOMPARE(cmd[qint32(0)].toInt(), 10);
}

void TestRtServerCommands::testCommandConstSubscript()
{
    QStringList names;
    names << "id";
    QList<QVariant> values;
    values << QVariant(int(7));

    Command cmd("get", "Get", names, values, true);
    const Command& cref = cmd;
    QVariant val = cref["id"];
    QCOMPARE(val.toInt(), 7);
}

void TestRtServerCommands::testCommandExecuteSignal()
{
    Command cmd("test", "Test");
    QSignalSpy spy(&cmd, &Command::executed);
    cmd.execute();
    QCOMPARE(spy.count(), 1);
}

void TestRtServerCommands::testCommandIsJson()
{
    Command cmd1(true);
    QVERIFY(cmd1.isJson());

    Command cmd2(false);
    QVERIFY(!cmd2.isJson());

    // isJson returns a reference, so we can toggle it
    cmd2.isJson() = true;
    QVERIFY(cmd2.isJson());
}

//=============================================================================================================
// RawCommand tests
//=============================================================================================================

void TestRtServerCommands::testRawCommandDefault()
{
    RawCommand raw;
    QCOMPARE(raw.command(), QString(""));
    QCOMPARE(raw.count(), quint32(0));
    QVERIFY(!raw.isJson());
}

void TestRtServerCommands::testRawCommandFromString_CLI()
{
    RawCommand raw("help", false);
    QCOMPARE(raw.command(), QString("help"));
    QVERIFY(!raw.isJson());
    QCOMPARE(raw.count(), quint32(0));

    // Add parameters manually
    raw.pValues().append("param1");
    raw.pValues().append("param2");
    QCOMPARE(raw.count(), quint32(2));
    QCOMPARE(raw.pValues().at(0), QString("param1"));
}

void TestRtServerCommands::testRawCommandFromString_JSON()
{
    RawCommand raw("measinfo", true);
    QCOMPARE(raw.command(), QString("measinfo"));
    QVERIFY(raw.isJson());
}

void TestRtServerCommands::testRawCommandCopy()
{
    RawCommand original("test", true);
    original.pValues().append("val1");

    RawCommand copy(original);
    QCOMPARE(copy.command(), QString("test"));
    QVERIFY(copy.isJson());
    QCOMPARE(copy.count(), quint32(1));
    QCOMPARE(copy.pValues().at(0), QString("val1"));
}

void TestRtServerCommands::testRawCommandAssignment()
{
    RawCommand a("alpha", true);
    a.pValues().append("x");

    RawCommand b;
    b = a;
    QCOMPARE(b.command(), QString("alpha"));
    QCOMPARE(b.count(), quint32(1));
    QCOMPARE(b.pValues().at(0), QString("x"));
}

void TestRtServerCommands::testRawCommandExecuteSignal()
{
    RawCommand raw("test", false);
    raw.pValues().append("p1");
    QSignalSpy spy(&raw, &RawCommand::executed);
    raw.execute();
    QCOMPARE(spy.count(), 1);
}

//=============================================================================================================
// CommandParser tests
//=============================================================================================================

void TestRtServerCommands::testCommandParserCLI()
{
    CommandManager manager;
    manager.insert("help", "Show help");

    CommandParser parser;
    parser.attach(&manager);

    QStringList parsed;
    bool result = parser.parse("help", parsed);
    QVERIFY(result);
    QCOMPARE(parsed.size(), 1);
    QCOMPARE(parsed.at(0), QString("help"));
}

void TestRtServerCommands::testCommandParserJSON()
{
    CommandManager manager;
    manager.insert("stop", "Stop");

    CommandParser parser;
    parser.attach(&manager);

    QString jsonInput = "{\"commands\":{\"stop\":{}}}";
    QStringList parsed;
    bool result = parser.parse(jsonInput, parsed);
    QVERIFY(result);
    QCOMPARE(parsed.size(), 1);
    QCOMPARE(parsed.at(0), QString("stop"));
}

void TestRtServerCommands::testCommandParserEmpty()
{
    CommandParser parser;
    QStringList parsed;
    QVERIFY(!parser.parse("", parsed));
}

void TestRtServerCommands::testCommandParserUnknown()
{
    CommandManager manager;
    manager.insert("help", "Show help");

    CommandParser parser;
    parser.attach(&manager);

    QStringList parsed;
    QVERIFY(!parser.parse("unknown_command", parsed));
    QCOMPARE(parsed.size(), 0);
}

void TestRtServerCommands::testCommandParserExists()
{
    CommandManager manager;
    manager.insert("help", "Show help");

    CommandParser parser;
    parser.attach(&manager);

    QVERIFY(parser.exists("help"));
    QVERIFY(!parser.exists("nonexistent"));
}

//=============================================================================================================
// CommandManager tests
//=============================================================================================================

void TestRtServerCommands::testCommandManagerDefault()
{
    CommandManager mgr;
    QVERIFY(mgr.isActive());
    QVERIFY(mgr.commandMap().isEmpty());
}

void TestRtServerCommands::testCommandManagerInsertKeyDesc()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");

    QVERIFY(mgr.hasCommand("help"));
    QCOMPARE(mgr["help"].command(), QString("help"));
    QCOMPARE(mgr["help"].description(), QString("Show help"));
}

void TestRtServerCommands::testCommandManagerInsertCommand()
{
    CommandManager mgr;
    Command cmd("stop", "Stop acquisition");
    mgr.insert("stop", cmd);

    QVERIFY(mgr.hasCommand("stop"));
    QCOMPARE(mgr["stop"].description(), QString("Stop acquisition"));
}

void TestRtServerCommands::testCommandManagerInsertJsonDoc()
{
    QJsonObject idParam;
    idParam.insert("type", QString("int"));
    idParam.insert("description", QString("Client ID"));

    QJsonObject params;
    params.insert("id", idParam);

    QJsonObject cmdContent;
    cmdContent.insert("description", QString("Get measurement info"));
    cmdContent.insert("parameters", params);

    QJsonObject cmds;
    cmds.insert("measinfo", cmdContent);

    QJsonObject root;
    root.insert("commands", cmds);
    QJsonDocument doc(root);

    CommandManager mgr(doc);
    QVERIFY(mgr.hasCommand("measinfo"));
    QCOMPARE(mgr["measinfo"].description(), QString("Get measurement info"));
    QCOMPARE(mgr["measinfo"].count(), quint32(1));
}

void TestRtServerCommands::testCommandManagerHasCommand()
{
    CommandManager mgr;
    QVERIFY(!mgr.hasCommand("help"));
    mgr.insert("help", "Show help");
    QVERIFY(mgr.hasCommand("help"));
    QVERIFY(!mgr.hasCommand("stop"));
}

void TestRtServerCommands::testCommandManagerClear()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");
    mgr.insert("stop", "Stop");
    QCOMPARE(mgr.commandMap().size(), 2);

    mgr.clear();
    QVERIFY(mgr.commandMap().isEmpty());
}

void TestRtServerCommands::testCommandManagerSubscript()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");

    Command& cmd = mgr["help"];
    QCOMPARE(cmd.command(), QString("help"));

    const CommandManager& constMgr = mgr;
    Command constCmd = constMgr["help"];
    QCOMPARE(constCmd.command(), QString("help"));
}

void TestRtServerCommands::testCommandManagerActiveToggle()
{
    CommandManager mgr;
    QVERIFY(mgr.isActive());

    mgr.setStatus(false);
    QVERIFY(!mgr.isActive());

    mgr.setStatus(true);
    QVERIFY(mgr.isActive());
}

void TestRtServerCommands::testCommandManagerCommandMapChanged()
{
    CommandManager mgr;
    QSignalSpy spy(&mgr, &CommandManager::commandMapChanged);

    mgr.insert("help", "Show help");
    QCOMPARE(spy.count(), 1);

    mgr.insert("stop", "Stop");
    QCOMPARE(spy.count(), 2);
}

void TestRtServerCommands::testCommandManagerUpdate()
{
    // Build a manager with a command that has one int parameter
    QStringList names;
    names << "id";
    QList<QVariant> values;
    values << QVariant(int(0));

    CommandManager mgr;
    Command cmd("measinfo", "Get info", names, values, true);
    mgr.insert("measinfo", cmd);

    // Create a parser and attach the manager
    CommandParser parser;
    parser.attach(&mgr);

    // Spy on the executed signal
    QSignalSpy spy(&mgr["measinfo"], &Command::executed);

    // Parse a CLI command with parameter
    QStringList parsed;
    parser.parse("measinfo 42", parsed);

    // Check the command was executed and parameter was set
    QCOMPARE(spy.count(), 1);
    QCOMPARE(mgr["measinfo"][qint32(0)].toInt(), 42);
}

//=============================================================================================================
// Gap-fill tests
//=============================================================================================================

void TestRtServerCommands::testCommandReply()
{
    // reply() emits CommandManager::response when parent is a CommandManager
    CommandManager mgr;
    Command cmd("test", "A test");
    mgr.insert("test", cmd);

    QSignalSpy spy(&mgr, &CommandManager::response);

    mgr["test"].reply("some reply");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("some reply"));
}

void TestRtServerCommands::testCommandSend()
{
    // send() emits CommandManager::triggered when parent is a CommandManager
    CommandManager mgr;
    Command cmd("test", "A test");
    mgr.insert("test", cmd);

    QSignalSpy spy(&mgr, &CommandManager::triggered);

    mgr["test"].send();
    QCOMPARE(spy.count(), 1);
}

void TestRtServerCommands::testCommandParserGetRawCommand()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");

    CommandParser parser;
    parser.attach(&mgr);

    QStringList parsed;
    parser.parse("help", parsed);

    RawCommand& raw = parser.getRawCommand();
    QCOMPARE(raw.command(), QString("help"));
}

void TestRtServerCommands::testCommandManagerFromByteArray()
{
    QJsonObject cmdContent;
    cmdContent.insert("description", QString("Stop acquisition"));
    cmdContent.insert("parameters", QJsonObject());

    QJsonObject cmds;
    cmds.insert("stop", cmdContent);

    QJsonObject root;
    root.insert("commands", cmds);

    QByteArray jsonBytes = QJsonDocument(root).toJson(QJsonDocument::Compact);
    CommandManager mgr(jsonBytes);

    QVERIFY(mgr.hasCommand("stop"));
    QCOMPARE(mgr["stop"].description(), QString("Stop acquisition"));
}

void TestRtServerCommands::testCommandManagerTriggeredSignal()
{
    CommandManager mgr;
    Command cmd("go", "Go");
    mgr.insert("go", cmd);

    QSignalSpy spy(&mgr, &CommandManager::triggered);

    // send() on a child command emits triggered on the manager
    mgr["go"].send();
    QCOMPARE(spy.count(), 1);

    // Verify the command in the signal
    Command signalCmd = spy.at(0).at(0).value<Command>();
    QCOMPARE(signalCmd.command(), QString("go"));
}

void TestRtServerCommands::testCommandManagerResponseSignal()
{
    CommandManager mgr;
    Command cmd("info", "Get info");
    mgr.insert("info", cmd);

    QSignalSpy spy(&mgr, &CommandManager::response);

    mgr["info"].reply("result data");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("result data"));

    Command signalCmd = spy.at(0).at(1).value<Command>();
    QCOMPARE(signalCmd.command(), QString("info"));
}

void TestRtServerCommands::testCommandParserResponseSignal()
{
    CommandParser parser;
    QSignalSpy spy(&parser, &CommandParser::response);

    // The response signal can be emitted directly (it's just a signal declaration)
    // In practice it's emitted by the manager through the observer pattern,
    // but we can verify the signal is connectable and emittable
    QVERIFY(spy.isValid());
}

//=============================================================================================================
// Integration: Parser + Manager
//=============================================================================================================

void TestRtServerCommands::testParserManagerIntegration_CLI()
{
    // Setup a manager with two commands
    CommandManager mgr;
    mgr.insert("help", "Show help");

    QStringList names;
    names << "value";
    QList<QVariant> values;
    values << QVariant(int(0));
    Command setCmd("set", "Set value", names, values, false);
    mgr.insert("set", setCmd);

    CommandParser parser;
    parser.attach(&mgr);

    // Parse a CLI help command
    QStringList parsed;
    QVERIFY(parser.parse("help", parsed));
    QCOMPARE(parsed.at(0), QString("help"));

    // Parse a CLI set command with parameter
    parsed.clear();
    QVERIFY(parser.parse("set 100", parsed));
    QCOMPARE(parsed.at(0), QString("set"));
    QCOMPARE(mgr["set"][qint32(0)].toInt(), 100);
}

void TestRtServerCommands::testParserManagerIntegration_JSON()
{
    // Setup manager
    QStringList names;
    names << "id";
    QList<QVariant> values;
    values << QVariant(int(0));

    CommandManager mgr;
    Command cmd("measinfo", "Get info", names, values, true);
    mgr.insert("measinfo", cmd);

    CommandParser parser;
    parser.attach(&mgr);

    QSignalSpy spy(&mgr["measinfo"], &Command::executed);

    // Parse JSON command
    QString jsonInput = "{\"commands\":{\"measinfo\":{\"id\":\"7\"}}}";
    QStringList parsed;
    QVERIFY(parser.parse(jsonInput, parsed));
    QCOMPARE(parsed.at(0), QString("measinfo"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(mgr["measinfo"]["id"].toInt(), 7);
}

//=============================================================================================================

//=============================================================================================================
// Command edge cases
//=============================================================================================================

void TestRtServerCommands::testCommandDirectMemberAccess()
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

void TestRtServerCommands::testCommandReserializeRoundTrip()
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

void TestRtServerCommands::testCommandEmptyParamsToJson()
{
    Command cmd("help", "Show help", true);
    QJsonObject json = cmd.toJsonObject();

    QCOMPARE(json.value("description").toString(), QString("Show help"));
    QJsonObject params = json.value("parameters").toObject();
    QVERIFY(params.isEmpty());
}

//=============================================================================================================

void TestRtServerCommands::testCommandMultipleParamTypes()
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

void TestRtServerCommands::testCommandExecuteSignalPayload()
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

void TestRtServerCommands::testRawCommandMultipleParams()
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

void TestRtServerCommands::testRawCommandExecuteSignalPayload()
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

void TestRtServerCommands::testRawCommandCopyIndependence()
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

void TestRtServerCommands::testCommandManagerOverwriteCommand()
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

void TestRtServerCommands::testCommandManagerInsertMultipleFromJson()
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

void TestRtServerCommands::testCommandManagerInactiveSkipsParsing()
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

void TestRtServerCommands::testCommandManagerCommandMapAccess()
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

void TestRtServerCommands::testCommandParserMultipleSequentialParses()
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

void TestRtServerCommands::testCommandParserCLIWithMultipleParams()
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

void TestRtServerCommands::testCommandParserJsonWithMultipleCommands()
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

//=============================================================================================================

QTEST_GUILESS_MAIN(TestRtServerCommands)
#include "test_rtserver_commands.moc"
