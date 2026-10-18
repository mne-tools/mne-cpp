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
#include <ieeg/ieeg_implantation.h>
#include <mri/mri_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDataStream>
#include <QObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include <QTextStream>

using namespace ELECTRODESPLUGIN;
using namespace DISP3DLIB;
using namespace IEEGLIB;

namespace
{
IeegImplantation oneLead(int nContacts)
{
    QVector<IeegContact> contacts(nContacts);
    for (int i = 0; i < nContacts; ++i) {
        contacts[i].name = QStringLiteral("LA%1").arg(i);
        contacts[i].position = Eigen::Vector3f(0.0f, 0.0f, float(i) * 0.005f);
    }
    return IeegImplantation(contacts);
}

// Minimal 2x2x2 uchar MGH volume, 1 mm voxels, centred at cRas (mm)
bool writeMgh(const QString& path, const Eigen::Vector3f& cRas)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream << qint32(MRILIB::MRI_MGH_VERSION) << qint32(2) << qint32(2) << qint32(2) << qint32(1) << qint32(MRILIB::MRI_UCHAR) << qint32(0);
    stream << qint16(1) << 1.0f << 1.0f << 1.0f;
    stream << -1.0f << 0.0f << 0.0f << 0.0f << 0.0f << -1.0f << 0.0f << 1.0f << 0.0f;
    stream << cRas.x() << cRas.y() << cRas.z();
    data.append(QByteArray(MRILIB::MRI_MGH_DATA_OFFSET - data.size() + 8, '\0'));
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}
} // namespace

//=============================================================================================================

class TestElectrodesPlugin : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void emptyOnConstruction();
    void setImplantationReplacesGeometry();
    void csvLoadGroupsByArrayLabel();
    void bidsLoadGroupsLeadsByName();
    void loadValuesMatchesContacts();
    void bidsLoadConvertsScannerRas();
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
    QCOMPARE(plugin.leadCount(), 0);
    QCOMPARE(plugin.contactCount(), 0);
    QVERIFY(plugin.electrodeObject() != nullptr);
    QCOMPARE(plugin.sceneLayerId(), QStringLiteral("electrodes"));
    QCOMPARE(plugin.source(), ElectrodeSource::None);
    QVERIFY(plugin.sourcePath().isEmpty());
    QVERIFY(plugin.selectedContact().isEmpty());
}

//=============================================================================================================

void TestElectrodesPlugin::setImplantationReplacesGeometry()
{
    ElectrodesPlugin plugin;
    QSignalSpy spy(&plugin, &ElectrodesPlugin::electrodesChanged);
    plugin.setImplantation(oneLead(4));

    QCOMPARE(plugin.leadCount(), 1);
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
        out << "# label,x,y,z,lead,type\n";
        out << "LA0,0,0,0,LA,depth\n";
        out << "LA1,0,0,0.005,LA,depth\n";
        out << "G00,0.01,0,0,GRID,grid\n";
        out << "G01,0.02,0,0,GRID,grid\n";
        out << "G02,0.03,0,0,GRID,grid\n";
    }
    file.close();

    ElectrodesPlugin plugin;
    QVERIFY(plugin.loadCsv(file.fileName()));
    QCOMPARE(plugin.leadCount(), 2);
    QCOMPARE(plugin.contactCount(), 5);
    QCOMPARE(plugin.source(), ElectrodeSource::Csv);
    QCOMPARE(plugin.sourcePath(), file.fileName());

    const QVector<IeegLead>& leads = plugin.electrodeObject()->implantation().leads();
    QCOMPARE(leads.size(), 2);
    QCOMPARE(leads[0].name, QStringLiteral("LA"));
    QCOMPARE(leads[0].type, IeegLeadType::Depth);
    QCOMPARE(leads[1].name, QStringLiteral("GRID"));
    QCOMPARE(leads[1].type, IeegLeadType::Grid);
    QCOMPARE(leads[1].contacts.size(), 3);
}

//=============================================================================================================

void TestElectrodesPlugin::loadValuesMatchesContacts()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto write = [&dir](const QString& name, const char* text) {
        QFile file(dir.filePath(name));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
            file.write(text);
        return file.fileName();
    };

    ElectrodesPlugin plugin;
    QVERIFY(!plugin.loadValues(write(QStringLiteral("early.tsv"), "LA0\t1\n")));
    plugin.setImplantation(oneLead(4));

    QSignalSpy spy(&plugin, &ElectrodesPlugin::valuesChanged);
    QVERIFY(plugin.loadValues(write(QStringLiteral("values.tsv"), "name\tvalue\n# comment\nLA0\t0.5\nLA2\t-1.5\nXX9\t3\n")));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(plugin.values().size(), 2);
    QCOMPARE(plugin.values().value(QStringLiteral("LA0")), 0.5f);
    QCOMPARE(plugin.values().value(QStringLiteral("LA2")), -1.5f);

    QVERIFY(plugin.loadValues(write(QStringLiteral("values.csv"), "LA1,2\n")));
    QCOMPARE(plugin.values().keys(), QStringList{QStringLiteral("LA1")});

    // A failed load keeps the shown values and says why
    QVERIFY(!plugin.loadValues(write(QStringLiteral("bad.tsv"), "name\tvalue\nLA0\tlots\n")));
    QVERIFY(plugin.lastError().contains(QStringLiteral("Line 2")));
    QVERIFY(!plugin.loadValues(write(QStringLiteral("other.tsv"), "ZZ1\t1\n")));
    QVERIFY(!plugin.loadValues(dir.filePath(QStringLiteral("missing.tsv"))));
    QCOMPARE(plugin.values().keys(), QStringList{QStringLiteral("LA1")});
    QCOMPARE(spy.count(), 2);

    plugin.setImplantation(oneLead(2));
    QVERIFY(plugin.values().isEmpty());
    QCOMPARE(spy.count(), 3);
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
        json.write(R"({"iEEGCoordinateSystem": "Other", "iEEGCoordinateUnits": "mm"})");
    }
    QVERIFY(plugin.loadBids(tsv));
    QCOMPARE(plugin.source(), ElectrodeSource::Bids);
    QCOMPARE(plugin.contactCount(), 7);

    const IeegImplantation& implantation = plugin.electrodeObject()->implantation();
    const QVector<IeegLead>& leads = implantation.leads();
    QCOMPARE(leads.size(), 4);
    QCOMPARE(leads[0].name, QStringLiteral("LA"));
    QCOMPARE(implantation.contacts()[leads[0].contacts[1]].name, QStringLiteral("LA2"));
    const IeegContact& la10 = implantation.contacts()[leads[0].contacts[2]];
    QCOMPARE(la10.name, QStringLiteral("LA10"));
    QVERIFY(la10.position.isApprox(Eigen::Vector3f(0.0f, 0.0f, 0.0315f)));
    QCOMPARE(leads[0].type, IeegLeadType::Depth); // contacts 3.5 mm apart
    QCOMPARE(leads[1].name, QStringLiteral("A'"));
    QCOMPARE(leads[1].type, IeegLeadType::Strip); // 10 mm apart
    QCOMPARE(leads[2].name, QStringLiteral("A"));
    QCOMPARE(leads[3].type, IeegLeadType::Grid);
}

//=============================================================================================================

void TestElectrodesPlugin::bidsLoadConvertsScannerRas()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString tsv = dir.filePath(QStringLiteral("sub-01_electrodes.tsv"));
    {
        QFile file(tsv);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("name\tx\ty\tz\tsize\nLA1\t10\t20\t30\tn/a\nLA2\t10\t20\t33.5\tn/a\n");
        QFile json(dir.filePath(QStringLiteral("sub-01_coordsystem.json")));
        QVERIFY(json.open(QIODevice::WriteOnly | QIODevice::Text));
        json.write(R"({"iEEGCoordinateSystem": "ACPC", "iEEGCoordinateUnits": "mm"})");
    }

    // Scanner RAS needs the T1 to reach the surfaces
    ElectrodesPlugin plugin;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("T1.mgz is needed")));
    QVERIFY(!plugin.loadBids(tsv));
    QVERIFY(plugin.lastError().contains(QStringLiteral("ACPC")));

    // Surface RAS drops the T1's centre (c_ras)
    const QString t1 = dir.filePath(QStringLiteral("T1.mgh"));
    QVERIFY(writeMgh(t1, Eigen::Vector3f(10.0f, 20.0f, 30.0f)));
    QVERIFY(plugin.loadBids(tsv, t1));
    QVERIFY(plugin.lastError().isEmpty());
    const IeegImplantation& implantation = plugin.electrodeObject()->implantation();
    QCOMPARE(implantation.coordinateSystem(), QStringLiteral("surface RAS"));
    QVERIFY(implantation.contacts()[0].position.isZero(1e-6f));
    QVERIFY(implantation.contacts()[1].position.isApprox(Eigen::Vector3f(0.0f, 0.0f, 0.0035f)));
}

//=============================================================================================================

void TestElectrodesPlugin::attachScenePublishesLayer()
{
    ElectrodesPlugin plugin;

    plugin.setImplantation(oneLead(1));

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
    plugin.setImplantation(oneLead(1));

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
