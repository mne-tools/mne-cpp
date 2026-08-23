//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_studio_skill_host.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     August, 2026
 * @brief    Integration tests for the local Studio skill-host service.
 */

#include <skillhostservice.h>

#include <jsonrpcmessage.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QUuid>
#include <QtTest>

using namespace MNEANALYZESTUDIO;

class TestStudioSkillHost : public QObject {
    Q_OBJECT

    private slots:
    void initTestCase();
    void malformedRequest();
    void listCapabilities();
    void toolCalls();
    void viewSessions();
    void reloadExtensions();

    private:
    void sendRequest(const QString& method, const QJsonObject& params, QJsonObject& result);

    SkillHostService m_service;
    QLocalSocket m_socket;
    QString m_dummySessionId;
};

void TestStudioSkillHost::initTestCase() {
    const QString socketName = QStringLiteral("mne-cpp-skill-host-test-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QVERIFY(m_service.start(socketName, QStringLiteral(MNE_STUDIO_EXTENSIONS_DIR)));
    m_socket.connectToServer(socketName);
    QVERIFY(m_socket.waitForConnected(5000));
}

void TestStudioSkillHost::sendRequest(const QString& method,
                                      const QJsonObject& params,
                                      QJsonObject& result) {
    const QJsonObject request = JsonRpcMessage::createRequest(QStringLiteral("request"), method, params);
    m_socket.write(JsonRpcMessage::serialize(request));
    QVERIFY(m_socket.flush());
    if (!m_socket.canReadLine()) {
        QSignalSpy readyReadSpy(&m_socket, &QLocalSocket::readyRead);
        QVERIFY(readyReadSpy.wait(5000));
    }

    QJsonObject response;
    QString errorString;
    QVERIFY2(JsonRpcMessage::deserialize(m_socket.readLine(), response, errorString),
             qPrintable(errorString));
    result = response.value(QStringLiteral("result")).toObject();
}

void TestStudioSkillHost::malformedRequest() {
    m_socket.write("{bad json\n");
    QVERIFY(m_socket.flush());
    if (!m_socket.canReadLine()) {
        QSignalSpy readyReadSpy(&m_socket, &QLocalSocket::readyRead);
        QVERIFY(readyReadSpy.wait(5000));
    }

    QJsonObject response;
    QString errorString;
    QVERIFY(JsonRpcMessage::deserialize(m_socket.readLine(), response, errorString));
    QCOMPARE(response.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toInt(),
             -32700);
}

void TestStudioSkillHost::listCapabilities() {
    QJsonObject resources;
    sendRequest(QStringLiteral("resources/list"), {}, resources);
    QVERIFY(resources.value(QStringLiteral("resources")).toArray().size() >= 7);

    QJsonObject tools;
    sendRequest(QStringLiteral("tools/list"), {}, tools);
    QVERIFY(tools.value(QStringLiteral("tools")).toArray().size() >= 2);

    QJsonObject missingResource;
    sendRequest(QStringLiteral("resources/read"),
                QJsonObject{{QStringLiteral("uri"), QStringLiteral("missing://resource")}},
                missingResource);
    QVERIFY(!missingResource.isEmpty());
}

void TestStudioSkillHost::toolCalls() {
    auto callTool = [this](const QString& name, const QJsonObject& arguments) {
        QJsonObject result;
        sendRequest(QStringLiteral("tools/call"),
                    QJsonObject{{QStringLiteral("name"), name},
                                {QStringLiteral("arguments"), arguments}},
                    result);
        return result;
    };

    QCOMPARE(callTool(QStringLiteral("studio.workflow.load"), {}).value(QStringLiteral("status")).toString(),
             QStringLiteral("error"));
    QCOMPARE(callTool(QStringLiteral("studio.workflow.save"), {}).value(QStringLiteral("status")).toString(),
             QStringLiteral("error"));
    QCOMPARE(callTool(QStringLiteral("dummy3d.set_opacity"),
                      QJsonObject{{QStringLiteral("opacity"), 2.0}})
             .value(QStringLiteral("opacity"))
             .toDouble(),
             1.0);
    QCOMPARE(callTool(QStringLiteral("fiffbrowser.reveal_active_state"), {})
             .value(QStringLiteral("session_count"))
             .toInt(),
             0);
    QCOMPARE(callTool(QStringLiteral("unknown.tool"), {}).value(QStringLiteral("status")).toString(),
             QStringLiteral("ignored"));
}

void TestStudioSkillHost::viewSessions() {
    QJsonObject invalid;
    sendRequest(QStringLiteral("views/open"), {}, invalid);
    QCOMPARE(invalid.value(QStringLiteral("status")).toString(), QStringLiteral("error"));

    QJsonObject unknown;
    sendRequest(QStringLiteral("views/open"),
                QJsonObject{{QStringLiteral("file"), QStringLiteral("surface.bem")},
                            {QStringLiteral("provider_id"), QStringLiteral("missing.provider")}},
                unknown);
    QCOMPARE(unknown.value(QStringLiteral("status")).toString(), QStringLiteral("error"));

    QJsonObject dummy;
    sendRequest(QStringLiteral("views/open"),
                QJsonObject{{QStringLiteral("file"), QStringLiteral("surface.bem")},
                            {QStringLiteral("provider_id"), QStringLiteral("inspect3d.surface_view")},
                            {QStringLiteral("sceneId"), QStringLiteral("scene-1")}},
                dummy);
    QCOMPARE(dummy.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
    m_dummySessionId = dummy.value(QStringLiteral("session_id")).toString();
    QVERIFY(!m_dummySessionId.isEmpty());
    QVERIFY(dummy.value(QStringLiteral("capabilities")).toObject().value(QStringLiteral("raw_browser_embedded")).toBool() == false);

    QJsonObject command;
    sendRequest(QStringLiteral("views/command"), {}, command);
    QCOMPARE(command.value(QStringLiteral("status")).toString(), QStringLiteral("error"));
    sendRequest(QStringLiteral("views/command"),
                QJsonObject{{QStringLiteral("session_id"), QStringLiteral("missing")},
                            {QStringLiteral("command"), QStringLiteral("reset")}},
                command);
    QCOMPARE(command.value(QStringLiteral("status")).toString(), QStringLiteral("error"));
    sendRequest(QStringLiteral("views/command"),
                QJsonObject{{QStringLiteral("session_id"), m_dummySessionId},
                            {QStringLiteral("command"), QStringLiteral("unsupported")}},
                command);
    QCOMPARE(command.value(QStringLiteral("status")).toString(), QStringLiteral("ignored"));

    QJsonObject opacity;
    sendRequest(QStringLiteral("tools/call"),
                QJsonObject{{QStringLiteral("name"), QStringLiteral("dummy3d.set_opacity")},
                            {QStringLiteral("arguments"), QJsonObject{{QStringLiteral("opacity"), 0.25}}}},
                opacity);
    QCOMPARE(opacity.value(QStringLiteral("updated_sessions")).toArray().size(), 1);

    QJsonObject browser;
    sendRequest(QStringLiteral("views/open"),
                QJsonObject{{QStringLiteral("file"), QStringLiteral("sample_raw.fif")},
                            {QStringLiteral("provider_id"), QStringLiteral("fiffbrowser.raw_view")}},
                browser);
    QCOMPARE(browser.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
    QVERIFY(browser.value(QStringLiteral("capabilities")).toObject().value(QStringLiteral("raw_browser_embedded")).toBool());

    QJsonObject revealed;
    sendRequest(QStringLiteral("tools/call"),
                QJsonObject{{QStringLiteral("name"), QStringLiteral("fiffbrowser.reveal_active_state")},
                            {QStringLiteral("arguments"), QJsonObject()}},
                revealed);
    QCOMPARE(revealed.value(QStringLiteral("session_count")).toInt(), 1);

    QJsonObject sessions;
    sendRequest(QStringLiteral("views/list"), {}, sessions);
    QCOMPARE(sessions.value(QStringLiteral("sessions")).toArray().size(), 2);
}

void TestStudioSkillHost::reloadExtensions() {
    QJsonObject result;
    sendRequest(QStringLiteral("extensions.reload"),
                QJsonObject{{QStringLiteral("disabled_extension_ids"),
                             QJsonArray{QStringLiteral("dummy-3d-extension"), QString(), 4}}},
                result);
    QCOMPARE(result.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
    QCOMPARE(result.value(QStringLiteral("disabled_extension_count")).toInt(), 1);
    QCOMPARE(result.value(QStringLiteral("invalidated_session_count")).toInt(), 1);

    sendRequest(QStringLiteral("extensions/reload"),
                QJsonObject{{QStringLiteral("extensions_directory"), QStringLiteral(MNE_STUDIO_EXTENSIONS_DIR)}},
                result);
    QCOMPARE(result.value(QStringLiteral("disabled_extension_count")).toInt(), 0);
}

QTEST_GUILESS_MAIN(TestStudioSkillHost)
#include "test_studio_skill_host.moc"