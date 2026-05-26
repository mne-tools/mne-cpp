//=============================================================================================================
/**
 * @file     test_electrodes_plugin.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Unit tests for ELECTRODESPLUGIN::ElectrodesPlugin.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <electrodes.h>

#include <disp3D/renderable/electrodeobject.h>
#include <disp3D/scene/multimodalscene.h>
#include <disp3D/scene/pickresult.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>
#include <QTextStream>

using namespace ELECTRODESPLUGIN;
using namespace DISP3DLIB;

//=============================================================================================================

class TestElectrodesPlugin : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void emptyOnConstruction();
    void setArraysReplacesGeometry();
    void csvLoadGroupsByArrayLabel();
    void attachScenePublishesLayer();
    void handlePickIgnoresOtherKinds();
    void handlePickIgnoresOtherSourceIds();
    void handlePickSelectsContact();
};

//=============================================================================================================

void TestElectrodesPlugin::initTestCase()
{
    // No-op; constructor used per test.
}

//=============================================================================================================

void TestElectrodesPlugin::emptyOnConstruction()
{
    ElectrodesPlugin plugin;
    QCOMPARE(plugin.arrayCount(), 0);
    QCOMPARE(plugin.contactCount(), 0);
    QVERIFY(plugin.electrodeObject() != nullptr);
    QCOMPARE(plugin.sceneLayerId(), QStringLiteral("electrodes"));
    QCOMPARE(plugin.source(), ElectrodeSource::None);
    QVERIFY(plugin.sourcePath().isEmpty());
    QVERIFY(plugin.selectedContact().isEmpty());
}

//=============================================================================================================

void TestElectrodesPlugin::setArraysReplacesGeometry()
{
    ElectrodesPlugin plugin;
    QSignalSpy spy(&plugin, &ElectrodesPlugin::electrodesChanged);

    ElectrodeArray arr;
    arr.label = QStringLiteral("LA");
    arr.layout = ElectrodeLayout::Depth;
    for (int i = 0; i < 4; ++i) {
        ElectrodeContact c;
        c.name = QStringLiteral("LA%1").arg(i);
        c.position = QVector3D(0.0f, 0.0f, static_cast<float>(i) * 0.005f);
        arr.contacts.append(c);
    }
    QVector<ElectrodeArray> arrays {arr};
    plugin.setArrays(arrays);

    QCOMPARE(plugin.arrayCount(), 1);
    QCOMPARE(plugin.contactCount(), 4);
    QCOMPARE(spy.count(), 1);
}

//=============================================================================================================

void TestElectrodesPlugin::csvLoadGroupsByArrayLabel()
{
    QTemporaryFile file(QStringLiteral("XXXXXX_electrodes.csv"));
    QVERIFY(file.open());
    {
        QTextStream out(&file);
        out << "# label,x,y,z,array,layout\n";
        out << "LA0,0,0,0,LA,depth\n";
        out << "LA1,0,0,0.005,LA,depth\n";
        out << "G00,0.01,0,0,GRID,grid\n";
        out << "G01,0.02,0,0,GRID,grid\n";
        out << "G02,0.03,0,0,GRID,grid\n";
    }
    file.close();

    ElectrodesPlugin plugin;
    QVERIFY(plugin.loadCsv(file.fileName()));
    QCOMPARE(plugin.arrayCount(), 2);
    QCOMPARE(plugin.contactCount(), 5);
    QCOMPARE(plugin.source(), ElectrodeSource::Csv);
    QCOMPARE(plugin.sourcePath(), file.fileName());

    const auto& arrays = plugin.electrodeObject()->arrays();
    QCOMPARE(arrays.size(), 2);
    QCOMPARE(arrays[0].label, QStringLiteral("LA"));
    QCOMPARE(arrays[0].layout, ElectrodeLayout::Depth);
    QCOMPARE(arrays[1].label, QStringLiteral("GRID"));
    QCOMPARE(arrays[1].layout, ElectrodeLayout::Grid);
    QCOMPARE(arrays[1].gridRows, 1);
    QCOMPARE(arrays[1].gridCols, 3);
}

//=============================================================================================================

void TestElectrodesPlugin::attachScenePublishesLayer()
{
    ElectrodesPlugin plugin;

    ElectrodeArray arr;
    arr.label = QStringLiteral("LA");
    arr.layout = ElectrodeLayout::Depth;
    ElectrodeContact c;
    c.name = QStringLiteral("LA0");
    c.position = QVector3D(0.0f, 0.0f, 0.0f);
    arr.contacts.append(c);
    plugin.setArrays({arr});

    MultimodalScene scene;
    plugin.attachScene(&scene);

    const SceneLayer* layer = scene.findLayer(plugin.sceneLayerId());
    QVERIFY(layer != nullptr);
    QCOMPARE(layer->kind, SceneLayerKind::Electrode);
    QVERIFY(layer->payload != nullptr);
    QCOMPARE(layer->payload.get(),
             static_cast<void*>(plugin.electrodeObject()));

    plugin.attachScene(nullptr);
    QVERIFY(scene.findLayer(plugin.sceneLayerId()) == nullptr);
}

//=============================================================================================================

void TestElectrodesPlugin::handlePickIgnoresOtherKinds()
{
    ElectrodesPlugin plugin;
    QSignalSpy spy(&plugin, &ElectrodesPlugin::contactPicked);

    PickResult pick;
    pick.kind = PickKind::CorticalVertex;
    pick.label = QStringLiteral("LA0");
    pick.sourceId = plugin.sceneLayerId();
    plugin.handlePick(pick);

    QCOMPARE(spy.count(), 0);
    QVERIFY(plugin.selectedContact().isEmpty());
}

//=============================================================================================================

void TestElectrodesPlugin::handlePickIgnoresOtherSourceIds()
{
    ElectrodesPlugin plugin;
    QSignalSpy spy(&plugin, &ElectrodesPlugin::contactPicked);

    PickResult pick;
    pick.kind = PickKind::ElectrodeContact;
    pick.label = QStringLiteral("LA0");
    pick.sourceId = QStringLiteral("some_other_layer");
    plugin.handlePick(pick);

    QCOMPARE(spy.count(), 0);
    QVERIFY(plugin.selectedContact().isEmpty());
}

//=============================================================================================================

void TestElectrodesPlugin::handlePickSelectsContact()
{
    ElectrodesPlugin plugin;
    ElectrodeArray arr;
    arr.label = QStringLiteral("LA");
    arr.layout = ElectrodeLayout::Depth;
    ElectrodeContact c;
    c.name = QStringLiteral("LA0");
    c.position = QVector3D(0.0f, 0.0f, 0.0f);
    arr.contacts.append(c);
    plugin.setArrays({arr});

    QSignalSpy spy(&plugin, &ElectrodesPlugin::contactPicked);

    PickResult pick;
    pick.kind = PickKind::ElectrodeContact;
    pick.label = QStringLiteral("LA0");
    pick.sourceId = plugin.sceneLayerId();
    plugin.handlePick(pick);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("LA0"));
    QCOMPARE(plugin.selectedContact(), QStringLiteral("LA0"));
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestElectrodesPlugin)
#include "test_electrodes_plugin.moc"
