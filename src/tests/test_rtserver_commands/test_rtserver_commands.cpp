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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <rtserver_connector/command.h>
#include <rtserver_connector/raw_command.h>
#include <rtserver_connector/command_parser.h>
#include <rtserver_connector/command_manager.h>

#include <mne/base/json.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest/QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace RTSERVER;

namespace
{

/** @return Whether @p text contains @p part. */
bool contains(const std::string& text, const std::string& part)
{
    return text.find(part) != std::string::npos;
}

/** @return A "commands" document with one command and one parameter of @p type. */
mne::JsonDocument oneCommandDocument(const std::string& name, const std::string& description, const std::string& param, const std::string& type)
{
    mne::JsonObject params;
    if (!param.empty()) {
        params.insert(param, mne::JsonObject{{"type", type}, {"description", "Client ID"}});
    }
    mne::JsonObject commands;
    commands.insert(name, mne::JsonObject{{"description", description}, {"parameters", params}});
    return mne::JsonDocument(mne::JsonObject{{"commands", commands}});
}

} // namespace

//=============================================================================================================

class TestRtServerCommands : public QObject
{
    Q_OBJECT

private slots:
    // ---- CommandValue tests ----
    void testCommandValueTypes();
    void testCommandValueConvertFrom();

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
    void testCommandExecuteCallback();
    void testCommandIsJson();

    // ---- RawCommand tests ----
    void testRawCommandDefault();
    void testRawCommandFromString_CLI();
    void testRawCommandFromString_JSON();
    void testRawCommandCopy();
    void testRawCommandAssignment();
    void testRawCommandExecuteCallback();

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
    void testCommandManagerUpdateRejectsBadParameter();

    // ---- Reply channels ----
    void testCommandReply();
    void testCommandSend();
    void testCommandParserGetRawCommand();
    void testCommandManagerFromJsonText();
    void testCommandParserResponse();

    // ---- Integration: Parser + Manager ----
    void testParserManagerIntegration_CLI();
    void testParserManagerIntegration_JSON();

    // ---- Edge cases ----
    void testCommandReserializeRoundTrip();
    void testCommandEmptyParamsToJson();
    void testCommandMultipleParamTypes();
    void testRawCommandCopyIndependence();
    void testCommandManagerOverwriteCommand();
    void testCommandManagerInsertMultipleFromJson();
    void testCommandManagerInactiveSkipsParsing();
    void testCommandParserMultipleSequentialParses();
    void testCommandParserCLIWithMultipleParams();
    void testCommandParserJsonWithMultipleCommands();
};

//=============================================================================================================
// CommandValue tests
//=============================================================================================================

void TestRtServerCommands::testCommandValueTypes()
{
    // The type names are those of the command port's help
    for (const char* name : {"int", "uint", "float", "double", "bool", "QString"}) {
        QCOMPARE(CommandValue::fromTypeName(name).typeName(), std::string(name));
    }
    QVERIFY(!CommandValue::fromTypeName("QStringList").isValid());
    QCOMPARE(CommandValue(42).toString(), std::string("42"));
    QCOMPARE(CommandValue(3.14).toString(), std::string("3.14"));
    QCOMPARE(CommandValue(true).toString(), std::string("true"));
    QCOMPARE(CommandValue("7").toInt(), 7);
    bool ok = true;
    QCOMPARE(CommandValue("x").toInt(&ok), 0);
    QVERIFY(!ok);
}

void TestRtServerCommands::testCommandValueConvertFrom()
{
    CommandValue value = CommandValue::fromTypeName("int");
    QVERIFY(value.convertFrom("12"));
    QCOMPARE(value.toInt(), 12);
    QVERIFY(!value.convertFrom("abc"));
    QCOMPARE(value.toInt(), 12); // unchanged

    CommandValue unsignedValue = CommandValue::fromTypeName("uint");
    QVERIFY(!unsignedValue.convertFrom("-1"));
    QVERIFY(unsignedValue.convertFrom("200"));
    QCOMPARE(unsignedValue.toUInt(), 200U);

    CommandValue floatValue = CommandValue::fromTypeName("float");
    QVERIFY(floatValue.convertFrom("2.5"));
    QCOMPARE(floatValue.toFloat(), 2.5F);
    QVERIFY(!floatValue.convertFrom("1e300"));

    CommandValue boolValue = CommandValue::fromTypeName("bool");
    QVERIFY(boolValue.convertFrom("yes"));
    QVERIFY(boolValue.toBool());
    QVERIFY(boolValue.convertFrom("False"));
    QVERIFY(!boolValue.toBool());

    CommandValue text = CommandValue::fromTypeName("QString");
    QVERIFY(text.convertFrom("a b"));
    QCOMPARE(text.toString(), std::string("a b"));

    CommandValue invalid;
    QVERIFY(!invalid.convertFrom("1"));
}

//=============================================================================================================
// Command tests
//=============================================================================================================

void TestRtServerCommands::testCommandDefaultConstruction()
{
    Command cmd;
    QCOMPARE(cmd.command(), std::string(""));
    QCOMPARE(cmd.description(), std::string(""));
    QCOMPARE(cmd.count(), std::uint32_t(0));
    QVERIFY(cmd.isJson());
}

void TestRtServerCommands::testCommandNameDescription()
{
    Command cmd("help", "Show help text", false);
    QCOMPARE(cmd.command(), std::string("help"));
    QCOMPARE(cmd.description(), std::string("Show help text"));
    QCOMPARE(cmd.count(), std::uint32_t(0));
    QVERIFY(!cmd.isJson());
}

void TestRtServerCommands::testCommandWithParams()
{
    Command cmd("set", "Set a value", {"id", "value"}, {CommandValue(42), CommandValue("test")}, true);
    QCOMPARE(cmd.count(), std::uint32_t(2));
    QCOMPARE(cmd.pNames(), (mne::StringList{"id", "value"}));
    QCOMPARE(cmd.pValues().at(0).toInt(), 42);
    QCOMPARE(cmd.pValues().at(1).toString(), std::string("test"));
    // Auto-generated descriptions are empty
    QCOMPARE(cmd.pDescriptions(), (mne::StringList{"", ""}));
}

void TestRtServerCommands::testCommandWithParamsAndDescriptions()
{
    Command cmd("get", "Get info", {"id"}, {CommandValue(0)}, {"Client ID"}, true);
    QCOMPARE(cmd.count(), std::uint32_t(1));
    QCOMPARE(cmd.pDescriptions().at(0), std::string("Client ID"));
}

void TestRtServerCommands::testCommandFromJson()
{
    const mne::JsonDocument doc = oneCommandDocument("measinfo", "Get measurement info", "id", "int");
    Command cmd("measinfo", doc.object().value("commands").to_object().value("measinfo").to_object(), true);
    QCOMPARE(cmd.command(), std::string("measinfo"));
    QCOMPARE(cmd.description(), std::string("Get measurement info"));
    QCOMPARE(cmd.count(), std::uint32_t(1));
    QCOMPARE(cmd.pNames().at(0), std::string("id"));
    QCOMPARE(cmd.pDescriptions().at(0), std::string("Client ID"));
    QCOMPARE(cmd.pValues().at(0).typeName(), std::string("int"));
}

void TestRtServerCommands::testCommandCopy()
{
    Command original("test", "A test", {"x"}, {CommandValue(3.14)}, true);
    int executed = 0;
    original.connectExecuted([&executed](const Command&) { ++executed; });
    Command copy(original);
    QCOMPARE(copy.command(), original.command());
    QCOMPARE(copy.description(), original.description());
    QCOMPARE(copy.count(), original.count());
    QCOMPARE(copy.pValues().at(0).toDouble(), 3.14);
    copy.execute(); // callbacks belong to the original, as connections belong to a QObject
    QCOMPARE(executed, 0);
}

void TestRtServerCommands::testCommandAssignment()
{
    Command a("alpha", "First");
    Command b("beta", "Second");
    b = a;
    QCOMPARE(b.command(), std::string("alpha"));
    QCOMPARE(b.description(), std::string("First"));
}

void TestRtServerCommands::testCommandToJsonObject()
{
    Command cmd("measinfo", "Get info", {"id"}, {CommandValue("0")}, {"Client ID"}, true);
    const mne::JsonObject json = cmd.toJsonObject();
    QCOMPARE(json.value("description").to_std_string(), std::string("Get info"));
    const mne::JsonObject& params = json.value("parameters").to_object();
    QVERIFY(params.contains("id"));
    QCOMPARE(params.value("id").to_object().value("description").to_std_string(), std::string("Client ID"));
    QCOMPARE(params.value("id").to_object().value("type").to_std_string(), std::string("QString"));
}

void TestRtServerCommands::testCommandToStringList()
{
    Command cmd("measinfo", "Get info", {"id"}, {CommandValue(0)}, {"Client ID"}, true);
    QCOMPARE(cmd.toStringList(), (mne::StringList{"measinfo", "[Client ID]", "Get info"}));
}

void TestRtServerCommands::testCommandToStringReadySend()
{
    Command cmd("measinfo", "Get info", {"id", "alias"}, {CommandValue(42), CommandValue("x")}, true);
    QCOMPARE(cmd.toStringReadySend(), std::string(R"("measinfo":{"id":"42","alias":"x"})"));
}

void TestRtServerCommands::testCommandSubscriptByName()
{
    Command cmd("set", "Set", {"id", "value"}, {CommandValue(1), CommandValue("abc")}, true);
    QCOMPARE(cmd["id"].toInt(), 1);
    QCOMPARE(cmd["value"].toString(), std::string("abc"));
    cmd["id"].setValue(99);
    QCOMPARE(cmd["id"].toInt(), 99);
    QVERIFY(!cmd["missing"].isValid());
}

void TestRtServerCommands::testCommandSubscriptByIndex()
{
    Command cmd("get", "Get", {"x"}, {CommandValue(5)}, true);
    QCOMPARE(cmd[std::int32_t(0)].toInt(), 5);
    cmd[std::int32_t(0)].setValue(10);
    QCOMPARE(cmd[std::int32_t(0)].toInt(), 10);
    QVERIFY(!cmd[std::int32_t(3)].isValid());
}

void TestRtServerCommands::testCommandConstSubscript()
{
    Command cmd("get", "Get", {"id"}, {CommandValue(7)}, true);
    const Command& cref = cmd;
    QCOMPARE(cref["id"].toInt(), 7);
}

void TestRtServerCommands::testCommandExecuteCallback()
{
    Command cmd("test", "Test");
    std::vector<std::string> received;
    cmd.connectExecuted([&received](const Command& command) { received.push_back(command.command()); });
    cmd.connectExecuted([&received](const Command&) { received.emplace_back("second"); });
    cmd.execute();
    QCOMPARE(received, (std::vector<std::string>{"test", "second"}));
}

void TestRtServerCommands::testCommandIsJson()
{
    Command cmd1(true);
    QVERIFY(cmd1.isJson());
    Command cmd2(false);
    QVERIFY(!cmd2.isJson());
    cmd2.isJson() = true;
    QVERIFY(cmd2.isJson());
}

//=============================================================================================================
// RawCommand tests
//=============================================================================================================

void TestRtServerCommands::testRawCommandDefault()
{
    RawCommand raw;
    QCOMPARE(raw.command(), std::string(""));
    QCOMPARE(raw.count(), std::uint32_t(0));
    QVERIFY(!raw.isJson());
}

void TestRtServerCommands::testRawCommandFromString_CLI()
{
    RawCommand raw("help", false);
    QCOMPARE(raw.command(), std::string("help"));
    QVERIFY(!raw.isJson());
    raw.pValues().emplace_back("param1");
    raw.pValues().emplace_back("param2");
    QCOMPARE(raw.count(), std::uint32_t(2));
    QCOMPARE(raw.pValues().at(0), std::string("param1"));
}

void TestRtServerCommands::testRawCommandFromString_JSON()
{
    RawCommand raw("measinfo", true);
    QCOMPARE(raw.command(), std::string("measinfo"));
    QVERIFY(raw.isJson());
}

void TestRtServerCommands::testRawCommandCopy()
{
    RawCommand original("test", true);
    original.pValues().emplace_back("val1");
    RawCommand copy(original);
    QCOMPARE(copy.command(), std::string("test"));
    QVERIFY(copy.isJson());
    QCOMPARE(copy.count(), std::uint32_t(1));
    QCOMPARE(copy.pValues().at(0), std::string("val1"));
}

void TestRtServerCommands::testRawCommandAssignment()
{
    RawCommand a("alpha", true);
    a.pValues().emplace_back("x");
    RawCommand b;
    b = a;
    QCOMPARE(b.command(), std::string("alpha"));
    QCOMPARE(b.count(), std::uint32_t(1));
    QCOMPARE(b.pValues().at(0), std::string("x"));
}

void TestRtServerCommands::testRawCommandExecuteCallback()
{
    RawCommand raw("test", false);
    raw.pValues().emplace_back("param1");
    raw.pValues().emplace_back("param2");
    std::vector<std::string> received;
    raw.connectExecuted([&received](const std::vector<std::string>& params) { received = params; });
    raw.execute();
    QCOMPARE(received, (std::vector<std::string>{"param1", "param2"}));
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
    mne::StringList parsed;
    QVERIFY(parser.parse("help", parsed));
    QCOMPARE(parsed, mne::StringList{"help"});
}

void TestRtServerCommands::testCommandParserJSON()
{
    CommandManager manager;
    manager.insert("stop", "Stop");
    CommandParser parser;
    parser.attach(&manager);
    mne::StringList parsed;
    QVERIFY(parser.parse(R"({"commands":{"stop":{}}})", parsed));
    QCOMPARE(parsed, mne::StringList{"stop"});
}

void TestRtServerCommands::testCommandParserEmpty()
{
    CommandParser parser;
    mne::StringList parsed;
    QVERIFY(!parser.parse("", parsed));
    QVERIFY(!parser.parse("{not json", parsed));
}

void TestRtServerCommands::testCommandParserUnknown()
{
    CommandManager manager;
    manager.insert("help", "Show help");
    CommandParser parser;
    parser.attach(&manager);
    mne::StringList parsed;
    QVERIFY(!parser.parse("unknown_command", parsed));
    QVERIFY(parsed.empty());
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
    QVERIFY(mgr.commandMap().empty());
}

void TestRtServerCommands::testCommandManagerInsertKeyDesc()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");
    QVERIFY(mgr.hasCommand("help"));
    QCOMPARE(mgr["help"].command(), std::string("help"));
    QCOMPARE(mgr["help"].description(), std::string("Show help"));
    QVERIFY(!mgr["help"].isJson());
}

void TestRtServerCommands::testCommandManagerInsertCommand()
{
    CommandManager mgr;
    mgr.insert("stop", Command("stop", "Stop acquisition"));
    QVERIFY(mgr.hasCommand("stop"));
    QCOMPARE(mgr["stop"].description(), std::string("Stop acquisition"));
}

void TestRtServerCommands::testCommandManagerInsertJsonDoc()
{
    CommandManager mgr(oneCommandDocument("measinfo", "Get measurement info", "id", "int"));
    QVERIFY(mgr.hasCommand("measinfo"));
    QCOMPARE(mgr["measinfo"].description(), std::string("Get measurement info"));
    QCOMPARE(mgr["measinfo"].count(), std::uint32_t(1));
    QVERIFY(mgr["measinfo"].isJson());

    // Commands already known are kept
    mgr.insert(oneCommandDocument("measinfo", "Other", "", ""));
    QCOMPARE(mgr["measinfo"].description(), std::string("Get measurement info"));
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
    QCOMPARE(mgr.commandMap().size(), std::size_t(2));
    mgr.clear();
    QVERIFY(mgr.commandMap().empty());
}

void TestRtServerCommands::testCommandManagerSubscript()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");
    QCOMPARE(mgr["help"].command(), std::string("help"));
    const CommandManager& constMgr = mgr;
    QCOMPARE(constMgr["help"].command(), std::string("help"));
    QCOMPARE(constMgr["none"].command(), std::string(""));
    QVERIFY(!mgr.hasCommand("none"));
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
    int changes = 0;
    mgr.connectCommandMapChanged([&changes] { ++changes; });
    mgr.insert("help", "Show help");
    QCOMPARE(changes, 1);
    mgr.insert("stop", "Stop");
    QCOMPARE(changes, 2);
}

void TestRtServerCommands::testCommandManagerUpdate()
{
    CommandManager mgr;
    mgr.insert("measinfo", Command("measinfo", "Get info", {"id"}, {CommandValue(0)}, true));
    CommandParser parser;
    parser.attach(&mgr);
    int executed = 0;
    mgr["measinfo"].connectExecuted([&executed](const Command&) { ++executed; });

    mne::StringList parsed;
    parser.parse("measinfo 42", parsed);

    QCOMPARE(executed, 1);
    QCOMPARE(mgr["measinfo"][std::int32_t(0)].toInt(), 42);
    QVERIFY(!mgr["measinfo"].isJson()); // the dialect of the request
}

void TestRtServerCommands::testCommandManagerUpdateRejectsBadParameter()
{
    CommandManager mgr;
    mgr.insert("selcon", Command("selcon", "Select", {"ConID"}, {CommandValue(0)}, true));
    CommandParser parser;
    parser.attach(&mgr);
    int executed = 0;
    mgr["selcon"].connectExecuted([&executed](const Command&) { ++executed; });

    mne::StringList parsed;
    QVERIFY(parser.parse("selcon abc", parsed)); // known command ...
    QVERIFY(parser.parse("selcon", parsed));     // ... and too few parameters
    QCOMPARE(executed, 0);
}

//=============================================================================================================
// Reply channels
//=============================================================================================================

void TestRtServerCommands::testCommandReply()
{
    CommandManager mgr;
    mgr.insert("info", Command("info", "Get info"));
    std::vector<std::pair<std::string, std::string>> replies;
    mgr.connectResponse([&replies](const std::string& reply, const Command& command) { replies.emplace_back(reply, command.command()); });
    mgr["info"].reply("result data");
    QCOMPARE(replies.size(), std::size_t(1));
    QCOMPARE(replies[0].first, std::string("result data"));
    QCOMPARE(replies[0].second, std::string("info"));

    Command loose("loose", "Not in a manager");
    loose.reply("nobody listens"); // no manager, no effect
    QCOMPARE(replies.size(), std::size_t(1));
}

void TestRtServerCommands::testCommandSend()
{
    CommandManager mgr;
    mgr.insert("go", Command("go", "Go"));
    std::vector<std::string> sent;
    mgr.connectTriggered([&sent](const Command& command) { sent.push_back(command.command()); });
    mgr["go"].send();
    QCOMPARE(sent, std::vector<std::string>{"go"});
}

void TestRtServerCommands::testCommandParserGetRawCommand()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");
    CommandParser parser;
    parser.attach(&mgr);
    mne::StringList parsed;
    parser.parse("help", parsed);
    QCOMPARE(parser.getRawCommand().command(), std::string("help"));
}

void TestRtServerCommands::testCommandManagerFromJsonText()
{
    const std::string json = oneCommandDocument("stop", "Stop acquisition", "", "").to_json(mne::JsonDocument::Compact);
    CommandManager mgr(json);
    QVERIFY(mgr.hasCommand("stop"));
    QCOMPARE(mgr["stop"].description(), std::string("Stop acquisition"));
}

void TestRtServerCommands::testCommandParserResponse()
{
    CommandParser parser;
    std::vector<std::string> replies;
    parser.connectResponse([&replies](const std::string& reply, const Command&) { replies.push_back(reply); });
    parser.response("ok", Command("x", "y"));
    QCOMPARE(replies, std::vector<std::string>{"ok"});
}

//=============================================================================================================
// Integration: Parser + Manager
//=============================================================================================================

void TestRtServerCommands::testParserManagerIntegration_CLI()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");
    mgr.insert("set", Command("set", "Set value", {"value"}, {CommandValue(0)}, false));
    CommandParser parser;
    parser.attach(&mgr);

    mne::StringList parsed;
    QVERIFY(parser.parse("help", parsed));
    QCOMPARE(parsed.at(0), std::string("help"));

    QVERIFY(parser.parse("set 100", parsed));
    QCOMPARE(parsed, mne::StringList{"set"});
    QCOMPARE(mgr["set"][std::int32_t(0)].toInt(), 100);
}

void TestRtServerCommands::testParserManagerIntegration_JSON()
{
    CommandManager mgr;
    mgr.insert("measinfo", Command("measinfo", "Get info", {"id"}, {CommandValue(0)}, false));
    CommandParser parser;
    parser.attach(&mgr);
    int executed = 0;
    mgr["measinfo"].connectExecuted([&executed](const Command&) { ++executed; });

    mne::StringList parsed;
    QVERIFY(parser.parse(R"({"commands":{"measinfo":{"id":"7"}}})", parsed));
    QCOMPARE(parsed, mne::StringList{"measinfo"});
    QCOMPARE(executed, 1);
    QCOMPARE(mgr["measinfo"]["id"].toInt(), 7);
    QVERIFY(mgr["measinfo"].isJson());
}

//=============================================================================================================
// Edge cases
//=============================================================================================================

void TestRtServerCommands::testCommandReserializeRoundTrip()
{
    Command original("set", "Set a value", {"id", "value"}, {CommandValue(42), CommandValue("hello")}, {"Client ID", "Setting value"}, true);
    Command reconstructed("set", original.toJsonObject(), true);
    QCOMPARE(reconstructed.command(), original.command());
    QCOMPARE(reconstructed.description(), original.description());
    QCOMPARE(reconstructed.count(), original.count());
    QCOMPARE(reconstructed.pNames(), original.pNames());
    QCOMPARE(reconstructed.pDescriptions().at(0), std::string("Client ID"));
    QCOMPARE(reconstructed.pValues().at(0).typeName(), std::string("int"));
    QCOMPARE(reconstructed.pValues().at(1).typeName(), std::string("QString"));
}

void TestRtServerCommands::testCommandEmptyParamsToJson()
{
    Command cmd("help", "Show help", true);
    const mne::JsonObject json = cmd.toJsonObject();
    QCOMPARE(json.value("description").to_std_string(), std::string("Show help"));
    QVERIFY(json.value("parameters").is_object());
    QVERIFY(json.value("parameters").to_object().empty());
}

void TestRtServerCommands::testCommandMultipleParamTypes()
{
    Command cmd("multi", "Multi-type params", {"intParam", "floatParam", "stringParam", "boolParam"},
                {CommandValue(100), CommandValue(3.14), CommandValue("text"), CommandValue(true)}, true);
    QCOMPARE(cmd.count(), std::uint32_t(4));
    QCOMPARE(cmd["intParam"].toInt(), 100);
    QVERIFY(std::abs(cmd["floatParam"].toDouble() - 3.14) < 0.001);
    QCOMPARE(cmd["stringParam"].toString(), std::string("text"));
    QCOMPARE(cmd["boolParam"].toBool(), true);
    const std::string ready = cmd.toStringReadySend();
    QVERIFY(contains(ready, "\"intParam\":\"100\""));
    QVERIFY(contains(ready, "\"boolParam\":\"true\""));
}

void TestRtServerCommands::testRawCommandCopyIndependence()
{
    RawCommand original("test", true);
    original.pValues().emplace_back("val");
    RawCommand copy(original);
    copy.pValues().emplace_back("extra");
    QCOMPARE(original.count(), std::uint32_t(1));
    QCOMPARE(copy.count(), std::uint32_t(2));
}

void TestRtServerCommands::testCommandManagerOverwriteCommand()
{
    CommandManager mgr;
    mgr.insert("test", "First description");
    int executed = 0;
    mgr["test"].connectExecuted([&executed](const Command&) { ++executed; });
    mgr.insert("test", "Second description");
    QCOMPARE(mgr["test"].description(), std::string("Second description"));
    QCOMPARE(mgr.commandMap().size(), std::size_t(1));
    mgr["test"].execute(); // the callbacks stay with the stored command
    QCOMPARE(executed, 1);
}

void TestRtServerCommands::testCommandManagerInsertMultipleFromJson()
{
    mne::JsonObject commands;
    commands.insert("start", mne::JsonObject{{"description", "Start acquisition"}, {"parameters", mne::JsonObject()}});
    commands.insert("stop", mne::JsonObject{{"description", "Stop acquisition"}, {"parameters", mne::JsonObject()}});
    commands.insert("help", mne::JsonObject{{"description", "Show help"}, {"parameters", mne::JsonObject()}});
    CommandManager mgr(mne::JsonDocument(mne::JsonObject{{"commands", commands}}));
    QCOMPARE(mgr.commandMap().size(), std::size_t(3));
    QVERIFY(mgr.hasCommand("start"));
    QVERIFY(mgr.hasCommand("stop"));
    QVERIFY(mgr.hasCommand("help"));
    QCOMPARE(mgr["start"].description(), std::string("Start acquisition"));
}

void TestRtServerCommands::testCommandManagerInactiveSkipsParsing()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");
    CommandParser parser;
    parser.attach(&mgr);
    mgr.setStatus(false);
    int executed = 0;
    mgr["help"].connectExecuted([&executed](const Command&) { ++executed; });
    mne::StringList parsed;
    QVERIFY(parser.parse("help", parsed));
    QCOMPARE(executed, 0);
}

void TestRtServerCommands::testCommandParserMultipleSequentialParses()
{
    CommandManager mgr;
    mgr.insert("help", "Show help");
    mgr.insert("stop", "Stop");
    CommandParser parser;
    parser.attach(&mgr);
    mne::StringList parsed;
    QVERIFY(parser.parse("help", parsed));
    QCOMPARE(parsed, mne::StringList{"help"});
    QVERIFY(parser.parse("stop", parsed));
    QCOMPARE(parsed, mne::StringList{"stop"});
    QVERIFY(parser.parse("help", parsed));
    QCOMPARE(parsed, mne::StringList{"help"});
}

void TestRtServerCommands::testCommandParserCLIWithMultipleParams()
{
    CommandManager mgr;
    mgr.insert("move", Command("move", "Move to position", {"x", "y", "z"}, {CommandValue(0), CommandValue(0), CommandValue(0)}, false));
    CommandParser parser;
    parser.attach(&mgr);
    mne::StringList parsed;
    QVERIFY(parser.parse("move 10 20 30", parsed));
    QCOMPARE(mgr["move"][std::int32_t(0)].toInt(), 10);
    QCOMPARE(mgr["move"][std::int32_t(1)].toInt(), 20);
    QCOMPARE(mgr["move"][std::int32_t(2)].toInt(), 30);
}

void TestRtServerCommands::testCommandParserJsonWithMultipleCommands()
{
    CommandManager mgr;
    mgr.insert("start", "Start");
    mgr.insert("stop", "Stop");
    CommandParser parser;
    parser.attach(&mgr);
    mne::StringList parsed;
    QVERIFY(parser.parse(R"({"commands":{"stop":{},"start":{}}})", parsed));
    QCOMPARE(parsed, (mne::StringList{"start", "stop"})); // in key order, as QJsonObject iterates
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestRtServerCommands)
#include "test_rtserver_commands.moc"
