//=============================================================================================================
/**
 * @file     test_multimodal_scene.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 *       the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *       and the following disclaimer in the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Tests for MultimodalScene, PickResult and the v2.3.0 ElectrodeArray
 *           extension to ElectrodeObject.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/scene/multimodalscene.h>
#include <disp3D/scene/pickresult.h>
#include <disp3D/renderable/electrodeobject.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSignalSpy>
#include <QtTest>

using namespace DISP3DLIB;

//=============================================================================================================
class TestMultimodalScene : public QObject
{
    Q_OBJECT

private slots:
    // PickResult
    void testPickResultDefaults();
    void testIsHitHelper();

    // MultimodalScene — registry
    void testAddAndFindLayer();
    void testAddReplacesByIdInPlace();
    void testRemoveLayer();
    void testRemoveUnknownLayerReturnsFalse();
    void testClearEmitsLayersChanged();
    void testRejectsEmptyId();

    // MultimodalScene — ordering
    void testLayersSortedByKindThenDrawOrder();
    void testDrawOrderTieBreaker();

    // MultimodalScene — visibility / opacity
    void testSetLayerVisibleEmitsChange();
    void testSetLayerOpacityClampedToUnit();
    void testNoSignalOnUnchangedFlag();

    // MultimodalScene — timeline
    void testCurrentTimeSampleDefaultsToMinusOne();
    void testSetCurrentTimeSampleEmitsAndClampsNegative();
    void testSetCurrentTimeSampleNoSignalOnSameValue();

    // MultimodalScene — picking
    void testReportPickStoresLastAndEmits();

    // MultimodalScene — bounds
    void testWorldBoundsFallsBackToUnitCubeWhenEmpty();
    void testWorldBoundsUnionWithRegisteredFn();

    // ElectrodeArray
    void testElectrodeArrayDefaultLayoutIsDepth();
    void testStripLayoutSkipsCylinderGeometry();
};

//=============================================================================================================
// PickResult
//=============================================================================================================

void TestMultimodalScene::testPickResultDefaults()
{
    PickResult r;
    QCOMPARE(r.kind, PickKind::None);
    QCOMPARE(r.objectId, qint64(-1));
    QCOMPARE(r.hemisphere, -1);
    QCOMPARE(r.sliceOrientation, -1);
    QCOMPARE(r.timeSample, -1);
    QVERIFY(std::isnan(r.value));
    QVERIFY(r.label.isEmpty());
    QVERIFY(r.sourceId.isEmpty());
    QVERIFY(r.extras.isEmpty());
}

void TestMultimodalScene::testIsHitHelper()
{
    PickResult r;
    QVERIFY(!isHit(r));
    r.kind = PickKind::CorticalVertex;
    QVERIFY(isHit(r));
}

//=============================================================================================================
// MultimodalScene — registry
//=============================================================================================================

namespace
{

SceneLayer makeLayer(const QString& id,
                     SceneLayerKind kind = SceneLayerKind::Custom,
                     int drawOrder = 0)
{
    SceneLayer l;
    l.id = id;
    l.displayName = id;
    l.kind = kind;
    l.drawOrder = drawOrder;
    return l;
}

} // namespace

void TestMultimodalScene::testAddAndFindLayer()
{
    MultimodalScene s;
    QSignalSpy spy(&s, &MultimodalScene::layersChanged);

    s.addLayer(makeLayer("cortex_lh", SceneLayerKind::BrainSurface));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(s.layers().size(), 1);
    const SceneLayer* l = s.findLayer("cortex_lh");
    QVERIFY(l != nullptr);
    QCOMPARE(l->kind, SceneLayerKind::BrainSurface);
    QVERIFY(s.findLayer("missing") == nullptr);
}

void TestMultimodalScene::testAddReplacesByIdInPlace()
{
    MultimodalScene s;
    s.addLayer(makeLayer("seeg_LH", SceneLayerKind::Electrode));

    SceneLayer replacement = makeLayer("seeg_LH", SceneLayerKind::Electrode);
    replacement.displayName = "replaced";
    s.addLayer(replacement);

    QCOMPARE(s.layers().size(), 1);
    QCOMPARE(s.findLayer("seeg_LH")->displayName, QString("replaced"));
}

void TestMultimodalScene::testRemoveLayer()
{
    MultimodalScene s;
    s.addLayer(makeLayer("a"));
    s.addLayer(makeLayer("b"));

    QSignalSpy spy(&s, &MultimodalScene::layersChanged);
    QVERIFY(s.removeLayer("a"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(s.layers().size(), 1);
    QCOMPARE(s.layers().first().id, QString("b"));
}

void TestMultimodalScene::testRemoveUnknownLayerReturnsFalse()
{
    MultimodalScene s;
    QVERIFY(!s.removeLayer("nope"));
}

void TestMultimodalScene::testClearEmitsLayersChanged()
{
    MultimodalScene s;
    s.addLayer(makeLayer("a"));

    QSignalSpy spy(&s, &MultimodalScene::layersChanged);
    s.clear();
    QCOMPARE(spy.count(), 1);
    QVERIFY(s.layers().isEmpty());

    // Clearing an already-empty scene is a no-op (no spurious signal).
    s.clear();
    QCOMPARE(spy.count(), 1);
}

void TestMultimodalScene::testRejectsEmptyId()
{
    MultimodalScene s;
    QSignalSpy spy(&s, &MultimodalScene::layersChanged);
    s.addLayer(makeLayer(""));
    QCOMPARE(spy.count(), 0);
    QVERIFY(s.layers().isEmpty());
}

//=============================================================================================================
// Ordering
//=============================================================================================================

void TestMultimodalScene::testLayersSortedByKindThenDrawOrder()
{
    MultimodalScene s;
    // Insertion order intentionally reversed against expected draw order.
    s.addLayer(makeLayer("net",   SceneLayerKind::Network));
    s.addLayer(makeLayer("seeg",  SceneLayerKind::Electrode));
    s.addLayer(makeLayer("cortex", SceneLayerKind::BrainSurface));

    const auto layers = s.layers();
    QCOMPARE(layers.size(), 3);
    QCOMPARE(layers[0].kind, SceneLayerKind::BrainSurface);
    QCOMPARE(layers[1].kind, SceneLayerKind::Electrode);
    QCOMPARE(layers[2].kind, SceneLayerKind::Network);
}

void TestMultimodalScene::testDrawOrderTieBreaker()
{
    MultimodalScene s;
    s.addLayer(makeLayer("seeg_b", SceneLayerKind::Electrode, /*drawOrder*/ 5));
    s.addLayer(makeLayer("seeg_a", SceneLayerKind::Electrode, /*drawOrder*/ 1));

    const auto layers = s.layers();
    QCOMPARE(layers.first().id, QString("seeg_a"));
    QCOMPARE(layers.last().id,  QString("seeg_b"));
}

//=============================================================================================================
// Visibility / opacity
//=============================================================================================================

void TestMultimodalScene::testSetLayerVisibleEmitsChange()
{
    MultimodalScene s;
    s.addLayer(makeLayer("a"));

    QSignalSpy spy(&s, &MultimodalScene::layersChanged);
    s.setLayerVisible("a", false);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!s.findLayer("a")->visible);
}

void TestMultimodalScene::testSetLayerOpacityClampedToUnit()
{
    MultimodalScene s;
    s.addLayer(makeLayer("a"));

    s.setLayerOpacity("a", 2.5f);
    QCOMPARE(s.findLayer("a")->opacity, 1.0f);

    s.setLayerOpacity("a", -0.5f);
    QCOMPARE(s.findLayer("a")->opacity, 0.0f);
}

void TestMultimodalScene::testNoSignalOnUnchangedFlag()
{
    MultimodalScene s;
    s.addLayer(makeLayer("a"));
    QSignalSpy spy(&s, &MultimodalScene::layersChanged);
    s.setLayerVisible("a", true);   // already true
    QCOMPARE(spy.count(), 0);
}

//=============================================================================================================
// Timeline
//=============================================================================================================

void TestMultimodalScene::testCurrentTimeSampleDefaultsToMinusOne()
{
    MultimodalScene s;
    QCOMPARE(s.currentTimeSample(), -1);
}

void TestMultimodalScene::testSetCurrentTimeSampleEmitsAndClampsNegative()
{
    MultimodalScene s;
    QSignalSpy spy(&s, &MultimodalScene::timeSampleChanged);
    s.setCurrentTimeSample(42);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toInt(), 42);

    s.setCurrentTimeSample(-7);
    QCOMPARE(s.currentTimeSample(), -1);
}

void TestMultimodalScene::testSetCurrentTimeSampleNoSignalOnSameValue()
{
    MultimodalScene s;
    s.setCurrentTimeSample(10);
    QSignalSpy spy(&s, &MultimodalScene::timeSampleChanged);
    s.setCurrentTimeSample(10);
    QCOMPARE(spy.count(), 0);
}

//=============================================================================================================
// Picking
//=============================================================================================================

void TestMultimodalScene::testReportPickStoresLastAndEmits()
{
    MultimodalScene s;
    QSignalSpy spy(&s, &MultimodalScene::picked);

    PickResult p;
    p.kind = PickKind::ElectrodeContact;
    p.objectId = 17;
    p.label = "LH3";
    p.world = QVector3D(1.0f, 2.0f, 3.0f);
    p.sourceId = "seeg_LH";

    s.reportPick(p);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(s.lastPick().kind, PickKind::ElectrodeContact);
    QCOMPARE(s.lastPick().label, QString("LH3"));
    QCOMPARE(s.lastPick().objectId, qint64(17));
}

//=============================================================================================================
// Bounds
//=============================================================================================================

void TestMultimodalScene::testWorldBoundsFallsBackToUnitCubeWhenEmpty()
{
    MultimodalScene s;
    QVector3D mn;
    QVector3D mx;
    s.worldBounds(mn, mx);
    QCOMPARE(mn, QVector3D(-1, -1, -1));
    QCOMPARE(mx, QVector3D( 1,  1,  1));
}

void TestMultimodalScene::testWorldBoundsUnionWithRegisteredFn()
{
    MultimodalScene s;
    s.addLayer(makeLayer("a", SceneLayerKind::Custom));
    s.addLayer(makeLayer("b", SceneLayerKind::Custom));

    // Each Custom layer reports a fixed AABB via the extras map.
    s.registerBoundsFn(SceneLayerKind::Custom,
        [](const SceneLayer& l, QVector3D& mn, QVector3D& mx) {
            if (l.id == QString("a")) {
                mn = QVector3D(-1, -1, -1);
                mx = QVector3D( 1,  1,  1);
                return true;
            }
            mn = QVector3D(2, 2, 2);
            mx = QVector3D(3, 3, 3);
            return true;
        });

    QVector3D mn;
    QVector3D mx;
    s.worldBounds(mn, mx);
    QCOMPARE(mn, QVector3D(-1, -1, -1));
    QCOMPARE(mx, QVector3D( 3,  3,  3));
}

//=============================================================================================================
// ElectrodeArray
//=============================================================================================================

void TestMultimodalScene::testElectrodeArrayDefaultLayoutIsDepth()
{
    ElectrodeArray a;
    QCOMPARE(a.layout, ElectrodeLayout::Depth);
    QCOMPARE(a.gridRows, 1);
    QCOMPARE(a.gridCols, 1);
}

void TestMultimodalScene::testStripLayoutSkipsCylinderGeometry()
{
    // Build a Strip array with two contacts and verify generateShaftGeometry
    // emits no triangles (the renderer reaches the contacts via the per-
    // instance buffer instead).
    ElectrodeArray strip;
    strip.label = "GridStrip";
    strip.layout = ElectrodeLayout::Strip;
    strip.gridRows = 1;
    strip.gridCols = 2;
    ElectrodeContact c0;
    c0.name = "S1";
    c0.position = QVector3D(0, 0, 0);
    ElectrodeContact c1;
    c1.name = "S2";
    c1.position = QVector3D(1, 0, 0);
    strip.contacts = { c0, c1 };

    ElectrodeObject obj;
    obj.setArrays({ strip });

    QVector<float> verts;
    QVector<unsigned int> idx;
    obj.generateShaftGeometry(verts, idx);

    QVERIFY(verts.isEmpty());
    QVERIFY(idx.isEmpty());

    // Per-instance contact data must still be emitted.
    QVector<float> instances;
    obj.generateContactInstances(instances);
    QCOMPARE(instances.size(), 2 * 9);
}

QTEST_GUILESS_MAIN(TestMultimodalScene)
#include "test_multimodal_scene.moc"
