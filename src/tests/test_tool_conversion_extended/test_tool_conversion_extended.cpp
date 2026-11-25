//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_tool_conversion_extended.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
 * @brief    Tests for extended conversion tools (ctf_dig2fiff, tufts2fiff, ctf2fiff, kit2fiff and others).
 */

//=============================================================================================================
// Include tool sources
//=============================================================================================================

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
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <numbers>
#include <sstream>
#include <string>
#include <vector>

//=============================================================================================================

class TestToolConversionExtended : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

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
    void testReadMghFloatAndMgz();
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
    QCOMPARE(channel.ch_name, std::string("MLC11"));
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
    info.meg4Path = meg4Path.toStdString();
    info.nChannels = 1;
    info.nSamples = 2;
    info.nTrials = 1;
    info.sensors.push_back(sensor);
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
    QVERIFY(parseSqdHeader(sqdPath.toStdString(), info));
    QCOMPARE(info.systemId, 1234);
    QCOMPARE(info.nChannels, 2);
    QCOMPARE(info.nSamples, 3);
    QCOMPARE(info.sfreq, 1000.0);

    const QString sensorPath = m_tempDir.filePath("synthetic.sns");
    QFile sensors(sensorPath);
    QVERIFY(sensors.open(QIODevice::WriteOnly | QIODevice::Text));
    sensors.write("# index x y z ox oy oz\n1 10 20 30 0 0 1\n2 -10 0 40 0 1 0\n");
    sensors.close();
    QVERIFY(readSensorLayout(sensorPath.toStdString(), info));
    QCOMPARE(info.sensors[0].coil.position, Vector3d(0.01, 0.02, 0.03));

    const QString hpiPath = m_tempDir.filePath("synthetic.hpi");
    QFile hpi(hpiPath);
    QVERIFY(hpi.open(QIODevice::WriteOnly | QIODevice::Text));
    hpi.write("0 0 0 10 20 30\n10 0 0 20 20 30\n0 10 0 10 30 30\n");
    hpi.close();
    FiffCoordTrans transform;
    QVERIFY(computeDevHeadTransform(hpiPath.toStdString(), transform));
    QCOMPARE(transform.from, FIFFV_COORD_DEVICE);
    QCOMPARE(transform.to, FIFFV_COORD_HEAD);
    QVERIFY((transform.trans.block<3, 1>(0, 3) - Vector3f(0.01f, 0.02f, 0.03f)).norm() < 1e-6f);

    info.sensors[0].gain = 2.0;
    info.sensors[1].gain = 0.5;
    MatrixXd data;
    QVERIFY(readSqdData(sqdPath.toStdString(), info, data));
    QCOMPARE(data(0, 0), 2.0);
    QCOMPARE(data(1, 0), 1.0);
    QCOMPARE(data(0, 2), 10.0);
    QCOMPARE(data(1, 2), 3.0);
}

namespace
{

// A 2 x 2 x 1 mgh volume of 4-byte big-endian floats
std::vector<std::byte> floatMgh(const std::vector<float>& voxels)
{
    std::vector<std::byte> bytes(284 + 4 * voxels.size());
    std::size_t pos = 0;
    const auto put = [&bytes, &pos](auto value) {
        value = mne::to_big_endian(value);
        std::memcpy(bytes.data() + pos, &value, sizeof(value));
        pos += sizeof(value);
    };
    for (const std::int32_t field : {1, 2, 2, 1, 1, 3}) // version, width, height, depth, frames, MRI_FLOAT
        put(field);
    pos = 284;
    for (const float voxel : voxels)
        put(voxel);
    return bytes;
}

// The bytes as a gzip member with one stored (uncompressed) deflate block
std::vector<std::byte> gzipStored(const std::vector<std::byte>& data)
{
    std::uint32_t crc = 0xffffffffu;
    for (const std::byte b : data) {
        crc ^= static_cast<std::uint32_t>(b);
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    crc = ~crc;
    const auto size = static_cast<std::uint16_t>(data.size());
    std::vector<std::byte> gz = {std::byte{0x1f}, std::byte{0x8b}, std::byte{8}, std::byte{0}, std::byte{0},
                                 std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{255},
                                 std::byte{1},
                                 std::byte(size & 0xff), std::byte(size >> 8),
                                 std::byte(~size & 0xff), std::byte((~size >> 8) & 0xff)};
    gz.insert(gz.end(), data.begin(), data.end());
    for (const std::uint32_t word : {crc, static_cast<std::uint32_t>(data.size())})
        for (int shift = 0; shift < 32; shift += 8)
            gz.push_back(std::byte((word >> shift) & 0xff));
    return gz;
}

} // namespace

//=============================================================================================================

void TestToolConversionExtended::testReadMghFloatAndMgz()
{
    const std::vector<std::byte> mgh = floatMgh({0.0f, 100.0f, 254.0f, 400.0f});
    const QString mghPath = m_tempDir.filePath("float.mgh");
    const QString mgzPath = m_tempDir.filePath("float.mgz");
    QVERIFY(mne::write_file_bytes(mghPath.toStdString(), mgh).has_value());
    QVERIFY(mne::write_file_bytes(mgzPath.toStdString(), gzipStored(mgh)).has_value());

    const std::vector<unsigned char> expected = {0, 100, 254, 255};
    for (const QString& path : {mghPath, mgzPath}) {
        std::vector<std::vector<unsigned char>> slices;
        int width = 0, height = 0, nslice = 0;
        QVERIFY2(readMghVolume(path.toStdString(), slices, width, height, nslice), qPrintable(path));
        QCOMPARE(width, 2);
        QCOMPARE(height, 2);
        QCOMPARE(nslice, 1);
        QVERIFY(slices.size() == 1 && slices[0] == expected);
    }
}

//=============================================================================================================

void TestToolConversionExtended::testWriteCorDescription()
{
    const std::vector<std::vector<unsigned char>> slices = {{0, 1, 2, 3}};
    const QString outputPath = m_tempDir.filePath("cor-description.fif");
    QVERIFY(writeMriDescription(outputPath.toStdString(), slices, 2, 2, 1));
    QVERIFY(QFileInfo(outputPath).size() > 0);
}

void TestToolConversionExtended::testParseLspMatrix()
{
    std::istringstream validStream("; covariance\n((1 2) (2 4))\n");
    MatrixXd matrix;
    QVERIFY(parseLspMatrix(validStream, matrix));
    QCOMPARE(matrix.rows(), 2);
    QCOMPARE(matrix.cols(), 2);
    QCOMPARE(matrix(0, 1), 2.0);
    QCOMPARE(matrix(1, 1), 4.0);

    std::istringstream invalidStream("((1 2 3) (4 5))\n");
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

    const std::vector<CtfMarker> markers = parseCtfMarkers(markerPath.toStdString());
    QCOMPARE(std::ssize(markers), 2);
    QCOMPARE(markers[0].name, std::string("auditory"));
    QCOMPARE(markers[0].eventId, 7);
    QVERIFY(markers[0].samples == std::vector<int>({12, 34}));
    QVERIFY(markers[1].samples == std::vector<int>({56}));
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
    out << "0.0 8.0 0.0\n"  // Nasion (cm)
        << "-7.0 0.0 0.0\n" // LPA
        << "7.0 0.0 0.0\n"  // RPA
        << "0.0 5.0 5.0\n"  // Extra point
        << "1.0 2.0 3.0\n"; // Extra point
    file.close();

    std::vector<DigPoint> points = readCtfDig(digPath.toStdString(), true);
    QCOMPARE(std::ssize(points), 5);
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

    std::vector<DigPoint> points = readCtfDig(digPath.toStdString(), true);
    QCOMPARE(std::ssize(points), 3);
    // Nasion ident = 1, LPA = 2, RPA = 3
    QCOMPARE(points[0].ident, 1); // Nasion
    QCOMPARE(points[1].ident, 2); // LPA
    QCOMPARE(points[2].ident, 3); // RPA
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

    std::vector<DigPoint> points = readCtfDig(digPath.toStdString(), true);
    QVERIFY(!points.empty());
    // 10 cm = 0.10 m
    QVERIFY(qAbs(points[0].r[0] - 0.10f) < 1e-4f);
    QVERIFY(qAbs(points[0].r[1] - 0.20f) < 1e-4f);
    QVERIFY(qAbs(points[0].r[2] - 0.30f) < 1e-4f);
}

void TestToolConversionExtended::testComputeHeadTransformIdentity()
{
    // Nasion on +Y, LPA on -X, RPA on +X → should give identity-like transform
    DigPoint nasion, lpa, rpa;
    nasion.r[0] = 0.0f;
    nasion.r[1] = 0.1f;
    nasion.r[2] = 0.0f;
    lpa.r[0] = -0.08f;
    lpa.r[1] = 0.0f;
    lpa.r[2] = 0.0f;
    rpa.r[0] = 0.08f;
    rpa.r[1] = 0.0f;
    rpa.r[2] = 0.0f;

    Matrix4f trans;
    QVERIFY(computeHeadTransform(nasion, lpa, rpa, trans));

    // Transform should be close to identity for aligned fiducials
    // The origin should be between LPA and RPA
    QVERIFY(trans.determinant() > 0.0f); // Right-handed

    // Apply to nasion → should be on +X axis in head coordinates
    Vector4f nasHead = trans * Vector4f(nasion.r[0], nasion.r[1], nasion.r[2], 1.0f);
    // Nasion should be positive on some axis
    QVERIFY(nasHead.head<3>().norm() > 0.0f);
}

void TestToolConversionExtended::testComputeHeadTransformOrthogonal()
{
    // Verify the rotation component is orthogonal
    DigPoint nasion, lpa, rpa;
    nasion.r[0] = 0.0f;
    nasion.r[1] = 0.09f;
    nasion.r[2] = 0.02f;
    lpa.r[0] = -0.07f;
    lpa.r[1] = -0.01f;
    lpa.r[2] = 0.0f;
    rpa.r[0] = 0.07f;
    rpa.r[1] = 0.01f;
    rpa.r[2] = 0.0f;

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
    float c = std::cos(std::numbers::pi / 4), s = std::sin(std::numbers::pi / 4);
    nasion.r[0] = -0.08f * s;
    nasion.r[1] = 0.08f * c;
    nasion.r[2] = 0.0f;
    lpa.r[0] = -0.07f * c;
    lpa.r[1] = -0.07f * s;
    lpa.r[2] = 0.0f;
    rpa.r[0] = 0.07f * c;
    rpa.r[1] = 0.07f * s;
    rpa.r[2] = 0.0f;

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
    StringList names;
    QVERIFY(readElpFile(elpPath.toStdString(), positions, names));
    QCOMPARE(std::ssize(names), 3);
    QCOMPARE(names[0], std::string("Fp1"));
    QCOMPARE(names[2], std::string("Cz"));
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
    StringList names;
    QVERIFY(readElpFile(elpPath.toStdString(), positions, names));
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
    QVERIFY(readCalibration(calPath.toStdString(), cals));
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
    QVERIFY(readCalibration(calPath.toStdString(), cals));
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
