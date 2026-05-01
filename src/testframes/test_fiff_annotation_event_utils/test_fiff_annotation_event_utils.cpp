//=============================================================================================================
/**
 * @file     test_fiff_annotation_event_utils.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 *
 * @brief    Tests for Annotation / Event conversion utility functions.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_annotation_event_utils.h>
#include <fiff/fiff_annotations.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest/QtTest>
#include <QMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * Tests for annotationsFromEvents, eventsFromAnnotations, and countAnnotations.
 */
class TestFiffAnnotationEventUtils : public QObject
{
    Q_OBJECT

private slots:
    void testAnnotationsFromEvents();
    void testAnnotationsFromEventsWithMap();
    void testEventsFromAnnotations();
    void testEventsFromAnnotationsWithMap();
    void testRoundTrip();
    void testRoundTripAnnotations();
    void testCountAnnotations();
    void testEmptyInputs();
    void testFirstSampleOffset();
};

//=============================================================================================================

void TestFiffAnnotationEventUtils::testAnnotationsFromEvents()
{
    // 3 events at samples 0, 100, 200 with event ids 1, 2, 3
    MatrixXi events(3, 3);
    events << 0,   0, 1,
              100, 0, 2,
              200, 0, 3;

    const double sfreq = 100.0;
    FiffAnnotations annot = annotationsFromEvents(events, sfreq);

    QCOMPARE(annot.size(), 3);

    // Check onsets
    QCOMPARE(annot[0].onset, 0.0);
    QCOMPARE(annot[1].onset, 1.0);
    QCOMPARE(annot[2].onset, 2.0);

    // Check durations are zero
    QCOMPARE(annot[0].duration, 0.0);
    QCOMPARE(annot[1].duration, 0.0);
    QCOMPARE(annot[2].duration, 0.0);

    // Without a map, descriptions should be the event id as string
    QCOMPARE(annot[0].description, QString("1"));
    QCOMPARE(annot[1].description, QString("2"));
    QCOMPARE(annot[2].description, QString("3"));
}

//=============================================================================================================

void TestFiffAnnotationEventUtils::testAnnotationsFromEventsWithMap()
{
    MatrixXi events(3, 3);
    events << 0,   0, 1,
              100, 0, 2,
              200, 0, 3;

    QMap<int, QString> descMap;
    descMap[1] = "auditory/left";
    descMap[2] = "auditory/right";
    // event id 3 not in map -> should fall back to "3"

    const double sfreq = 100.0;
    FiffAnnotations annot = annotationsFromEvents(events, sfreq, descMap);

    QCOMPARE(annot.size(), 3);
    QCOMPARE(annot[0].description, QString("auditory/left"));
    QCOMPARE(annot[1].description, QString("auditory/right"));
    QCOMPARE(annot[2].description, QString("3"));
}

//=============================================================================================================

void TestFiffAnnotationEventUtils::testEventsFromAnnotations()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.0, "1");
    annot.append(1.0, 0.0, "2");
    annot.append(2.0, 0.0, "3");

    const double sfreq = 100.0;
    MatrixXi events = eventsFromAnnotations(annot, sfreq);

    QCOMPARE(static_cast<int>(events.rows()), 3);
    QCOMPARE(static_cast<int>(events.cols()), 3);

    // Check samples
    QCOMPARE(events(0, 0), 0);
    QCOMPARE(events(1, 0), 100);
    QCOMPARE(events(2, 0), 200);

    // "before" column should be 0
    QCOMPARE(events(0, 1), 0);
    QCOMPARE(events(1, 1), 0);
    QCOMPARE(events(2, 1), 0);

    // event ids parsed from description strings
    QCOMPARE(events(0, 2), 1);
    QCOMPARE(events(1, 2), 2);
    QCOMPARE(events(2, 2), 3);
}

//=============================================================================================================

void TestFiffAnnotationEventUtils::testEventsFromAnnotationsWithMap()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.0, "auditory/left");
    annot.append(1.0, 0.0, "auditory/right");
    annot.append(2.0, 0.0, "visual");

    QMap<QString, int> eventIds;
    eventIds["auditory/left"]  = 1;
    eventIds["auditory/right"] = 2;
    // "visual" not in map and not parseable as int -> event_id = 0

    const double sfreq = 100.0;
    MatrixXi events = eventsFromAnnotations(annot, sfreq, eventIds);

    QCOMPARE(static_cast<int>(events.rows()), 3);
    QCOMPARE(events(0, 2), 1);
    QCOMPARE(events(1, 2), 2);
    QCOMPARE(events(2, 2), 0);
}

//=============================================================================================================

void TestFiffAnnotationEventUtils::testRoundTrip()
{
    // events -> annotations -> events
    MatrixXi original(4, 3);
    original << 0,   0, 10,
                500, 0, 20,
                1000,0, 30,
                1500,0, 10;

    const double sfreq = 500.0;

    QMap<int, QString> descMap;
    descMap[10] = "stim_a";
    descMap[20] = "stim_b";
    descMap[30] = "stim_c";

    QMap<QString, int> idMap;
    idMap["stim_a"] = 10;
    idMap["stim_b"] = 20;
    idMap["stim_c"] = 30;

    FiffAnnotations annot = annotationsFromEvents(original, sfreq, descMap);
    MatrixXi recovered = eventsFromAnnotations(annot, sfreq, idMap);

    QCOMPARE(static_cast<int>(recovered.rows()), static_cast<int>(original.rows()));
    for (int i = 0; i < static_cast<int>(original.rows()); ++i) {
        QCOMPARE(recovered(i, 0), original(i, 0));
        QCOMPARE(recovered(i, 2), original(i, 2));
    }
}

//=============================================================================================================

void TestFiffAnnotationEventUtils::testRoundTripAnnotations()
{
    // annotations -> events -> annotations
    FiffAnnotations original;
    original.append(0.0, 0.0, "5");
    original.append(0.5, 0.0, "10");
    original.append(1.0, 0.0, "15");

    const double sfreq = 1000.0;

    MatrixXi events = eventsFromAnnotations(original, sfreq);
    FiffAnnotations recovered = annotationsFromEvents(events, sfreq);

    QCOMPARE(recovered.size(), original.size());
    for (int i = 0; i < original.size(); ++i) {
        QVERIFY(qAbs(recovered[i].onset - original[i].onset) < 1e-9);
        QCOMPARE(recovered[i].description, original[i].description);
    }
}

//=============================================================================================================

void TestFiffAnnotationEventUtils::testCountAnnotations()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.0, "BAD");
    annot.append(1.0, 0.0, "stimulus");
    annot.append(2.0, 0.0, "BAD");
    annot.append(3.0, 0.0, "stimulus");
    annot.append(4.0, 0.0, "BAD");
    annot.append(5.0, 0.0, "response");

    QMap<QString, int> counts = countAnnotations(annot);

    QCOMPARE(counts.size(), 3);
    QCOMPARE(counts["BAD"], 3);
    QCOMPARE(counts["stimulus"], 2);
    QCOMPARE(counts["response"], 1);
}

//=============================================================================================================

void TestFiffAnnotationEventUtils::testEmptyInputs()
{
    // Empty events
    MatrixXi emptyEvents(0, 3);
    FiffAnnotations annot = annotationsFromEvents(emptyEvents, 100.0);
    QVERIFY(annot.isEmpty());

    // Empty annotations
    FiffAnnotations emptyAnnot;
    MatrixXi events = eventsFromAnnotations(emptyAnnot, 100.0);
    QCOMPARE(static_cast<int>(events.rows()), 0);

    // Count of empty
    QMap<QString, int> counts = countAnnotations(emptyAnnot);
    QVERIFY(counts.isEmpty());
}

//=============================================================================================================

void TestFiffAnnotationEventUtils::testFirstSampleOffset()
{
    const int firstSample = 1000;
    const double sfreq = 200.0;

    // Events with samples relative to firstSample
    MatrixXi events(2, 3);
    events << 1000, 0, 1,
              1200, 0, 2;

    FiffAnnotations annot = annotationsFromEvents(events, sfreq, QMap<int, QString>(), firstSample);

    QCOMPARE(annot.size(), 2);
    QVERIFY(qAbs(annot[0].onset - 0.0) < 1e-9);   // (1000 - 1000) / 200 = 0.0
    QVERIFY(qAbs(annot[1].onset - 1.0) < 1e-9);   // (1200 - 1000) / 200 = 1.0

    // Convert back with firstSample
    MatrixXi recovered = eventsFromAnnotations(annot, sfreq, QMap<QString, int>(), firstSample);

    QCOMPARE(recovered(0, 0), 1000);
    QCOMPARE(recovered(1, 0), 1200);
}

//=============================================================================================================

QTEST_MAIN(TestFiffAnnotationEventUtils)
#include "test_fiff_annotation_event_utils.moc"
