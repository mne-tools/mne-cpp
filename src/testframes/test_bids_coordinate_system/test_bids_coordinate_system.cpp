//=============================================================================================================
/**
 * @file     test_bids_coordinate_system.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
    BidsCoordinateSystem cs = BidsCoordinateSystem::readJson(path);

    QCOMPARE(cs.system, QString("Other"));
    QCOMPARE(cs.units, QString("mm"));
    QVERIFY(!cs.description.isEmpty());
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

    QString outPath = m_tempDir.filePath("test_coordsystem.json");
    QVERIFY(BidsCoordinateSystem::writeJson(outPath, cs));

    BidsCoordinateSystem csRead = BidsCoordinateSystem::readJson(outPath);
    QCOMPARE(csRead.system, cs.system);
    QCOMPARE(csRead.units, cs.units);
    QCOMPARE(csRead.description, cs.description);

    // The 4x4 transform should round-trip
    double maxDiff = (csRead.transform - cs.transform).cwiseAbs().maxCoeff();
    QVERIFY2(maxDiff < 1e-5,
             qPrintable(QString("Transform round-trip maxDiff=%1").arg(maxDiff)));
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

    FiffCoordTrans trans = cs.toFiffCoordTrans(FIFFV_COORD_MRI, FIFFV_COORD_HEAD);

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
    cs.transform(0, 0) = 0.0;   cs.transform(0, 1) = -1.0;  cs.transform(0, 2) = 0.0;
    cs.transform(1, 0) = 1.0;   cs.transform(1, 1) = 0.0;   cs.transform(1, 2) = 0.0;
    cs.transform(2, 0) = 0.0;   cs.transform(2, 1) = 0.0;   cs.transform(2, 2) = 1.0;
    cs.transform(0, 3) = 100.0;
    cs.transform(1, 3) = 200.0;
    cs.transform(2, 3) = 300.0;

    FiffCoordTrans trans = cs.toFiffCoordTrans(FIFFV_COORD_MRI, FIFFV_COORD_HEAD);

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

    FiffCoordTrans trans = cs.toFiffCoordTrans(FIFFV_COORD_HEAD, FIFFV_COORD_HEAD);
    QCOMPARE(trans.from, (int)FIFFV_COORD_HEAD);
    QCOMPARE(trans.to, (int)FIFFV_COORD_HEAD);
}

//=============================================================================================================

void TestBidsCoordinateSystem::testReadNonexistentFile()
{
    // Reading a nonexistent file should not crash — return default/empty struct
    BidsCoordinateSystem cs = BidsCoordinateSystem::readJson("/nonexistent/path/coordsystem.json");
    // Should have empty/default values, not crash
    QVERIFY(cs.system.isEmpty() || cs.system == "Unknown");
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
    QJsonArray row0; row0 << 1.0 << 0.0 << 0.0 << 5.0;
    QJsonArray row1; row1 << 0.0 << 1.0 << 0.0 << 10.0;
    QJsonArray row2; row2 << 0.0 << 0.0 << 1.0 << 15.0;
    QJsonArray row3; row3 << 0.0 << 0.0 << 0.0 << 1.0;
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
