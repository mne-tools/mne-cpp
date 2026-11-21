//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_annotation_event_utils.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Tests for Annotation / Event conversion utility functions.
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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
/**
 * Tests for annotations_from_events, events_from_annotations, and count_annotations.
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
    events << 0, 0, 1,
        100, 0, 2,
        200, 0, 3;

    const double sfreq = 100.0;
    FiffAnnotations annot = annotations_from_events(events, sfreq);

    QCOMPARE(annot.size(), std::size_t{3});

    // Check onsets
    QCOMPARE(annot[0].onset, 0.0);
    QCOMPARE(annot[1].onset, 1.0);
    QCOMPARE(annot[2].onset, 2.0);

    // Check durations are zero
    QCOMPARE(annot[0].duration, 0.0);
    QCOMPARE(annot[1].duration, 0.0);
    QCOMPARE(annot[2].duration, 0.0);

    // Without a map, descriptions should be the event id as string
    QCOMPARE(annot[0].description, std::string("1"));
    QCOMPARE(annot[1].description, std::string("2"));
    QCOMPARE(annot[2].description, std::string("3"));
}

//=============================================================================================================

void TestFiffAnnotationEventUtils::testAnnotationsFromEventsWithMap()
{
    MatrixXi events(3, 3);
    events << 0, 0, 1,
        100, 0, 2,
        200, 0, 3;

    std::map<int, std::string> descMap;
    descMap[1] = "auditory/left";
    descMap[2] = "auditory/right";
    // event id 3 not in map -> should fall back to "3"

    const double sfreq = 100.0;
    FiffAnnotations annot = annotations_from_events(events, sfreq, descMap);

    QCOMPARE(annot.size(), std::size_t{3});
    QCOMPARE(annot[0].description, std::string("auditory/left"));
    QCOMPARE(annot[1].description, std::string("auditory/right"));
    QCOMPARE(annot[2].description, std::string("3"));
}

//=============================================================================================================

void TestFiffAnnotationEventUtils::testEventsFromAnnotations()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.0, "1");
    annot.append(1.0, 0.0, "2");
    annot.append(2.0, 0.0, "3");

    const double sfreq = 100.0;
    MatrixXi events = events_from_annotations(annot, sfreq);

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

    std::map<std::string, int, std::less<>> eventIds;
    eventIds["auditory/left"] = 1;
    eventIds["auditory/right"] = 2;
    // "visual" not in map and not parseable as int -> event_id = 0

    const double sfreq = 100.0;
    MatrixXi events = events_from_annotations(annot, sfreq, eventIds);

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
    original << 0, 0, 10,
        500, 0, 20,
        1000, 0, 30,
        1500, 0, 10;

    const double sfreq = 500.0;

    std::map<int, std::string> descMap;
    descMap[10] = "stim_a";
    descMap[20] = "stim_b";
    descMap[30] = "stim_c";

    std::map<std::string, int, std::less<>> idMap;
    idMap["stim_a"] = 10;
    idMap["stim_b"] = 20;
    idMap["stim_c"] = 30;

    FiffAnnotations annot = annotations_from_events(original, sfreq, descMap);
    MatrixXi recovered = events_from_annotations(annot, sfreq, idMap);

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

    MatrixXi events = events_from_annotations(original, sfreq);
    FiffAnnotations recovered = annotations_from_events(events, sfreq);

    QCOMPARE(recovered.size(), original.size());
    for (std::size_t i = 0; i < original.size(); ++i) {
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

    const auto counts = count_annotations(annot);

    QCOMPARE(counts.size(), std::size_t{3});
    QCOMPARE(counts.at("BAD"), 3);
    QCOMPARE(counts.at("stimulus"), 2);
    QCOMPARE(counts.at("response"), 1);
}

//=============================================================================================================

void TestFiffAnnotationEventUtils::testEmptyInputs()
{
    // Empty events
    MatrixXi emptyEvents(0, 3);
    FiffAnnotations annot = annotations_from_events(emptyEvents, 100.0);
    QVERIFY(annot.empty());

    // Empty annotations
    FiffAnnotations emptyAnnot;
    MatrixXi events = events_from_annotations(emptyAnnot, 100.0);
    QCOMPARE(static_cast<int>(events.rows()), 0);

    // Count of empty
    const auto counts = count_annotations(emptyAnnot);
    QVERIFY(counts.empty());
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

    FiffAnnotations annot = annotations_from_events(events, sfreq, std::map<int, std::string>(), firstSample);

    QCOMPARE(annot.size(), std::size_t{2});
    QVERIFY(qAbs(annot[0].onset - 0.0) < 1e-9); // (1000 - 1000) / 200 = 0.0
    QVERIFY(qAbs(annot[1].onset - 1.0) < 1e-9); // (1200 - 1000) / 200 = 1.0

    // Convert back with firstSample
    MatrixXi recovered = events_from_annotations(annot, sfreq, std::map<std::string, int, std::less<>>(), firstSample);

    QCOMPARE(recovered(0, 0), 1000);
    QCOMPARE(recovered(1, 0), 1200);
}

//=============================================================================================================

QTEST_MAIN(TestFiffAnnotationEventUtils)
#include "test_fiff_annotation_event_utils.moc"
