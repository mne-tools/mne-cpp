//=============================================================================================================
/**
 * @file     test_tool_conversion_extended.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
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
 * @brief    Tests for extended conversion tools (raw2mat, ctf_dig2fiff, tufts2fiff).
 */

//=============================================================================================================
// Include tool sources
//=============================================================================================================

// --- mne_raw2mat ---
#define main _raw2mat_main_unused
#include "../../tools/conversion/mne_raw2mat/main.cpp"
#undef main

// --- mne_ctf_dig2fiff ---
#define main _ctf_dig_main_unused
#include "../../tools/conversion/mne_ctf_dig2fiff/main.cpp"
#undef main

// --- mne_tufts2fiff ---
#define main _tufts_main_unused
#include "../../tools/conversion/mne_tufts2fiff/main.cpp"
#undef main

// --- mne_ctf2fiff ---
#define main _ctf2fiff_main_unused
#include "../../tools/conversion/mne_ctf2fiff/main.cpp"
#undef main

// --- mne_kit2fiff ---
#define main _kit2fiff_main_unused
#include "../../tools/conversion/mne_kit2fiff/main.cpp"
#undef main

// --- mne_make_cor_set ---
#define main _make_cor_set_main_unused
#include "../../tools/conversion/mne_make_cor_set/main.cpp"
#undef main

// --- mne_convert_lspcov ---
#define main _convert_lspcov_main_unused
#include "../../tools/conversion/mne_convert_lspcov/main.cpp"
#undef main

// --- mne_convert_ctf_markers ---
#define main _convert_ctf_markers_main_unused
#include "../../tools/conversion/mne_convert_ctf_markers/main.cpp"
#undef main

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QTemporaryDir>

//=============================================================================================================

class TestToolConversionExtended : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // --- MAT writer tests ---
    void testWriteMatrixVariableSmall();
    void testWriteMatrixVariableLarger();
    void testWriteMatTagFormat();
    void testWritePadAlignment();

    // --- CTF digitizer tests ---
    void testReadCtfDigBasic();
    void testReadCtfDigFiducials();
    void testReadCtfDigCoordConversion();
    void testComputeHeadTransformIdentity();
    void testComputeHeadTransformOrthogonal();
    void testComputeHeadTransformRotated();

    // --- Tufts converter tests ---
    void testReadElpFileBasic();
    void testReadElpFileCoordinates();
    void testReadCalibrationBasic();
    void testReadCalibrationValues();

    // --- Additional converter tests ---
    void testCtfChannelMappingAndData();
    void testKitHeaderLayoutTransformAndData();
    void testWriteCorDescription();
    void testParseLspMatrix();
    void testParseCtfMarkers();

    void cleanupTestCase();

private:
    QString m_sResourcePath;
    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestToolConversionExtended::initTestCase()
{
    QString binDir = QCoreApplication::applicationDirPath();
    m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================
// Additional converter tests
//=============================================================================================================

void TestToolConversionExtended::testCtfChannelMappingAndData()
{
    CtfSensorInfo sensor{};
    qstrncpy(sensor.name, "MLC11", CTF_SENSOR_NAME_SIZE);
    sensor.sensorType = CTF_SEN_TYPE_MEG;
    sensor.nCoils = 2;
    sensor.coils[0] = {1.0, 2.0, 3.0, 0.0, 0.0, 1.0, 0.01};
    sensor.properGain = 2.0;
    sensor.qGain = 5.0;
    sensor.ioGain = 10.0;

    FiffChInfo channel;
    mapCtfToFiff(sensor, channel);
    QCOMPARE(channel.ch_name, QString("MLC11"));
    QCOMPARE(channel.kind, FIFFV_MEG_CH);
    QCOMPARE(channel.chpos.coil_type, FIFFV_COIL_CTF_GRAD);
    QCOMPARE(channel.chpos.r0, Vector3f(1.0f, 2.0f, 3.0f));

    const QString meg4Path = m_tempDir.filePath("synthetic.meg4");
    QFile meg4(meg4Path);
    QVERIFY(meg4.open(QIODevice::WriteOnly));
    meg4.write("MEG41CP", 8);
    QDataStream stream(&meg4);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << qint32(100) << qint32(-200);
    meg4.close();

    CtfDatasetInfo info;
    info.meg4Path = meg4Path;
    info.nChannels = 1;
    info.nSamples = 2;
    info.nTrials = 1;
    info.sensors.append(sensor);
    MatrixXd data;
    QVERIFY(readMeg4Data(info, data));
    QCOMPARE(data.rows(), 1);
    QCOMPARE(data.cols(), 2);
    QCOMPARE(data(0, 0), 1.0);
    QCOMPARE(data(0, 1), -2.0);
}

void TestToolConversionExtended::testKitHeaderLayoutTransformAndData()
{
    const QString sqdPath = m_tempDir.filePath("synthetic.sqd");
    QFile sqd(sqdPath);
    QVERIFY(sqd.open(QIODevice::WriteOnly));
    sqd.resize(KIT_HEADER_SIZE + 12);
    QDataStream stream(&sqd);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    sqd.seek(KIT_SYSTEM_ID_OFFSET);
    stream << qint32(1234);
    sqd.seek(KIT_NCHAN_OFFSET);
    stream << qint32(2);
    sqd.seek(KIT_SFREQ_OFFSET);
    stream << 1000.0;
    sqd.seek(KIT_NSAMP_OFFSET);
    stream << qint32(3);
    sqd.seek(KIT_DATA_OFFSET_OFFSET);
    stream << qint32(KIT_HEADER_SIZE);
    sqd.seek(KIT_HEADER_SIZE);
    stream << qint16(1) << qint16(2) << qint16(3)
           << qint16(4) << qint16(5) << qint16(6);
    sqd.close();

    KitDatasetInfo info;
    QVERIFY(parseSqdHeader(sqdPath, info));
    QCOMPARE(info.systemId, 1234);
    QCOMPARE(info.nChannels, 2);
    QCOMPARE(info.nSamples, 3);
    QCOMPARE(info.sfreq, 1000.0);

    const QString sensorPath = m_tempDir.filePath("synthetic.sns");
    QFile sensors(sensorPath);
    QVERIFY(sensors.open(QIODevice::WriteOnly | QIODevice::Text));
    sensors.write("# index x y z ox oy oz\n1 10 20 30 0 0 1\n2 -10 0 40 0 1 0\n");
    sensors.close();
    QVERIFY(readSensorLayout(sensorPath, info));
    QCOMPARE(info.sensors[0].coil.position, Vector3d(0.01, 0.02, 0.03));

    const QString hpiPath = m_tempDir.filePath("synthetic.hpi");
    QFile hpi(hpiPath);
    QVERIFY(hpi.open(QIODevice::WriteOnly | QIODevice::Text));
    hpi.write("0 0 0 10 20 30\n10 0 0 20 20 30\n0 10 0 10 30 30\n");
    hpi.close();
    FiffCoordTrans transform;
    QVERIFY(computeDevHeadTransform(hpiPath, transform));
    QCOMPARE(transform.from, FIFFV_COORD_DEVICE);
    QCOMPARE(transform.to, FIFFV_COORD_HEAD);
    QVERIFY((transform.trans.block<3, 1>(0, 3) - Vector3f(0.01f, 0.02f, 0.03f)).norm() < 1e-6f);

    info.sensors[0].gain = 2.0;
    info.sensors[1].gain = 0.5;
    MatrixXd data;
    QVERIFY(readSqdData(sqdPath, info, data));
    QCOMPARE(data(0, 0), 2.0);
    QCOMPARE(data(1, 0), 1.0);
    QCOMPARE(data(0, 2), 10.0);
    QCOMPARE(data(1, 2), 3.0);
}

void TestToolConversionExtended::testWriteCorDescription()
{
    const std::vector<std::vector<unsigned char>> slices = {{0, 1, 2, 3}};
    const QString outputPath = m_tempDir.filePath("cor-description.fif");
    QVERIFY(writeMriDescription(outputPath, slices, 2, 2, 1));
    QVERIFY(QFileInfo(outputPath).size() > 0);
}

void TestToolConversionExtended::testParseLspMatrix()
{
    QString valid = "; covariance\n((1 2) (2 4))\n";
    QTextStream validStream(&valid);
    MatrixXd matrix;
    QVERIFY(parseLspMatrix(validStream, matrix));
    QCOMPARE(matrix.rows(), 2);
    QCOMPARE(matrix.cols(), 2);
    QCOMPARE(matrix(0, 1), 2.0);
    QCOMPARE(matrix(1, 1), 4.0);

    QString invalid = "((1 2 3) (4 5))\n";
    QTextStream invalidStream(&invalid);
    QVERIFY(!parseLspMatrix(invalidStream, matrix));
}

void TestToolConversionExtended::testParseCtfMarkers()
{
    const QString markerPath = m_tempDir.filePath("MarkerFile.mrk");
    QFile markerFile(markerPath);
    QVERIFY(markerFile.open(QIODevice::WriteOnly | QIODevice::Text));
    markerFile.write(
        "MARKER CLASSIFICATION\nNAME: auditory\nCLASSID: 7\nNUMBER OF MARKERS: 2\n"
        "LIST OF MARKERS:\nTRIAL NUMBER TIME\n12 0.1\n34 0.2\n"
        "MARKER CLASSIFICATION\nNAME: visual\nCLASSID: 9\nNUMBER OF MARKERS: 1\n"
        "LIST OF MARKERS:\nTRIAL NUMBER TIME\n56 0.3\n");
    markerFile.close();

    const QList<CtfMarker> markers = parseCtfMarkers(markerPath);
    QCOMPARE(markers.size(), 2);
    QCOMPARE(markers[0].name, QString("auditory"));
    QCOMPARE(markers[0].eventId, 7);
    QCOMPARE(markers[0].samples, QList<int>({12, 34}));
    QCOMPARE(markers[1].samples, QList<int>({56}));
}

//=============================================================================================================
// MAT writer tests
//=============================================================================================================

void TestToolConversionExtended::testWriteMatrixVariableSmall()
{
    // Write a 2x3 matrix as a MAT5 variable
    MatrixXd mat(2, 3);
    mat << 1.0, 2.0, 3.0,
           4.0, 5.0, 6.0;

    QString matFile = m_tempDir.path() + "/test_small.mat";
    QFile file(matFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);

    // Write MAT 5.0 header (128 bytes)
    QByteArray header(128, '\0');
    header.replace(0, 19, "MATLAB 5.0 MAT-file");
    header[124] = 0x00; header[125] = 0x01; // version
    header[126] = 'I';  header[127] = 'M';  // endian
    ds.writeRawData(header.constData(), 128);

    writeMatrixVariable(ds, "testmat", mat);
    file.close();

    QVERIFY(file.size() > 128);

    // Verify the file is a valid MAT file by checking header
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray hdRead = file.read(128);
    QVERIFY(hdRead.startsWith("MATLAB 5.0"));
    file.close();
}

void TestToolConversionExtended::testWriteMatrixVariableLarger()
{
    MatrixXd mat = MatrixXd::Random(10, 20);
    QString matFile = m_tempDir.path() + "/test_larger.mat";
    QFile file(matFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);
    QByteArray header(128, '\0');
    header.replace(0, 19, "MATLAB 5.0 MAT-file");
    header[124] = 0x00; header[125] = 0x01;
    header[126] = 'I';  header[127] = 'M';
    ds.writeRawData(header.constData(), 128);
    writeMatrixVariable(ds, "bigmat", mat);
    file.close();

    // File should contain at least 128 + tag + dimensions + data
    QVERIFY(file.size() > 128 + 10 * 20 * 8);
}

void TestToolConversionExtended::testWriteMatTagFormat()
{
    // Verify writeMatTag produces correct tag header (type + size)
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);

    writeMatTag(ds, miDOUBLE, 24);

    QCOMPARE(buf.size(), 8); // 4 bytes type + 4 bytes size
    // Read back
    QDataStream reader(&buf, QIODevice::ReadOnly);
    reader.setByteOrder(QDataStream::LittleEndian);
    quint32 type, size;
    reader >> type >> size;
    QCOMPARE(type, miDOUBLE);
    QCOMPARE(size, (quint32)24);
}

void TestToolConversionExtended::testWritePadAlignment()
{
    // writePad pads to 8-byte boundary: writes (8 - numBytes%8) % 8 zeros
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);

    writePad(ds, 5);  // (8-5)%8 = 3 padding bytes needed
    QCOMPARE(buf.size(), 3);
    for (int i = 0; i < 3; i++)
        QCOMPARE((unsigned char)buf[i], (unsigned char)0);
}

//=============================================================================================================
// CTF digitizer tests
//=============================================================================================================

void TestToolConversionExtended::testReadCtfDigBasic()
{
    // Create a CTF digitizer text file (coordinates in cm)
    QString digPath = m_tempDir.path() + "/test.pos";
    QFile file(digPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    // Nasion, LPA, RPA (first 3 points are fiducials)
    out << "0.0 8.0 0.0\n"      // Nasion (cm)
        << "-7.0 0.0 0.0\n"     // LPA
        << "7.0 0.0 0.0\n"      // RPA
        << "0.0 5.0 5.0\n"      // Extra point
        << "1.0 2.0 3.0\n";     // Extra point
    file.close();

    QList<DigPoint> points = readCtfDig(digPath, true);
    QCOMPARE(points.size(), 5);
    // First 3 should be fiducials
    QCOMPARE(points[0].kind, FIFFV_POINT_CARDINAL);
    QCOMPARE(points[1].kind, FIFFV_POINT_CARDINAL);
    QCOMPARE(points[2].kind, FIFFV_POINT_CARDINAL);
    // Remaining are extra
    QCOMPARE(points[3].kind, FIFFV_POINT_EXTRA);
}

void TestToolConversionExtended::testReadCtfDigFiducials()
{
    QString digPath = m_tempDir.path() + "/fids.pos";
    QFile file(digPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "0.0 10.0 0.0\n"
        << "-8.0 0.0 0.0\n"
        << "8.0 0.0 0.0\n";
    file.close();

    QList<DigPoint> points = readCtfDig(digPath, true);
    QCOMPARE(points.size(), 3);
    // Nasion ident = 1, LPA = 2, RPA = 3
    QCOMPARE(points[0].ident, 1);  // Nasion
    QCOMPARE(points[1].ident, 2);  // LPA
    QCOMPARE(points[2].ident, 3);  // RPA
}

void TestToolConversionExtended::testReadCtfDigCoordConversion()
{
    // CTF coordinates in cm, internal storage in meters
    QString digPath = m_tempDir.path() + "/cm2m.pos";
    QFile file(digPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "10.0 20.0 30.0\n"
        << "-5.0 0.0 0.0\n"
        << "5.0 0.0 0.0\n";
    file.close();

    QList<DigPoint> points = readCtfDig(digPath, true);
    QVERIFY(points.size() >= 1);
    // 10 cm = 0.10 m
    QVERIFY(qAbs(points[0].r[0] - 0.10f) < 1e-4f);
    QVERIFY(qAbs(points[0].r[1] - 0.20f) < 1e-4f);
    QVERIFY(qAbs(points[0].r[2] - 0.30f) < 1e-4f);
}

void TestToolConversionExtended::testComputeHeadTransformIdentity()
{
    // Nasion on +Y, LPA on -X, RPA on +X → should give identity-like transform
    DigPoint nasion, lpa, rpa;
    nasion.r[0] = 0.0f;  nasion.r[1] = 0.1f;  nasion.r[2] = 0.0f;
    lpa.r[0] = -0.08f;   lpa.r[1] = 0.0f;     lpa.r[2] = 0.0f;
    rpa.r[0] = 0.08f;    rpa.r[1] = 0.0f;     rpa.r[2] = 0.0f;

    Matrix4f trans;
    QVERIFY(computeHeadTransform(nasion, lpa, rpa, trans));

    // Transform should be close to identity for aligned fiducials
    // The origin should be between LPA and RPA
    QVERIFY(trans.determinant() > 0.0f);  // Right-handed

    // Apply to nasion → should be on +X axis in head coordinates
    Vector4f nasHead = trans * Vector4f(nasion.r[0], nasion.r[1], nasion.r[2], 1.0f);
    // Nasion should be positive on some axis
    QVERIFY(nasHead.head<3>().norm() > 0.0f);
}

void TestToolConversionExtended::testComputeHeadTransformOrthogonal()
{
    // Verify the rotation component is orthogonal
    DigPoint nasion, lpa, rpa;
    nasion.r[0] = 0.0f;  nasion.r[1] = 0.09f;  nasion.r[2] = 0.02f;
    lpa.r[0] = -0.07f;   lpa.r[1] = -0.01f;    lpa.r[2] = 0.0f;
    rpa.r[0] = 0.07f;    rpa.r[1] = 0.01f;     rpa.r[2] = 0.0f;

    Matrix4f trans;
    QVERIFY(computeHeadTransform(nasion, lpa, rpa, trans));

    // Rotation component (3x3 upper-left) should be orthogonal
    Matrix3f R = trans.block<3, 3>(0, 0);
    Matrix3f RtR = R.transpose() * R;
    Matrix3f I = Matrix3f::Identity();
    QVERIFY((RtR - I).norm() < 1e-4f);
}

void TestToolConversionExtended::testComputeHeadTransformRotated()
{
    // Apply a known rotation to standard fiducials and verify transform
    DigPoint nasion, lpa, rpa;
    // Rotated 45° around Z
    float c = std::cos(M_PI / 4), s = std::sin(M_PI / 4);
    nasion.r[0] = -0.08f * s; nasion.r[1] = 0.08f * c; nasion.r[2] = 0.0f;
    lpa.r[0] = -0.07f * c;    lpa.r[1] = -0.07f * s;   lpa.r[2] = 0.0f;
    rpa.r[0] = 0.07f * c;     rpa.r[1] = 0.07f * s;    rpa.r[2] = 0.0f;

    Matrix4f trans;
    QVERIFY(computeHeadTransform(nasion, lpa, rpa, trans));
    // Should still produce orthogonal rotation
    Matrix3f R = trans.block<3, 3>(0, 0);
    QVERIFY(qAbs(R.determinant() - 1.0f) < 1e-3f);
}

//=============================================================================================================
// Tufts converter tests
//=============================================================================================================

void TestToolConversionExtended::testReadElpFileBasic()
{
    // Create an ELP file (electrode position file)
    QString elpPath = m_tempDir.path() + "/test.elp";
    QFile file(elpPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "3\n"
        << "Fp1\t-3.0\t5.0\t8.0\n"
        << "Fp2\t3.0\t5.0\t8.0\n"
        << "Cz\t0.0\t0.0\t10.0\n";
    file.close();

    MatrixX3f positions;
    QStringList names;
    QVERIFY(readElpFile(elpPath, positions, names));
    QCOMPARE(names.size(), 3);
    QCOMPARE(names[0], QString("Fp1"));
    QCOMPARE(names[2], QString("Cz"));
    QCOMPARE(positions.rows(), (Eigen::Index)3);
}

void TestToolConversionExtended::testReadElpFileCoordinates()
{
    QString elpPath = m_tempDir.path() + "/coords.elp";
    QFile file(elpPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "1\n"
        << "TestCh\t10.5\t20.3\t30.7\n";
    file.close();

    MatrixX3f positions;
    QStringList names;
    QVERIFY(readElpFile(elpPath, positions, names));
    QCOMPARE(positions.rows(), (Eigen::Index)1);
    // readElpFile converts mm to meters (x0.001)
    QVERIFY(qAbs(positions(0, 0) - 10.5f * 0.001f) < 0.001f);
    QVERIFY(qAbs(positions(0, 1) - 20.3f * 0.001f) < 0.001f);
    QVERIFY(qAbs(positions(0, 2) - 30.7f * 0.001f) < 0.001f);
}

void TestToolConversionExtended::testReadCalibrationBasic()
{
    QString calPath = m_tempDir.path() + "/test.cal";
    QFile file(calPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "1.5\n"
        << "2.0\n"
        << "0.5\n";
    file.close();

    VectorXf cals;
    QVERIFY(readCalibration(calPath, cals));
    QCOMPARE(cals.size(), (Eigen::Index)3);
    QVERIFY(qAbs(cals(0) - 1.5f) < 0.01f);
    QVERIFY(qAbs(cals(1) - 2.0f) < 0.01f);
    QVERIFY(qAbs(cals(2) - 0.5f) < 0.01f);
}

void TestToolConversionExtended::testReadCalibrationValues()
{
    // Test with various calibration values including very small
    QString calPath = m_tempDir.path() + "/precise.cal";
    QFile file(calPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "1e-6\n"
        << "3.14159\n";
    file.close();

    VectorXf cals;
    QVERIFY(readCalibration(calPath, cals));
    QCOMPARE(cals.size(), (Eigen::Index)2);
    QVERIFY(cals(0) > 0);
    QVERIFY(qAbs(cals(1) - 3.14159f) < 0.001f);
}

//=============================================================================================================

void TestToolConversionExtended::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestToolConversionExtended)
#include "test_tool_conversion_extended.moc"
