//=============================================================================================================
/**
 * @file     test_mne_inspect_multimodal.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Headless integration tests for MNE Inspect's multimodal scene wiring
 *           wiring: ElectrodesPlugin + MriSlicesPlugin + PickReadoutModel
 *           composed on top of a single DISP3DLIB::MultimodalScene.
 *
 *           These tests exercise model state, not widget pixels — the
 *           MainWindow itself is intentionally not constructed so the
 *           tests can run on headless CI.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pickreadoutmodel.h"
#include "fixtures/inspect_demo_fixtures.h"

#include <electrodes.h>
#include <mri_slices.h>

#include <disp3D/renderable/electrodeobject.h>
#include <disp3D/scene/multimodalscene.h>
#include <disp3D/scene/pickresult.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSignalSpy>
#include <QTest>
#include <QVector3D>

using namespace DISP3DLIB;
using namespace ELECTRODESPLUGIN;
using namespace MRISLICESPLUGIN;
using namespace MNEINSPECT;

//=============================================================================================================

class TestMneInspectMultimodal : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void twoPluginsShareSceneAndToggleIndependently();
    void timeCursorPropagatesToSubscribers();
    void overlayThresholdsRoundTrip();
    void overlayThresholdsClampMonotone();
    void pickContactDrivesReadoutAndJumpsMri();
    void pickMriVoxelHighlightsNearestContact();
    void pickCorticalVertexFormatsReadout();

private:
    static QVector<ElectrodeArray> sampleArrays();
};

//=============================================================================================================

void TestMneInspectMultimodal::initTestCase()
{
    // No-op.
}

//=============================================================================================================

QVector<ElectrodeArray> TestMneInspectMultimodal::sampleArrays()
{
    // Delegate to the shared demo fixture so test scenes and doc-shot
    // renders stay byte-identical.
    return MNEINSPECT::demoOneDepthStrip();
}

//=============================================================================================================

void TestMneInspectMultimodal::twoPluginsShareSceneAndToggleIndependently()
{
    MultimodalScene scene;
    ElectrodesPlugin electrodes;
    MriSlicesPlugin mri;

    electrodes.setArrays(sampleArrays());
    electrodes.attachScene(&scene);
    mri.attachScene(&scene);

    // Electrodes layer registered. (MRI plugin has no volume loaded so it
    // does not register slice layers — that path is tested in
    // test_mri_slices_plugin and is not the focus here.)
    const SceneLayer* eLayer = scene.findLayer(electrodes.sceneLayerId());
    QVERIFY(eLayer != nullptr);
    QVERIFY(eLayer->visible);

    // Synthesise a third layer of MriSlice kind so we can verify per-kind
    // toggling without an MGH file.
    SceneLayer fake;
    fake.id = QStringLiteral("mri_axial");
    fake.displayName = QStringLiteral("MRI Axial");
    fake.kind = SceneLayerKind::MriSlice;
    fake.payload = std::shared_ptr<void>(reinterpret_cast<void*>(0x1), [](void*){});
    scene.addLayer(fake);
    QVERIFY(scene.findLayer(fake.id) != nullptr);

    QSignalSpy layersSpy(&scene, &MultimodalScene::layersChanged);

    // Toggle electrodes off; MRI layer remains visible.
    scene.setLayerVisible(electrodes.sceneLayerId(), false);
    QCOMPARE(scene.findLayer(electrodes.sceneLayerId())->visible, false);
    QCOMPARE(scene.findLayer(fake.id)->visible, true);
    QVERIFY(layersSpy.count() >= 1);

    // Toggle MRI off; electrodes still hidden.
    scene.setLayerVisible(fake.id, false);
    QCOMPARE(scene.findLayer(electrodes.sceneLayerId())->visible, false);
    QCOMPARE(scene.findLayer(fake.id)->visible, false);

    // Bring both back.
    scene.setLayerVisible(electrodes.sceneLayerId(), true);
    scene.setLayerVisible(fake.id, true);
    QCOMPARE(scene.findLayer(electrodes.sceneLayerId())->visible, true);
    QCOMPARE(scene.findLayer(fake.id)->visible, true);
}

//=============================================================================================================

void TestMneInspectMultimodal::timeCursorPropagatesToSubscribers()
{
    MultimodalScene scene;

    QSignalSpy spy(&scene, &MultimodalScene::timeCursorChanged);
    QCOMPARE(scene.timeCursor(), 0.0);

    scene.setTimeCursor(0.250);
    QCOMPARE(scene.timeCursor(), 0.250);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toDouble(), 0.250);

    // Setting the same value must not re-emit.
    scene.setTimeCursor(0.250);
    QCOMPARE(spy.count(), 1);

    scene.setTimeCursor(-0.100);
    QCOMPARE(scene.timeCursor(), -0.100);
    QCOMPARE(spy.count(), 2);
}

//=============================================================================================================

void TestMneInspectMultimodal::overlayThresholdsRoundTrip()
{
    MultimodalScene scene;
    QSignalSpy spy(&scene, &MultimodalScene::overlayThresholdsChanged);

    scene.setOverlayThresholds(0.1f, 0.5f, 0.9f);
    QCOMPARE(scene.overlayFmin(), 0.1f);
    QCOMPARE(scene.overlayFmid(), 0.5f);
    QCOMPARE(scene.overlayFmax(), 0.9f);
    QCOMPARE(spy.count(), 1);
    const auto args = spy.first();
    QCOMPARE(args.at(0).toFloat(), 0.1f);
    QCOMPARE(args.at(1).toFloat(), 0.5f);
    QCOMPARE(args.at(2).toFloat(), 0.9f);

    // Identical values should not re-emit.
    scene.setOverlayThresholds(0.1f, 0.5f, 0.9f);
    QCOMPARE(spy.count(), 1);
}

//=============================================================================================================

void TestMneInspectMultimodal::overlayThresholdsClampMonotone()
{
    MultimodalScene scene;
    scene.setOverlayThresholds(0.5f, 0.2f, 0.1f);
    // Clamped to fmin <= fmid <= fmax.
    QCOMPARE(scene.overlayFmin(), 0.5f);
    QCOMPARE(scene.overlayFmid(), 0.5f);
    QCOMPARE(scene.overlayFmax(), 0.5f);
}

//=============================================================================================================

void TestMneInspectMultimodal::pickContactDrivesReadoutAndJumpsMri()
{
    MultimodalScene scene;
    ElectrodesPlugin electrodes;
    MriSlicesPlugin mri;
    electrodes.setArrays(sampleArrays());
    electrodes.attachScene(&scene);
    mri.attachScene(&scene);

    PickReadoutModel model;
    model.attachScene(&scene);
    model.setElectrodesPlugin(&electrodes);
    model.setMriSlicesPlugin(&mri);

    QSignalSpy readoutSpy(&model, &PickReadoutModel::readoutChanged);
    QSignalSpy crossSpy(&mri, &MriSlicesPlugin::crosshairChanged);

    PickResult pick;
    pick.kind = PickKind::ElectrodeContact;
    pick.label = QStringLiteral("LA2");
    pick.world = QVector3D(0.0f, 0.0f, 0.02f);
    pick.sourceId = electrodes.sceneLayerId();
    pick.value = 12.5f;
    scene.reportPick(pick);

    QCOMPARE(readoutSpy.count(), 1);
    QVERIFY(model.labelRow().contains(QStringLiteral("LA2")));
    QVERIFY(model.worldRow().contains(QStringLiteral("0.0200")));
    QVERIFY(model.valueRow().contains(QStringLiteral("12.5000")));

    // The cross-modality jump moves the MRI crosshair even if no volume
    // is loaded — crosshairChanged still fires because the value changed.
    QCOMPARE(crossSpy.count(), 1);
    const auto crossArgs = crossSpy.first().first().value<QVector3D>();
    QCOMPARE(crossArgs, QVector3D(0.0f, 0.0f, 0.02f));
}

//=============================================================================================================

void TestMneInspectMultimodal::pickMriVoxelHighlightsNearestContact()
{
    MultimodalScene scene;
    ElectrodesPlugin electrodes;
    MriSlicesPlugin mri;
    electrodes.setArrays(sampleArrays());
    electrodes.attachScene(&scene);
    mri.attachScene(&scene);

    PickReadoutModel model;
    model.attachScene(&scene);
    model.setElectrodesPlugin(&electrodes);
    model.setMriSlicesPlugin(&mri);

    QSignalSpy contactSpy(&electrodes, &ElectrodesPlugin::contactPicked);
    QSignalSpy readoutSpy(&model, &PickReadoutModel::readoutChanged);

    PickResult pick;
    pick.kind = PickKind::MriVoxel;
    pick.world = QVector3D(0.0f, 0.0f, 0.011f);  // closest to LA1 (z=0.01)
    pick.voxel = QVector3D(10.0f, 20.0f, 30.0f);
    pick.sliceOrientation = 0;
    pick.sourceId = QStringLiteral("mri_axial");
    scene.reportPick(pick);

    QCOMPARE(readoutSpy.count(), 1);
    QVERIFY(model.voxelRow().contains(QStringLiteral("ori=0")));

    // electrodes plugin should report contactPicked AT LEAST once with
    // the nearest contact name ("LA1").
    QVERIFY(contactSpy.count() >= 1);
    QString lastName;
    for (const auto& args : contactSpy) {
        lastName = args.first().toString();
    }
    QCOMPARE(lastName, QStringLiteral("LA1"));
    QCOMPARE(electrodes.selectedContact(), QStringLiteral("LA1"));
}

//=============================================================================================================

void TestMneInspectMultimodal::pickCorticalVertexFormatsReadout()
{
    MultimodalScene scene;
    MriSlicesPlugin mri;
    mri.attachScene(&scene);

    PickReadoutModel model;
    model.attachScene(&scene);
    model.setMriSlicesPlugin(&mri);

    QSignalSpy readoutSpy(&model, &PickReadoutModel::readoutChanged);
    QSignalSpy crossSpy(&mri, &MriSlicesPlugin::crosshairChanged);

    PickResult pick;
    pick.kind = PickKind::CorticalVertex;
    pick.world = QVector3D(-0.030f, 0.012f, 0.045f);
    pick.hemisphere = 0;
    pick.objectId = 12345;
    pick.value = -1.75f;
    pick.sourceId = QStringLiteral("cortex_lh");
    scene.reportPick(pick);

    QCOMPARE(readoutSpy.count(), 1);
    QVERIFY(model.labelRow().contains(QStringLiteral("LH")));
    QVERIFY(model.labelRow().contains(QStringLiteral("12345")));
    QVERIFY(model.voxelRow().contains(QStringLiteral("cortex_lh")));
    QVERIFY(model.valueRow().contains(QStringLiteral("-1.7500")));

    // Surface picks also jump the MRI crosshair.
    QCOMPARE(crossSpy.count(), 1);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneInspectMultimodal)
#include "test_mne_inspect_multimodal.moc"
