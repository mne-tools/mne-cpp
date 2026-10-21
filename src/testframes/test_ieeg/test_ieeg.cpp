//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_ieeg.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Tests for IEEGLIB: lead grouping, BIDS iEEG reading, bipolar montages and surface mapping.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <ieeg/ieeg_implantation.h>
#include <ieeg/ieeg_reference.h>
#include <ieeg/ieeg_surface_map.h>
#include <dsp/eeg_reference.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>

#include <limits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace IEEGLIB;
using namespace FIFFLIB;

namespace
{

IeegContact contact(const QString& name, float xMm, float yMm, float zMm, const QString& lead = QString())
{
    IeegContact c;
    c.name = name;
    c.position = Eigen::Vector3f(xMm, yMm, zMm) * 0.001f;
    c.lead = lead;
    return c;
}

void writeFile(const QString& path, const QByteArray& content)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(content);
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestIeeg
 */
class TestIeeg : public QObject
{
    Q_OBJECT

private slots:
    void groupsContactsIntoLeads();
    void readsBidsElectrodes();
    void bipolarPairsFollowLeads();
    void surfaceMapMatchesStcNearSensors();
    void readsBidsUnitsAndDeclarations();
    void handlesDegenerateInput();
};

//=============================================================================================================

void TestIeeg::readsBidsUnitsAndDeclarations()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString tsv = dir.filePath(QStringLiteral("sub-01_ses-02_electrodes.tsv"));
    // seeg type on a grouped lead, grid by type without a shape, depth declared on a contact without group
    writeFile(tsv,
              "name\tx\ty\tz\tsize\ttype\tgroup\tdimension\n"
              "B1\t0\t0\t0\tn/a\tseeg\tB\tn/a\n"
              "B2\t0\t0\t1\tn/a\tn/a\tB\tn/a\n"
              "H1\t5\t0\t0\tn/a\tGRID\tH\t3x1\n"
              "H2\t6\t0\t0\tn/a\tn/a\tH\tn/a\n"
              "D1\t-5\t0\t0\tn/a\tdepth\tn/a\tn/a\n"
              "D2\t-5\t0\t2\tn/a\tn/a\tn/a\tn/a\n");
    // The sidecar sharing the longer prefix (ses-02) wins over another session's
    writeFile(dir.filePath(QStringLiteral("sub-01_ses-01_coordsystem.json")),
              R"({"iEEGCoordinateSystem": "Other", "iEEGCoordinateUnits": "mm"})");
    writeFile(dir.filePath(QStringLiteral("sub-01_ses-02_coordsystem.json")),
              R"({"iEEGCoordinateSystem": "Other", "iEEGCoordinateUnits": "cm"})");
    QString error;
    IeegImplantation implantation = IeegImplantation::readBids(tsv, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(!implantation.isScannerRas());
    QVERIFY(implantation.contacts()[1].position.isApprox(Eigen::Vector3f(0.0f, 0.0f, 0.01f))); // cm -> m
    const QVector<IeegLead>& leads = implantation.leads();
    QCOMPARE(leads.size(), 3);
    QCOMPARE(leads[0].type, IeegLeadType::Depth); // seeg, although 10 mm apart
    QCOMPARE(leads[1].type, IeegLeadType::Grid);  // declared by type; 3x1 does not fit two contacts
    QCOMPARE(leads[1].rows, 0);
    QCOMPARE(leads[2].name, QStringLiteral("D"));
    QCOMPARE(leads[2].type, IeegLeadType::Depth);

    writeFile(dir.filePath(QStringLiteral("sub-01_ses-02_coordsystem.json")),
              R"({"iEEGCoordinateSystem": "ScanRAS", "iEEGCoordinateUnits": "m"})");
    implantation = IeegImplantation::readBids(tsv);
    QVERIFY(implantation.isScannerRas());
    QVERIFY(implantation.contacts()[1].position.isApprox(Eigen::Vector3f(0.0f, 0.0f, 1.0f)));

    // Only contacts without coordinates: nothing to place
    writeFile(tsv, "name\tx\ty\tz\nA1\tn/a\tn/a\tn/a\nA2\t1\tn/a\t0\n");
    QVERIFY(IeegImplantation::readBids(tsv, &error).isEmpty());
    QVERIFY2(error.contains(QLatin1String("no contacts")), qPrintable(error));
}

//=============================================================================================================

void TestIeeg::handlesDegenerateInput()
{
    IeegImplantation implantation({contact("A1", 0, 0, 0), contact("A2", 0, 0, 3), contact("A3", 0, 0, 6), contact("A4", 0, 0, 9)});
    QVERIFY(!implantation.setLeadType(QStringLiteral("nope"), IeegLeadType::Grid));
    QVERIFY(implantation.setLeadType(QStringLiteral("A"), IeegLeadType::Grid, 2, 2));
    QCOMPARE(implantation.leads()[0].rows, 2);
    // A shape that does not fit the contacts is dropped, so the lead is paired as a single row
    QVERIFY(implantation.setLeadType(QStringLiteral("A"), IeegLeadType::Grid, 3, 2));
    QCOMPARE(implantation.leads()[0].cols, 0);
    // 4 x 1073741825 wraps to 4 in 32-bit int arithmetic; it must not pass as a 4-contact grid
    QVERIFY(implantation.setLeadType(QStringLiteral("A"), IeegLeadType::Grid, 4, 1073741825));
    QCOMPARE(implantation.leads()[0].cols, 0);
    QCOMPARE(IeegReference::bipolarPairs(implantation).size(), 3);
    QCOMPARE(IeegReference::bipolarPairs(IeegImplantation()).size(), 0);
    QCOMPARE(IeegReference::bipolarMatrix({}, 4).rows(), Eigen::Index(0));

    Eigen::MatrixX3f vertices = Eigen::MatrixX3f::Zero(2, 3);
    vertices(1, 0) = 0.001f;
    const Eigen::MatrixX3f contacts = Eigen::MatrixX3f::Zero(1, 3);
    QCOMPARE(IeegSurfaceMap::weights(contacts, Eigen::MatrixX3f(0, 3), 0.005, IeegSurfaceMapMode::Sum, false).nonZeros(), Eigen::Index(0));
    QCOMPARE(IeegSurfaceMap::weights(Eigen::MatrixX3f(0, 3), vertices, 0.005, IeegSurfaceMapMode::Sum, false).nonZeros(), Eigen::Index(0));
    QCOMPARE(IeegSurfaceMap::weights(contacts, vertices, 0.0, IeegSurfaceMapMode::Sum, false).nonZeros(), Eigen::Index(0));
    // A vertex exactly at the reach gets weight 0 and is left out; normalising cannot divide by zero
    const Eigen::MatrixXd w = IeegSurfaceMap::weights(contacts, vertices, 0.001, IeegSurfaceMapMode::Weighted, false);
    QCOMPARE(w(0, 0), 1.0);
    QCOMPARE(w(1, 0), 0.0);
}

//=============================================================================================================

void TestIeeg::groupsContactsIntoLeads()
{
    // File order is not lead order; numbers order a lead, A' never merges with A
    const IeegImplantation implantation({contact("LA10", 0, 0, 31.5),
                                         contact("LA1", 0, 0, 0),
                                         contact("A'1", -30, 0, 0),
                                         contact("LA2", 0, 0, 3.5),
                                         contact("A'2", -30, 0, 10),
                                         contact("A1", 30, 0, 0),
                                         contact("REF", 0, 50, 0)},
                                        QStringLiteral("ACPC"));
    QCOMPARE(implantation.coordinateSystem(), QStringLiteral("ACPC"));
    const QVector<IeegLead>& leads = implantation.leads();
    QCOMPARE(leads.size(), 4);
    QCOMPARE(leads[0].name, QStringLiteral("LA"));
    QCOMPARE(leads[0].contacts, QVector<int>({1, 3, 0}));
    QCOMPARE(leads[0].type, IeegLeadType::Depth); // 3.5 mm apart
    QCOMPARE(leads[1].name, QStringLiteral("A'"));
    QCOMPARE(leads[1].type, IeegLeadType::Strip); // 10 mm apart
    QCOMPARE(leads[2].name, QStringLiteral("A"));
    QCOMPARE(leads[3].name, QStringLiteral("REF")); // no number: the name is the lead
    QCOMPARE(implantation.contacts()[0].lead, QStringLiteral("LA"));
    QCOMPARE(implantation.indexOf(QStringLiteral("A1")), 5);
    QCOMPARE(implantation.indexOf(QStringLiteral("nope")), -1);
    QCOMPARE(implantation.positions().row(0), Eigen::RowVector3f(0.0f, 0.0f, 0.0315f));
}

//=============================================================================================================

void TestIeeg::readsBidsElectrodes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString tsv = dir.filePath(QStringLiteral("sub-01_ses-01_electrodes.tsv"));
    // A grid declared by group and dimension, a strip declared by type, a depth lead named only
    writeFile(tsv,
              "name\tx\ty\tz\tsize\ttype\tgroup\tdimension\n"
              "G1\t0\t0\t0\tn/a\tn/a\tGridA\t[2x2]\n"
              "G2\t10\t0\t0\tn/a\tn/a\tGridA\t[2x2]\n"
              "G3\t0\t10\t0\tn/a\tn/a\tGridA\t[2x2]\n"
              "G4\t10\t10\t0\tn/a\tn/a\tGridA\t[2x2]\n"
              "S1\t50\t0\t0\tn/a\tstrip\tn/a\tn/a\n"
              "S2\t53\t0\t0\tn/a\tstrip\tn/a\tn/a\n"
              "LA1\t-30\t0\t0\tn/a\tn/a\tn/a\tn/a\n"
              "LA2\t-30\t0\t3.5\tn/a\tn/a\tn/a\tn/a\n"
              "NOPOS\tn/a\tn/a\tn/a\tn/a\tn/a\tn/a\tn/a\n");

    // Units are unknown without a coordsystem sidecar, so the file is refused
    QString error;
    QVERIFY(IeegImplantation::readBids(tsv, &error).isEmpty());
    QVERIFY2(error.contains(QLatin1String("_coordsystem.json")), qPrintable(error));

    writeFile(dir.filePath(QStringLiteral("sub-01_ses-01_coordsystem.json")),
              R"({"iEEGCoordinateSystem": "ACPC", "iEEGCoordinateUnits": "furlong"})");
    QVERIFY(IeegImplantation::readBids(tsv, &error).isEmpty());
    QVERIFY2(error.contains(QLatin1String("furlong")), qPrintable(error));

    writeFile(dir.filePath(QStringLiteral("sub-01_ses-01_coordsystem.json")),
              R"({"iEEGCoordinateSystem": "ACPC", "iEEGCoordinateUnits": "mm"})");
    const IeegImplantation implantation = IeegImplantation::readBids(tsv, &error);
    QCOMPARE(implantation.contacts().size(), 8);
    QCOMPARE(implantation.coordinateSystem(), QStringLiteral("ACPC"));
    QVERIFY(implantation.contacts()[1].position.isApprox(Eigen::Vector3f(0.01f, 0.0f, 0.0f))); // mm -> m

    // ACPC is scanner RAS of the T1 (as MNE-BIDS reads it): moving it into surface RAS keeps the leads
    QVERIFY(implantation.isScannerRas());
    Eigen::Matrix4f scannerToSurface = Eigen::Matrix4f::Identity();
    scannerToSurface.block<3, 1>(0, 3) = Eigen::Vector3f(-0.005f, 0.02f, 0.0f);
    const IeegImplantation surfaceRas = implantation.transformed(scannerToSurface, QStringLiteral("surface RAS"));
    QVERIFY(!surfaceRas.isScannerRas());
    QVERIFY(surfaceRas.contacts()[1].position.isApprox(Eigen::Vector3f(0.005f, 0.02f, 0.0f)));
    QCOMPARE(surfaceRas.leads().size(), implantation.leads().size());
    const QVector<IeegLead>& leads = implantation.leads();
    QCOMPARE(leads.size(), 3);
    QCOMPARE(leads[0].name, QStringLiteral("GridA"));
    QCOMPARE(leads[0].type, IeegLeadType::Grid);
    QCOMPARE(leads[0].rows, 2);
    QCOMPARE(leads[0].cols, 2);
    QCOMPARE(leads[1].type, IeegLeadType::Strip); // declared, although 3 mm apart
    QCOMPARE(leads[2].name, QStringLiteral("LA"));
    QCOMPARE(leads[2].type, IeegLeadType::Depth);

    // A grid of known shape is paired along its rows, then down its columns
    QStringList names;
    for (const IeegBipolarPair& p : IeegReference::bipolarPairs(implantation))
        names << p.name;
    QCOMPARE(names, QStringList({"G1-G2", "G3-G4", "G1-G3", "G2-G4", "S1-S2", "LA1-LA2"}));
}

//=============================================================================================================

void TestIeeg::bipolarPairsFollowLeads()
{
    // A depth lead whose last contact is too far for a neighbour, and six grid contacts of unknown shape
    QVector<IeegContact> contacts = {contact("LA1", 0, 0, 0), contact("LA2", 0, 0, 3.5), contact("LA3", 0, 0, 7),
                                     contact("LA4", 0, 0, 40)};
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 3; ++c)
            contacts.append(contact(QStringLiteral("G%1").arg(r * 3 + c + 1), 50.0f + 10.0f * float(c), 10.0f * float(r), 0, QStringLiteral("G")));
    }
    IeegImplantation implantation(contacts);
    QCOMPARE(implantation.leads()[1].type, IeegLeadType::Strip); // no declared shape: one row

    // Without a known shape the grid is paired as one row: G3-G4 spans 22 mm and is dropped
    QVector<IeegBipolarPair> pairs = IeegReference::bipolarPairs(implantation);
    QStringList names;
    for (const IeegBipolarPair& p : pairs)
        names << p.name;
    QCOMPARE(names, QStringList({"LA1-LA2", "LA2-LA3", "G1-G2", "G2-G3", "G4-G5", "G5-G6"}));
    QCOMPARE(pairs[0].position, Eigen::Vector3f(0.0f, 0.0f, 0.00175f));

    // The montage subtracts the cathode from the anode
    Eigen::MatrixXd data = Eigen::MatrixXd::Zero(contacts.size(), 2);
    data(0, 0) = 5.0;
    data(1, 0) = 2.0;
    data(1, 1) = 1.0;
    const Eigen::MatrixXd bipolar = IeegReference::bipolarMatrix(pairs, int(contacts.size())) * data;
    QCOMPARE(bipolar.rows(), Eigen::Index(6));
    QCOMPARE(bipolar(0, 0), 3.0);
    QCOMPARE(bipolar(0, 1), -1.0);
    QCOMPARE(bipolar(1, 0), 2.0);

    // It equals the sensor-space derivation on the same pairs, which appends the bipolar rows last
    FiffInfo info;
    QStringList anodes, cathodes;
    for (const IeegContact& c : contacts) {
        FiffChInfo ch;
        ch.ch_name = c.name;
        ch.kind = FIFFV_SEEG_CH;
        info.chs.append(ch);
        info.ch_names.append(c.name);
    }
    info.nchan = int(contacts.size());
    for (const IeegBipolarPair& p : pairs) {
        anodes << contacts[p.anode].name;
        cathodes << contacts[p.cathode].name;
    }
    Eigen::MatrixXd derived = Eigen::MatrixXd::Random(contacts.size(), 5);
    const Eigen::MatrixXd expected = IeegReference::bipolarMatrix(pairs, int(contacts.size())) * derived;
    UTILSLIB::setBipolarReference(derived, info, anodes, cathodes);
    QCOMPARE(derived.bottomRows(pairs.size()), expected);
    QCOMPARE(info.ch_names.mid(info.nchan - int(pairs.size())), names);
}

//=============================================================================================================

void TestIeeg::surfaceMapMatchesStcNearSensors()
{
    // Vertices every 2 mm along x; contacts at 0.8 mm and 8 mm; the reach is 5 mm
    Eigen::MatrixX3f vertices = Eigen::MatrixX3f::Zero(6, 3);
    for (int v = 0; v < 6; ++v)
        vertices(v, 0) = 0.002f * float(v);
    Eigen::MatrixX3f contacts(3, 3);
    contacts << 0.0008f, 0.0f, 0.0f, 0.008f, 0.0f, 0.0f, std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f;
    const double reach = 0.005;
    auto near = [](double a, double b) {
        return qAbs(a - b) < 1e-6;
    };

    // Sum, without projection: w = 1 - d / reach within reach, as MNE-Python
    Eigen::MatrixXd w = IeegSurfaceMap::weights(contacts, vertices, reach, IeegSurfaceMapMode::Sum, false);
    QCOMPARE(w.rows(), Eigen::Index(6));
    QCOMPARE(w.cols(), Eigen::Index(3));
    QVERIFY(near(w(0, 0), 0.84) && near(w(1, 0), 0.76) && near(w(2, 0), 0.36) && near(w(3, 0), 0.0));
    QVERIFY(near(w(2, 1), 0.2) && near(w(3, 1), 0.6) && near(w(4, 1), 1.0) && near(w(5, 1), 0.6));
    QCOMPARE(w.col(2).norm(), 0.0); // no position: empty column

    // Weighted: the weights of vertex 2 (0.36 and 0.2) are normalised to sum to 1
    w = IeegSurfaceMap::weights(contacts, vertices, reach, IeegSurfaceMapMode::Weighted, false);
    QVERIFY(near(w(2, 0), 0.36 / 0.56) && near(w(2, 1), 0.2 / 0.56) && near(w(4, 1), 1.0));

    // Single and nearest keep the nearest contact only
    w = IeegSurfaceMap::weights(contacts, vertices, reach, IeegSurfaceMapMode::Single, false);
    QVERIFY(near(w(2, 0), 0.36) && near(w(2, 1), 0.0));
    w = IeegSurfaceMap::weights(contacts, vertices, reach, IeegSurfaceMapMode::Nearest, false);
    QVERIFY(near(w(2, 0), 1.0) && near(w(2, 1), 0.0) && near(w(5, 1), 1.0));

    // Projection snaps contact 0 to vertex 0
    w = IeegSurfaceMap::weights(contacts, vertices, reach, IeegSurfaceMapMode::Sum, true);
    QVERIFY(near(w(0, 0), 1.0) && near(w(1, 0), 0.6) && near(w(2, 0), 0.2));
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestIeeg)
#include "test_ieeg.moc"
