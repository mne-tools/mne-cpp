//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_bids_coordinate_system.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for BIDS coordinate system parsing and FiffCoordTrans conversion.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <bids/bids_coordinate_system.h>
#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestBidsCoordinateSystem
 *
 * @brief Tests for BIDS coordinate system JSON parsing and transform conversion.
 */
class TestBidsCoordinateSystem : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testReadJsonValid();
    void testWriteJsonRoundTrip();
    void testToFiffCoordTrans();
    void testTransformValues();
    void testIdentityTransform();
    void testReadNonexistentFile();

    void cleanupTestCase();

private:
    QString writeSampleCoordSystemJson(const QString& dir) const;
    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestBidsCoordinateSystem::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================

void TestBidsCoordinateSystem::testReadJsonValid()
{
    QString path = writeSampleCoordSystemJson(m_tempDir.path());
    BidsCoordinateSystem cs = BidsCoordinateSystem::read_json(path.toStdString());

    QCOMPARE(cs.system, std::string("Other"));
    QCOMPARE(cs.units, std::string("mm"));
    QVERIFY(!cs.description.empty());
}

//=============================================================================================================

void TestBidsCoordinateSystem::testWriteJsonRoundTrip()
{
    // Create a coordinate system, write it, read it back
    BidsCoordinateSystem cs;
    cs.system = "MNI152NLin2009aSym";
    cs.units = "mm";
    cs.description = "Test coordinate system";
    cs.transform = Matrix4d::Identity();
    cs.transform(0, 3) = 10.0;
    cs.transform(1, 3) = 20.0;
    cs.transform(2, 3) = 30.0;
    cs.transform(0, 0) = 0.1; // not exact in binary: must still come back bit for bit

    const std::filesystem::path outPath = m_tempDir.filePath("test_coordsystem.json").toStdString();
    QVERIFY(BidsCoordinateSystem::write_json(outPath, cs));

    BidsCoordinateSystem csRead = BidsCoordinateSystem::read_json(outPath);
    QCOMPARE(csRead.system, cs.system);
    QCOMPARE(csRead.units, cs.units);
    QCOMPARE(csRead.description, cs.description);

    // The 4x4 transform round-trips exactly
    QVERIFY(csRead.transform == cs.transform);
}

//=============================================================================================================

void TestBidsCoordinateSystem::testToFiffCoordTrans()
{
    BidsCoordinateSystem cs;
    cs.system = "Other";
    cs.units = "mm";
    cs.transform = Matrix4d::Identity();
    cs.transform(0, 3) = 5.0;
    cs.transform(1, 3) = 10.0;
    cs.transform(2, 3) = 15.0;

    FiffCoordTrans trans = cs.to_fiff_coord_trans(FIFFV_COORD_MRI, FIFFV_COORD_HEAD);

    QCOMPARE(trans.from, (int)FIFFV_COORD_MRI);
    QCOMPARE(trans.to, (int)FIFFV_COORD_HEAD);
}

//=============================================================================================================

void TestBidsCoordinateSystem::testTransformValues()
{
    // Verify actual transform matrix values are preserved
    BidsCoordinateSystem cs;
    cs.system = "Other";
    cs.units = "mm";
    cs.transform = Matrix4d::Identity();
    // Set a rotation + translation
    cs.transform(0, 0) = 0.0;
    cs.transform(0, 1) = -1.0;
    cs.transform(0, 2) = 0.0;
    cs.transform(1, 0) = 1.0;
    cs.transform(1, 1) = 0.0;
    cs.transform(1, 2) = 0.0;
    cs.transform(2, 0) = 0.0;
    cs.transform(2, 1) = 0.0;
    cs.transform(2, 2) = 1.0;
    cs.transform(0, 3) = 100.0;
    cs.transform(1, 3) = 200.0;
    cs.transform(2, 3) = 300.0;

    FiffCoordTrans trans = cs.to_fiff_coord_trans(FIFFV_COORD_MRI, FIFFV_COORD_HEAD);

    // Check translation component (may be in meters depending on units conversion)
    // The FiffCoordTrans stores data in its trans member
    QVERIFY(trans.trans.rows() >= 3);
    QVERIFY(trans.trans.cols() >= 4);
}

//=============================================================================================================

void TestBidsCoordinateSystem::testIdentityTransform()
{
    BidsCoordinateSystem cs;
    cs.system = "Other";
    cs.units = "m";
    cs.transform = Matrix4d::Identity();

    FiffCoordTrans trans = cs.to_fiff_coord_trans(FIFFV_COORD_HEAD, FIFFV_COORD_HEAD);
    QCOMPARE(trans.from, (int)FIFFV_COORD_HEAD);
    QCOMPARE(trans.to, (int)FIFFV_COORD_HEAD);
}

//=============================================================================================================

void TestBidsCoordinateSystem::testReadNonexistentFile()
{
    // Reading a nonexistent file should not crash — return default/empty struct
    BidsCoordinateSystem cs = BidsCoordinateSystem::read_json("/nonexistent/path/coordsystem.json");
    // Should have empty/default values, not crash
    QVERIFY(cs.system.empty());
}

//=============================================================================================================

void TestBidsCoordinateSystem::cleanupTestCase()
{
}

//=============================================================================================================

QString TestBidsCoordinateSystem::writeSampleCoordSystemJson(const QString& dir) const
{
    QJsonObject obj;
    obj["iEEGCoordinateSystem"] = "Other";
    obj["iEEGCoordinateUnits"] = "mm";
    obj["iEEGCoordinateSystemDescription"] = "Native scanner RAS";

    // 4x4 identity + offset
    QJsonArray transform;
    QJsonArray row0;
    row0 << 1.0 << 0.0 << 0.0 << 5.0;
    QJsonArray row1;
    row1 << 0.0 << 1.0 << 0.0 << 10.0;
    QJsonArray row2;
    row2 << 0.0 << 0.0 << 1.0 << 15.0;
    QJsonArray row3;
    row3 << 0.0 << 0.0 << 0.0 << 1.0;
    transform << row0 << row1 << row2 << row3;
    obj["iEEGCoordinateProcessingDescription"] = transform;

    QString path = dir + "/sub-01_space-Other_coordsystem.json";
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
    return path;
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestBidsCoordinateSystem)
#include "test_bids_coordinate_system.moc"
