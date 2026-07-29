//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_studio_dummy3d_view.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Checks the hosted 3D view against session descriptors it did not write.
 *
 * The workbench hands this widget a session descriptor and the widget reads
 * scene identity and file paths straight out of it. The descriptor comes from
 * a saved session, so it can name a file that has since been moved or deleted,
 * carry the wrong types in its fields, or be missing them entirely.
 *
 * The accessors are pure reads of descriptor keys, so they are asserted as
 * round trips. displayTitle is the one with real logic: it prefers the title
 * in the descriptor and falls back when there is none, and that fallback is
 * what a user sees when a session was saved without one.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dummy3dhostedviewwidget.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QWidget>
#include <QJsonObject>
#include <QJsonArray>
#include <QScopedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZESTUDIO;

//=============================================================================================================
/**
 * DECLARE CLASS TestStudioDummy3DView
 *
 * @brief Checks the hosted 3D view's handling of session descriptors.
 */
class TestStudioDummy3DView: public QObject
{
    Q_OBJECT

public:
    TestStudioDummy3DView() = default;

private:
    QScopedPointer<QWidget> m_pHolder;

private slots:
    void initTestCase();

    void freshWidget_namesNothing();
    void descriptorFields_roundTrip();
    void descriptor_missingFileIsSurvivable();
    void descriptor_wrongFieldTypesAreSurvivable_data();
    void descriptor_wrongFieldTypesAreSurvivable();
    void displayTitle_prefersTheDescriptorTitle();
    void sessionUpdate_beforeAnyDescriptorIsSurvivable();

    void cleanupTestCase();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

void TestStudioDummy3DView::initTestCase()
{
    m_pHolder.reset(new QWidget());
}

//=============================================================================================================

void TestStudioDummy3DView::freshWidget_namesNothing()
{
    Dummy3DHostedViewWidget widget(m_pHolder.data());

    // Before a descriptor arrives there is no session to name. Returning
    // something non empty here would let the workbench match this view to a
    // session it is not showing.
    QVERIFY2(widget.sessionId().isEmpty(), "a widget with no descriptor named a session");
    QVERIFY2(widget.sceneId().isEmpty(), "a widget with no descriptor named a scene");
    QVERIFY2(widget.filePath().isEmpty(), "a widget with no descriptor named a file");
}

//=============================================================================================================

void TestStudioDummy3DView::descriptorFields_roundTrip()
{
    Dummy3DHostedViewWidget widget(m_pHolder.data());

    // These identifiers are how the workbench matches a view back to its
    // session, so they have to come back exactly as supplied.
    const QJsonObject descriptor{
        {"session_id", "session-42"},
        {"scene_id", "scene-7"},
        {"title", "Left hemisphere"}
    };

    widget.setSessionDescriptor(descriptor);

    QCOMPARE(widget.sessionId(), QString("session-42"));
    QCOMPARE(widget.sceneId(), QString("scene-7"));
    QCOMPARE(widget.displayTitle(), QString("Left hemisphere"));

    // A second descriptor replaces the first rather than merging into it.
    widget.setSessionDescriptor(QJsonObject{{"session_id", "session-43"},
                                            {"scene_id", "scene-8"}});
    QCOMPARE(widget.sessionId(), QString("session-43"));
    QCOMPARE(widget.sceneId(), QString("scene-8"));
}

//=============================================================================================================

void TestStudioDummy3DView::descriptor_missingFileIsSurvivable()
{
    Dummy3DHostedViewWidget widget(m_pHolder.data());
    widget.resize(400, 300);

    // A saved session naming a file that has since been moved or deleted is
    // the ordinary way this fails, and it has to leave a usable view rather
    // than taking the workbench down while restoring a session.
    widget.setSessionDescriptor(QJsonObject{
        {"session_id", "session-1"},
        {"file", "/no/such/directory/missing-scene.fif"}
    });

    // The descriptor is still readable afterwards, so the workbench can report
    // which session failed rather than losing track of it.
    QCOMPARE(widget.sessionId(), QString("session-1"));
    QCOMPARE(widget.filePath(), QString("/no/such/directory/missing-scene.fif"));
}

//=============================================================================================================

void TestStudioDummy3DView::descriptor_wrongFieldTypesAreSurvivable_data()
{
    QTest::addColumn<QJsonObject>("descriptor");

    // A descriptor is read back from a session file, so its fields can be any
    // JSON type rather than the strings the widget expects.
    QTest::newRow("empty")          << QJsonObject{};
    QTest::newRow("numeric ids")    << QJsonObject{{"session_id", 42}, {"scene_id", 7}};
    QTest::newRow("array file")     << QJsonObject{{"file", QJsonArray{"a", "b"}}};
    QTest::newRow("object title")   << QJsonObject{{"title", QJsonObject{{"nested", true}}}};
    QTest::newRow("null fields")    << QJsonObject{{"session_id", QJsonValue::Null},
                                                   {"file", QJsonValue::Null}};
    QTest::newRow("bool file")      << QJsonObject{{"file", true}};
    QTest::newRow("blank strings")  << QJsonObject{{"session_id", "  "}, {"file", "   "}};
}

//=============================================================================================================

void TestStudioDummy3DView::descriptor_wrongFieldTypesAreSurvivable()
{
    QFETCH(QJsonObject, descriptor);

    Dummy3DHostedViewWidget widget(m_pHolder.data());
    widget.resize(400, 300);

    widget.setSessionDescriptor(descriptor);

    // A field that was not a string reads back as empty rather than as some
    // coerced nonsense, which is what stops the workbench matching this view
    // to a session named "42" that does not exist.
    if(!descriptor.value("session_id").isString()) {
        QVERIFY2(widget.sessionId().isEmpty(),
                 "a non string session_id should not produce a session name");
    }

    // Asking for the title must be answerable whatever the descriptor held.
    widget.displayTitle();
}

//=============================================================================================================

void TestStudioDummy3DView::displayTitle_prefersTheDescriptorTitle()
{
    Dummy3DHostedViewWidget widget(m_pHolder.data());

    // A title in the descriptor wins.
    widget.setSessionDescriptor(QJsonObject{{"title", "Explicit title"},
                                            {"file", "/tmp/some-scene.fif"}});
    QCOMPARE(widget.displayTitle(), QString("Explicit title"));

    // A title that is only whitespace is not a title. Trusting it would put a
    // blank label in the workbench tab where a name should be.
    widget.setSessionDescriptor(QJsonObject{{"title", "   "},
                                            {"file", "/tmp/some-scene.fif"}});
    QVERIFY2(widget.displayTitle() != QString("   "),
             "a whitespace only title was used as the display title");
}

//=============================================================================================================

void TestStudioDummy3DView::sessionUpdate_beforeAnyDescriptorIsSurvivable()
{
    Dummy3DHostedViewWidget widget(m_pHolder.data());
    widget.resize(400, 300);

    // An update arriving before the descriptor is the startup ordering, and an
    // empty update is what a skill that produced nothing sends.
    widget.applySessionUpdate(QJsonObject{});
    widget.applySessionUpdate(QJsonObject{{"unexpected", "shape"}});

    // The widget still names nothing, since no descriptor ever arrived.
    QVERIFY(widget.sessionId().isEmpty());
}

//=============================================================================================================

void TestStudioDummy3DView::cleanupTestCase()
{
    m_pHolder.reset();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestStudioDummy3DView)
#include "test_studio_dummy3d_view.moc"
