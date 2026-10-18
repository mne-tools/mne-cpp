//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *

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
#include <QTemporaryDir>
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
    void bidsLoadGroupsLeadsByName();
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
    QVector<ElectrodeArray> arrays{arr};
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

void TestElectrodesPlugin::bidsLoadGroupsLeadsByName()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString tsv = dir.filePath(QStringLiteral("sub-01_ses-01_electrodes.tsv"));
    {
        QFile file(tsv);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "name\tx\ty\tz\tsize\ttype\n";
        // Numbers order a lead; A' (left, French convention) never merges with A; G declares its type
        out << "LA10\t0\t0\t31.5\tn/a\tn/a\n";
        out << "LA1\t0\t0\t0\tn/a\tn/a\n";
        out << "LA2\t0\t0\t3.5\tn/a\tn/a\n";
        out << "A'1\t-30\t0\t0\tn/a\tn/a\n";
        out << "A'2\t-30\t0\t10\tn/a\tn/a\n";
        out << "A1\t30\t0\t0\tn/a\tn/a\n";
        out << "G1\t0\t40\t0\tn/a\tgrid\n";
        out << "NOPOS\tn/a\tn/a\tn/a\tn/a\tn/a\n";
    }

    // Units are unknown without a coordsystem sidecar, so the file is refused
    ElectrodesPlugin plugin;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("no \\*_coordsystem.json")));
    QVERIFY(!plugin.loadBids(tsv));

    {
        QFile json(dir.filePath(QStringLiteral("sub-01_ses-01_coordsystem.json")));
        QVERIFY(json.open(QIODevice::WriteOnly | QIODevice::Text));
        json.write(R"({"iEEGCoordinateSystem": "ACPC", "iEEGCoordinateUnits": "mm"})");
    }
    QVERIFY(plugin.loadBids(tsv));
    QCOMPARE(plugin.source(), ElectrodeSource::Bids);
    QCOMPARE(plugin.contactCount(), 7);

    const auto& arrays = plugin.electrodeObject()->arrays();
    QCOMPARE(arrays.size(), 4);
    QCOMPARE(arrays[0].label, QStringLiteral("LA"));
    QCOMPARE(arrays[0].contacts[1].name, QStringLiteral("LA2"));
    QCOMPARE(arrays[0].contacts[2].name, QStringLiteral("LA10"));
    QCOMPARE(arrays[0].contacts[2].position, QVector3D(0.0f, 0.0f, 0.0315f));
    QCOMPARE(arrays[0].layout, ElectrodeLayout::Depth); // contacts 3.5 mm apart
    QCOMPARE(arrays[1].label, QStringLiteral("A'"));
    QCOMPARE(arrays[1].layout, ElectrodeLayout::Strip); // 10 mm apart
    QCOMPARE(arrays[2].label, QStringLiteral("A"));
    QCOMPARE(arrays[3].layout, ElectrodeLayout::Grid);
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
