//=============================================================================================================
/**
 * @file     test_mri_slices_plugin.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Unit tests for MRISLICESPLUGIN::MriSlicesPlugin.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mri_slices.h>

#include <disp3D/scene/multimodalscene.h>
#include <disp3D/scene/pickresult.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSignalSpy>
#include <QTest>
#include <QVector3D>

using namespace MRISLICESPLUGIN;
using namespace DISP3DLIB;

//=============================================================================================================

class TestMriSlicesPlugin : public QObject
{
    Q_OBJECT

private slots:
    void emptyOnConstruction();
    void sceneLayerIdsAreStable();
    void loadVolumeRejectsUnsupportedFormat();
    void attachSceneWithoutVolumeDoesNotCrash();
    void setCrosshairEmitsSignal();
    void setCrosshairSuppressesDuplicate();
    void handlePickIgnoresOtherKinds();
    void handlePickMriVoxelMovesCrosshair();
};

//=============================================================================================================

void TestMriSlicesPlugin::emptyOnConstruction()
{
    MriSlicesPlugin plugin;
    QCOMPARE(plugin.volume(), nullptr);
    QVERIFY(plugin.sourcePath().isEmpty());
    QCOMPARE(plugin.crosshair(), Eigen::Vector3f::Zero());
    QCOMPARE(plugin.axialSlice(),    nullptr);
    QCOMPARE(plugin.coronalSlice(),  nullptr);
    QCOMPARE(plugin.sagittalSlice(), nullptr);
}

//=============================================================================================================

void TestMriSlicesPlugin::sceneLayerIdsAreStable()
{
    const auto ids = MriSlicesPlugin::sceneLayerIds();
    QCOMPARE(ids[0], QStringLiteral("mri_axial"));
    QCOMPARE(ids[1], QStringLiteral("mri_coronal"));
    QCOMPARE(ids[2], QStringLiteral("mri_sagittal"));
}

//=============================================================================================================

void TestMriSlicesPlugin::loadVolumeRejectsUnsupportedFormat()
{
    MriSlicesPlugin plugin;
    QVERIFY(!plugin.loadVolume(QStringLiteral("/no/such/file.nii.gz")));
    QVERIFY(!plugin.loadVolume(QStringLiteral("/no/such/file.txt")));
    QCOMPARE(plugin.volume(), nullptr);
}

//=============================================================================================================

void TestMriSlicesPlugin::attachSceneWithoutVolumeDoesNotCrash()
{
    MriSlicesPlugin plugin;
    MultimodalScene scene;
    plugin.attachScene(&scene);

    // No volume, so no layers should be published.
    for (const QString& id : MriSlicesPlugin::sceneLayerIds()) {
        QVERIFY(scene.findLayer(id) == nullptr);
    }

    plugin.attachScene(nullptr);
}

//=============================================================================================================

void TestMriSlicesPlugin::setCrosshairEmitsSignal()
{
    MriSlicesPlugin plugin;
    QSignalSpy spy(&plugin, &MriSlicesPlugin::crosshairChanged);

    plugin.setCrosshair(Eigen::Vector3f(1.0f, 2.0f, 3.0f));
    QCOMPARE(spy.count(), 1);
    const QVector3D got = spy.first().at(0).value<QVector3D>();
    QCOMPARE(got, QVector3D(1.0f, 2.0f, 3.0f));
    QCOMPARE(plugin.crosshair(), Eigen::Vector3f(1.0f, 2.0f, 3.0f));
}

//=============================================================================================================

void TestMriSlicesPlugin::setCrosshairSuppressesDuplicate()
{
    MriSlicesPlugin plugin;
    plugin.setCrosshair(Eigen::Vector3f(1.0f, 2.0f, 3.0f));

    QSignalSpy spy(&plugin, &MriSlicesPlugin::crosshairChanged);
    plugin.setCrosshair(Eigen::Vector3f(1.0f, 2.0f, 3.0f));
    QCOMPARE(spy.count(), 0);
}

//=============================================================================================================

void TestMriSlicesPlugin::handlePickIgnoresOtherKinds()
{
    MriSlicesPlugin plugin;
    QSignalSpy spy(&plugin, &MriSlicesPlugin::crosshairChanged);

    PickResult pick;
    pick.kind = PickKind::CorticalVertex;
    pick.world = QVector3D(5.0f, 5.0f, 5.0f);
    plugin.handlePick(pick);

    QCOMPARE(spy.count(), 0);
    QCOMPARE(plugin.crosshair(), Eigen::Vector3f::Zero());
}

//=============================================================================================================

void TestMriSlicesPlugin::handlePickMriVoxelMovesCrosshair()
{
    MriSlicesPlugin plugin;
    QSignalSpy spy(&plugin, &MriSlicesPlugin::crosshairChanged);

    PickResult pick;
    pick.kind = PickKind::MriVoxel;
    pick.world = QVector3D(10.0f, 20.0f, 30.0f);
    plugin.handlePick(pick);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(plugin.crosshair(), Eigen::Vector3f(10.0f, 20.0f, 30.0f));
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMriSlicesPlugin)
#include "test_mri_slices_plugin.moc"
