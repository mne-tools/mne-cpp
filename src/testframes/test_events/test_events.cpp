//=============================================================================================================
/**
 * @file     test_events.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
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
 * @brief    Comprehensive tests for EventManager — CRUD operations on events and groups.
 *           Inspired by mne-python test_event.py which tests event creation, merging, queries, and IO round-trips.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>
#include <events/eventmanager.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QDebug>
#include <QtTest>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <vector>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EVENTSLIB;

//=============================================================================================================
/**
 * @brief Full lifecycle tests for EventManager: add/query/move/delete events and groups,
 *        group operations (merge, duplicate, rename, color), and sample-range queries.
 */
class TestEvents : public QObject
{
    Q_OBJECT

public:
    TestEvents();

private slots:
    void initTestCase();

    // Event CRUD
    void testAddEventDefaultGroup();
    void testAddEventWithGroup();
    void testGetEvent();
    void testGetAllEvents();
    void testGetEventsInSample();
    void testGetEventsBetween();
    void testGetEventsBetweenWithGroup();
    void testGetEventsBetweenWithGroups();
    void testMoveEvent();
    void testDeleteEvent();
    void testDeleteEvents();

    // Group CRUD
    void testAddGroup();
    void testAddGroupWithColor();
    void testGetGroup();
    void testGetAllGroups();
    void testRenameGroup();
    void testSetGroupColor();
    void testDeleteGroup();
    void testDeleteGroups();

    // Group operations
    void testGetEventsInGroup();
    void testGetEventsInGroups();
    void testAddEventToGroup();
    void testAddEventsToGroup();
    void testMergeGroups();
    void testDuplicateGroup();
    void testDeleteEventsInGroup();

    // Edge cases
    void testDeleteNonExistentEvent();
    void testGetNonExistentEvent();
    void testEmptyManager();

    // RgbColor struct
    void testRgbColor();

    void cleanupTestCase();
};

//=============================================================================================================

TestEvents::TestEvents()
{
}

//=============================================================================================================

void TestEvents::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    qInfo() << "TestEvents: Starting EventManager unit tests";
}

//=============================================================================================================

void TestEvents::testAddEventDefaultGroup()
{
    EventManager mgr;
    QCOMPARE((int)mgr.getNumEvents(), 0);

    Event e = mgr.addEvent(100);
    QCOMPARE((int)mgr.getNumEvents(), 1);
    QCOMPARE(e.sample, 100);
}

//=============================================================================================================

void TestEvents::testAddEventWithGroup()
{
    EventManager mgr;
    EventGroup g = mgr.addGroup("TestGroup");

    Event e = mgr.addEvent(200, g.id);
    QCOMPARE((int)mgr.getNumEvents(), 1);
    QCOMPARE(e.sample, 200);
    QCOMPARE(e.groupId, g.id);
}

//=============================================================================================================

void TestEvents::testGetEvent()
{
    EventManager mgr;
    Event added = mgr.addEvent(300);

    Event retrieved = mgr.getEvent(added.id);
    QCOMPARE(retrieved.id, added.id);
    QCOMPARE(retrieved.sample, 300);
}

//=============================================================================================================

void TestEvents::testGetAllEvents()
{
    EventManager mgr;
    mgr.addEvent(100);
    mgr.addEvent(200);
    mgr.addEvent(300);

    auto all = mgr.getAllEvents();
    QCOMPARE((int)all->size(), 3);
}

//=============================================================================================================

void TestEvents::testGetEventsInSample()
{
    EventManager mgr;
    mgr.addEvent(100);
    mgr.addEvent(100);  // same sample
    mgr.addEvent(200);

    auto inSample = mgr.getEventsInSample(100);
    QCOMPARE((int)inSample->size(), 2);

    auto inSample2 = mgr.getEventsInSample(200);
    QCOMPARE((int)inSample2->size(), 1);

    auto empty = mgr.getEventsInSample(999);
    QCOMPARE((int)empty->size(), 0);
}

//=============================================================================================================

void TestEvents::testGetEventsBetween()
{
    EventManager mgr;
    mgr.addEvent(100);
    mgr.addEvent(200);
    mgr.addEvent(300);
    mgr.addEvent(400);

    auto between = mgr.getEventsBetween(150, 350);
    QCOMPARE((int)between->size(), 2);  // 200 and 300

    auto all = mgr.getEventsBetween(0, 1000);
    QCOMPARE((int)all->size(), 4);

    auto none = mgr.getEventsBetween(500, 600);
    QCOMPARE((int)none->size(), 0);
}

//=============================================================================================================

void TestEvents::testGetEventsBetweenWithGroup()
{
    EventManager mgr;
    EventGroup g1 = mgr.addGroup("Group1");
    EventGroup g2 = mgr.addGroup("Group2");

    mgr.addEvent(100, g1.id);
    mgr.addEvent(200, g2.id);
    mgr.addEvent(300, g1.id);

    auto filtered = mgr.getEventsBetween(0, 1000, g1.id);
    QCOMPARE((int)filtered->size(), 2);

    auto filtered2 = mgr.getEventsBetween(0, 1000, g2.id);
    QCOMPARE((int)filtered2->size(), 1);
}

//=============================================================================================================

void TestEvents::testGetEventsBetweenWithGroups()
{
    EventManager mgr;
    EventGroup g1 = mgr.addGroup("Group1");
    EventGroup g2 = mgr.addGroup("Group2");
    EventGroup g3 = mgr.addGroup("Group3");

    mgr.addEvent(100, g1.id);
    mgr.addEvent(200, g2.id);
    mgr.addEvent(300, g3.id);

    std::vector<idNum> groups = {g1.id, g2.id};
    auto filtered = mgr.getEventsBetween(0, 1000, groups);
    QCOMPARE((int)filtered->size(), 2);
}

//=============================================================================================================

void TestEvents::testMoveEvent()
{
    EventManager mgr;
    Event e = mgr.addEvent(100);

    QVERIFY(mgr.moveEvent(e.id, 500));

    Event moved = mgr.getEvent(e.id);
    QCOMPARE(moved.sample, 500);
}

//=============================================================================================================

void TestEvents::testDeleteEvent()
{
    EventManager mgr;
    Event e = mgr.addEvent(100);
    QCOMPARE((int)mgr.getNumEvents(), 1);

    QVERIFY(mgr.deleteEvent(e.id));
    QCOMPARE((int)mgr.getNumEvents(), 0);
}

//=============================================================================================================

void TestEvents::testDeleteEvents()
{
    EventManager mgr;
    Event e1 = mgr.addEvent(100);
    Event e2 = mgr.addEvent(200);
    Event e3 = mgr.addEvent(300);

    std::vector<idNum> toDelete = {e1.id, e3.id};
    QVERIFY(mgr.deleteEvents(toDelete));
    QCOMPARE((int)mgr.getNumEvents(), 1);

    // The remaining event should be e2
    auto remaining = mgr.getAllEvents();
    QCOMPARE((*remaining)[0].sample, 200);
}

//=============================================================================================================

void TestEvents::testAddGroup()
{
    EventManager mgr;
    // Manager starts with no user groups (default group created lazily)
    EventGroup g = mgr.addGroup("MyGroup");
    QCOMPARE(g.name, std::string("MyGroup"));
    QVERIFY(g.id > 0);
}

//=============================================================================================================

void TestEvents::testAddGroupWithColor()
{
    EventManager mgr;
    RgbColor color(255, 0, 128);
    EventGroup g = mgr.addGroup("Colored", color);

    QCOMPARE(g.name, std::string("Colored"));
    QCOMPARE(g.color.r, (unsigned char)255);
    QCOMPARE(g.color.g, (unsigned char)0);
    QCOMPARE(g.color.b, (unsigned char)128);
}

//=============================================================================================================

void TestEvents::testGetGroup()
{
    EventManager mgr;
    EventGroup created = mgr.addGroup("TestGet");
    EventGroup retrieved = mgr.getGroup(created.id);

    QCOMPARE(retrieved.id, created.id);
    QCOMPARE(retrieved.name, std::string("TestGet"));
}

//=============================================================================================================

void TestEvents::testGetAllGroups()
{
    EventManager mgr;
    mgr.addGroup("G1");
    mgr.addGroup("G2");
    mgr.addGroup("G3");

    auto allGroups = mgr.getAllGroups();
    QVERIFY((int)allGroups->size() >= 3);
}

//=============================================================================================================

void TestEvents::testRenameGroup()
{
    EventManager mgr;
    EventGroup g = mgr.addGroup("OldName");
    mgr.renameGroup(g.id, "NewName");

    EventGroup renamed = mgr.getGroup(g.id);
    QCOMPARE(renamed.name, std::string("NewName"));
}

//=============================================================================================================

void TestEvents::testSetGroupColor()
{
    EventManager mgr;
    EventGroup g = mgr.addGroup("ColorTest");
    RgbColor newColor(10, 20, 30);
    mgr.setGroupColor(g.id, newColor);

    EventGroup updated = mgr.getGroup(g.id);
    QCOMPARE(updated.color.r, (unsigned char)10);
    QCOMPARE(updated.color.g, (unsigned char)20);
    QCOMPARE(updated.color.b, (unsigned char)30);
}

//=============================================================================================================

void TestEvents::testDeleteGroup()
{
    EventManager mgr;
    EventGroup g = mgr.addGroup("ToDelete");
    int numBefore = mgr.getNumGroups();

    QVERIFY(mgr.deleteGroup(g.id));
    QCOMPARE(mgr.getNumGroups(), numBefore - 1);
}

//=============================================================================================================

void TestEvents::testDeleteGroups()
{
    EventManager mgr;
    EventGroup g1 = mgr.addGroup("Del1");
    EventGroup g2 = mgr.addGroup("Del2");
    int numBefore = mgr.getNumGroups();

    std::vector<idNum> toDelete = {g1.id, g2.id};
    QVERIFY(mgr.deleteGroups(toDelete));
    QCOMPARE(mgr.getNumGroups(), numBefore - 2);
}

//=============================================================================================================

void TestEvents::testGetEventsInGroup()
{
    EventManager mgr;
    EventGroup g = mgr.addGroup("QueryGroup");
    mgr.addEvent(100, g.id);
    mgr.addEvent(200, g.id);
    mgr.addEvent(300);  // default group

    auto inGroup = mgr.getEventsInGroup(g.id);
    QCOMPARE((int)inGroup->size(), 2);
}

//=============================================================================================================

void TestEvents::testGetEventsInGroups()
{
    EventManager mgr;
    EventGroup g1 = mgr.addGroup("QG1");
    EventGroup g2 = mgr.addGroup("QG2");
    mgr.addEvent(100, g1.id);
    mgr.addEvent(200, g2.id);
    mgr.addEvent(300, g1.id);

    std::vector<idNum> groups = {g1.id, g2.id};
    auto inGroups = mgr.getEventsInGroups(groups);
    QCOMPARE((int)inGroups->size(), 3);
}

//=============================================================================================================

void TestEvents::testAddEventToGroup()
{
    EventManager mgr;
    EventGroup g = mgr.addGroup("MoveToGroup");
    Event e = mgr.addEvent(100);  // default group

    QVERIFY(mgr.addEventToGroup(e.id, g.id));

    Event moved = mgr.getEvent(e.id);
    QCOMPARE(moved.groupId, g.id);
}

//=============================================================================================================

void TestEvents::testAddEventsToGroup()
{
    EventManager mgr;
    EventGroup g = mgr.addGroup("BulkMove");
    Event e1 = mgr.addEvent(100);
    Event e2 = mgr.addEvent(200);

    std::vector<idNum> eventIds = {e1.id, e2.id};
    QVERIFY(mgr.addEventsToGroup(eventIds, g.id));

    auto inGroup = mgr.getEventsInGroup(g.id);
    QCOMPARE((int)inGroup->size(), 2);
}

//=============================================================================================================

void TestEvents::testMergeGroups()
{
    EventManager mgr;
    EventGroup g1 = mgr.addGroup("Merge1");
    EventGroup g2 = mgr.addGroup("Merge2");
    mgr.addEvent(100, g1.id);
    mgr.addEvent(200, g2.id);
    mgr.addEvent(300, g1.id);

    std::vector<idNum> toMerge = {g1.id, g2.id};
    EventGroup merged = mgr.mergeGroups(toMerge, "Merged");

    QCOMPARE(merged.name, std::string("Merged"));

    auto inMerged = mgr.getEventsInGroup(merged.id);
    QCOMPARE((int)inMerged->size(), 3);
}

//=============================================================================================================

void TestEvents::testDuplicateGroup()
{
    EventManager mgr;
    EventGroup g = mgr.addGroup("Original");
    mgr.addEvent(100, g.id);
    mgr.addEvent(200, g.id);

    EventGroup dup = mgr.duplicateGroup(g.id, "Duplicate");

    QCOMPARE(dup.name, std::string("Duplicate"));

    auto inDup = mgr.getEventsInGroup(dup.id);
    QCOMPARE((int)inDup->size(), 2);

    // Original group should still have its events
    auto inOrig = mgr.getEventsInGroup(g.id);
    QCOMPARE((int)inOrig->size(), 2);
}

//=============================================================================================================

void TestEvents::testDeleteEventsInGroup()
{
    EventManager mgr;
    EventGroup g = mgr.addGroup("DelEvents");
    mgr.addEvent(100, g.id);
    mgr.addEvent(200, g.id);
    mgr.addEvent(300);  // different group

    QVERIFY(mgr.deleteEventsInGroup(g.id));
    QCOMPARE((int)mgr.getNumEvents(), 1);  // only the default-group event remains
}

//=============================================================================================================

void TestEvents::testDeleteNonExistentEvent()
{
    EventManager mgr;
    // Deleting a non-existent event should return false gracefully
    bool result = mgr.deleteEvent(999999);
    QVERIFY(!result);
}

//=============================================================================================================

void TestEvents::testGetNonExistentEvent()
{
    EventManager mgr;
    // Getting a non-existent event should return an empty/invalid result
    auto events = mgr.getEvents({999999});
    QVERIFY(events);
    QCOMPARE((int)events->size(), 0);
}

//=============================================================================================================

void TestEvents::testEmptyManager()
{
    EventManager mgr;

    QCOMPARE((int)mgr.getNumEvents(), 0);

    auto all = mgr.getAllEvents();
    QCOMPARE((int)all->size(), 0);

    auto between = mgr.getEventsBetween(0, 1000);
    QCOMPARE((int)between->size(), 0);
}

//=============================================================================================================

void TestEvents::testRgbColor()
{
    // Default constructor
    RgbColor defaultColor;
    Q_UNUSED(defaultColor);

    // RGB constructor
    RgbColor rgb(100, 150, 200);
    QCOMPARE(rgb.r, (unsigned char)100);
    QCOMPARE(rgb.g, (unsigned char)150);
    QCOMPARE(rgb.b, (unsigned char)200);

    // RGBA constructor
    RgbColor rgba(10, 20, 30, 40);
    QCOMPARE(rgba.r, (unsigned char)10);
    QCOMPARE(rgba.g, (unsigned char)20);
    QCOMPARE(rgba.b, (unsigned char)30);
    QCOMPARE(rgba.a, (unsigned char)40);
}

//=============================================================================================================

void TestEvents::cleanupTestCase()
{
    qInfo() << "TestEvents: All tests completed";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestEvents)
#include "test_events.moc"
