//=============================================================================================================
/**
 * @file     test_tool_conversion_functions.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
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
 * @brief    Data-driven tests for conversion tool static functions
 *           (mne_brain_vision2fiff, mne_convert_dig_data, mne_convert_surface).
 */

//=============================================================================================================
// Include tool sources via #define main trick
//=============================================================================================================

// --- mne_brain_vision2fiff ---
#define main _bv2fiff_main_unused
#include "../../tools/conversion/mne_brain_vision2fiff/main.cpp"
#undef main

// --- mne_convert_dig_data ---
#define main _dig_data_main_unused
#include "../../tools/conversion/mne_convert_dig_data/main.cpp"
#undef main

// --- mne_convert_surface ---
#define main _surface_main_unused
#include "../../tools/conversion/mne_convert_surface/main.cpp"
#undef main

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QTemporaryDir>

//=============================================================================================================
/**
 * @brief Data-driven tests for conversion tool functions.
 */
class TestToolConversionFunctions : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // --- BrainVision parser tests ---
    void testParseBVHeaderBasic();
    void testParseBVHeaderChannelInfo();
    void testParseBVHeaderDefaults();
    void testParseBVHeaderMissingChannels();
    void testParseBVHeaderNonexistent();
    void testParseBVMarkersBasic();
    void testParseBVMarkersEmpty();
    void testReadBVDataInt16Multiplexed();
    void testReadBVDataFloat32Vectorized();
    void testReadBVDataUint16();
    void testReadBVDataWithRealFile();
    void testBrainVisionRoundTrip();

    // --- Digitizer conversion tests ---
    void testReadWriteHptsRoundTrip();
    void testReadHptsCategories();
    void testReadHptsCoordinateConversion();
    void testWriteHptsFormat();
    void testReadWriteFifDigRoundTrip();
    void testReadHptsEmpty();
    void testReadHptsComments();

    // --- Surface conversion tests ---
    void testDetectFormatFif();
    void testDetectFormatTri();
    void testDetectFormatSmf();
    void testDetectFormatFreeSurfer();
    void testDetectFormatUnknown();
    void testTriFileRoundTrip();
    void testTriFileIndexing();
    void testSmfFileRoundTrip();
    void testFifSurfaceRoundTrip();
    void testFifSurfaceWithRealBem();
    void testConvertTriToSmf();
    void testSurfaceWithLargeMesh();

    void cleanupTestCase();

private:
    QString m_sResourcePath;
    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestToolConversionFunctions::initTestCase()
{
    QString binDir = QCoreApplication::applicationDirPath();
    m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================
// BrainVision parser tests
//=============================================================================================================

void TestToolConversionFunctions::testParseBVHeaderBasic()
{
    // Create a minimal .vhdr file
    QString vhdrPath = m_tempDir.path() + "/test.vhdr";
    QFile file(vhdrPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "Brain Vision Data Exchange Header File Version 1.0\n"
        << "; Test comment\n"
        << "\n"
        << "[Common Infos]\n"
        << "DataFile=test.eeg\n"
        << "MarkerFile=test.vmrk\n"
        << "DataFormat=BINARY\n"
        << "DataOrientation=MULTIPLEXED\n"
        << "NumberOfChannels=3\n"
        << "SamplingInterval=2000\n"
        << "\n"
        << "[Binary Infos]\n"
        << "BinaryFormat=INT_16\n"
        << "\n"
        << "[Channel Infos]\n"
        << "Ch1=Fp1,,0.5,µV\n"
        << "Ch2=Fp2,,1.0,µV\n"
        << "Ch3=Cz,,0.5,µV\n";
    file.close();

    BVHeader hdr;
    QVERIFY(parseBVHeader(vhdrPath, hdr));
    QCOMPARE(hdr.dataFile, QString("test.eeg"));
    QCOMPARE(hdr.markerFile, QString("test.vmrk"));
    QCOMPARE(hdr.dataFormat, QString("BINARY"));
    QCOMPARE(hdr.dataOrientation, 0);  // MULTIPLEXED
    QCOMPARE(hdr.numberOfChannels, 3);
    QCOMPARE(hdr.samplingInterval, 2000.0);
    QCOMPARE(hdr.binaryFormat, QString("INT_16"));
}

void TestToolConversionFunctions::testParseBVHeaderChannelInfo()
{
    QString vhdrPath = m_tempDir.path() + "/ch_test.vhdr";
    QFile file(vhdrPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "[Common Infos]\n"
        << "NumberOfChannels=3\n"
        << "SamplingInterval=1000\n"
        << "\n"
        << "[Channel Infos]\n"
        << "Ch1=EEG1,Ref,0.5,µV\n"
        << "Ch2=EEG2,Ref,1.0,mV\n"
        << "Ch3=EMG,,2.0,µV\n";
    file.close();

    BVHeader hdr;
    QVERIFY(parseBVHeader(vhdrPath, hdr));
    QCOMPARE(hdr.channels.size(), 3);
    QCOMPARE(hdr.channels[0].name, QString("EEG1"));
    QCOMPARE(hdr.channels[0].refName, QString("Ref"));
    QCOMPARE(hdr.channels[0].resolution, 0.5);
    QCOMPARE(hdr.channels[0].unit, QString("µV"));
    QCOMPARE(hdr.channels[1].resolution, 1.0);
    QCOMPARE(hdr.channels[1].unit, QString("mV"));
    QCOMPARE(hdr.channels[2].name, QString("EMG"));
}

void TestToolConversionFunctions::testParseBVHeaderDefaults()
{
    // Minimal header with very few fields - test defaults
    QString vhdrPath = m_tempDir.path() + "/default_test.vhdr";
    QFile file(vhdrPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "[Common Infos]\n"
        << "NumberOfChannels=2\n";
    file.close();

    BVHeader hdr;
    QVERIFY(parseBVHeader(vhdrPath, hdr));
    QCOMPARE(hdr.dataOrientation, 0);     // default: MULTIPLEXED
    QCOMPARE(hdr.binaryFormat, QString("INT_16"));  // default
    QCOMPARE(hdr.numberOfChannels, 2);
    // Missing channels should be auto-filled
    QCOMPARE(hdr.channels.size(), 2);
    QCOMPARE(hdr.channels[0].name, QString("Ch1"));
    QCOMPARE(hdr.channels[0].resolution, 1.0);
}

void TestToolConversionFunctions::testParseBVHeaderMissingChannels()
{
    // NumberOfChannels > actual channel definitions
    QString vhdrPath = m_tempDir.path() + "/missing_ch.vhdr";
    QFile file(vhdrPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "[Common Infos]\n"
        << "NumberOfChannels=5\n"
        << "[Channel Infos]\n"
        << "Ch1=EEG1,,1.0,µV\n"
        << "Ch2=EEG2,,1.0,µV\n";
    file.close();

    BVHeader hdr;
    QVERIFY(parseBVHeader(vhdrPath, hdr));
    QCOMPARE(hdr.channels.size(), 5);
    // First 2 from file, rest auto-generated
    QCOMPARE(hdr.channels[0].name, QString("EEG1"));
    QCOMPARE(hdr.channels[2].name, QString("Ch3"));
}

void TestToolConversionFunctions::testParseBVHeaderNonexistent()
{
    BVHeader hdr;
    QVERIFY(!parseBVHeader("/nonexistent/path.vhdr", hdr));
}

void TestToolConversionFunctions::testParseBVMarkersBasic()
{
    QString vmrkPath = m_tempDir.path() + "/test.vmrk";
    QFile file(vmrkPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "Brain Vision Data Exchange Marker File Version 1.0\n"
        << "\n"
        << "[Marker Infos]\n"
        << "Mk1=Stimulus,S  1,100,1,0\n"
        << "Mk2=Response,R  1,250,1,0\n"
        << "Mk3=Stimulus,S  2,500,2,3\n";
    file.close();

    BVHeader hdr;
    QVERIFY(parseBVMarkers(vmrkPath, hdr));
    QCOMPARE(hdr.markers.size(), 3);
    QCOMPARE(hdr.markers[0].type, QString("Stimulus"));
    QCOMPARE(hdr.markers[0].description, QString("S  1"));
    QCOMPARE(hdr.markers[0].position, 100);
    QCOMPARE(hdr.markers[0].duration, 1);
    QCOMPARE(hdr.markers[0].channel, 0);
    QCOMPARE(hdr.markers[2].channel, 3);
}

void TestToolConversionFunctions::testParseBVMarkersEmpty()
{
    BVHeader hdr;
    QVERIFY(!parseBVMarkers("/nonexistent/markers.vmrk", hdr));
}

void TestToolConversionFunctions::testReadBVDataInt16Multiplexed()
{
    // Create binary INT_16 data: 2 channels, 4 samples, multiplexed
    QString dataPath = m_tempDir.path() + "/int16_mux.eeg";
    QFile file(dataPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);
    // Multiplexed: [ch0_s0, ch1_s0, ch0_s1, ch1_s1, ...]
    qint16 values[] = {100, 200, 300, 400, 500, 600, 700, 800};
    for (int i = 0; i < 8; i++) ds << values[i];
    file.close();

    BVHeader hdr;
    hdr.numberOfChannels = 2;
    hdr.binaryFormat = "INT_16";
    hdr.dataOrientation = 0; // MULTIPLEXED
    hdr.channels.clear();
    BVChannelInfo ch1; ch1.name = "A"; ch1.resolution = 0.5; hdr.channels.append(ch1);
    BVChannelInfo ch2; ch2.name = "B"; ch2.resolution = 1.0; hdr.channels.append(ch2);

    MatrixXd data;
    QVERIFY(readBVData(dataPath, hdr, data));
    QCOMPARE(data.rows(), 2);
    QCOMPARE(data.cols(), (Eigen::Index)4);
    // ch0: 100*0.5, 300*0.5, 500*0.5, 700*0.5
    QCOMPARE(data(0, 0), 50.0);
    QCOMPARE(data(0, 1), 150.0);
    // ch1: 200*1.0, 400*1.0, 600*1.0, 800*1.0
    QCOMPARE(data(1, 0), 200.0);
    QCOMPARE(data(1, 3), 800.0);
}

void TestToolConversionFunctions::testReadBVDataFloat32Vectorized()
{
    // Create binary IEEE_FLOAT_32 data: 2 channels, 3 samples, vectorized
    QString dataPath = m_tempDir.path() + "/float32_vec.eeg";
    QFile file(dataPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setFloatingPointPrecision(QDataStream::SinglePrecision);
    // Vectorized: all ch0 samples, then all ch1 samples
    float values[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    for (int i = 0; i < 6; i++) ds << values[i];
    file.close();

    BVHeader hdr;
    hdr.numberOfChannels = 2;
    hdr.binaryFormat = "IEEE_FLOAT_32";
    hdr.dataOrientation = 1; // VECTORIZED
    hdr.channels.clear();
    BVChannelInfo ch1; ch1.name = "A"; ch1.resolution = 2.0; hdr.channels.append(ch1);
    BVChannelInfo ch2; ch2.name = "B"; ch2.resolution = 1.0; hdr.channels.append(ch2);

    MatrixXd data;
    QVERIFY(readBVData(dataPath, hdr, data));
    QCOMPARE(data.rows(), 2);
    QCOMPARE(data.cols(), (Eigen::Index)3);
    QCOMPARE(data(0, 0), 2.0);   // 1.0 * 2.0
    QCOMPARE(data(0, 2), 6.0);   // 3.0 * 2.0
    QCOMPARE(data(1, 0), 4.0);   // 4.0 * 1.0
}

void TestToolConversionFunctions::testReadBVDataUint16()
{
    QString dataPath = m_tempDir.path() + "/uint16.eeg";
    QFile file(dataPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);
    quint16 values[] = {1000, 2000};
    for (int i = 0; i < 2; i++) ds << values[i];
    file.close();

    BVHeader hdr;
    hdr.numberOfChannels = 1;
    hdr.binaryFormat = "UINT_16";
    hdr.dataOrientation = 0;
    hdr.channels.clear();
    BVChannelInfo ch; ch.name = "A"; ch.resolution = 0.1; hdr.channels.append(ch);

    MatrixXd data;
    QVERIFY(readBVData(dataPath, hdr, data));
    QCOMPARE(data.rows(), 1);
    QCOMPARE(data.cols(), (Eigen::Index)2);
    QVERIFY(qAbs(data(0, 0) - 100.0) < 0.01);  // 1000 * 0.1
    QVERIFY(qAbs(data(0, 1) - 200.0) < 0.01);  // 2000 * 0.1
}

void TestToolConversionFunctions::testReadBVDataWithRealFile()
{
    QString vhdrPath = m_sResourcePath + "BIDS/sub-01/ses-01/ieeg/sub-01_ses-01_task-rest_ieeg.vhdr";
    if (!QFile::exists(vhdrPath))
        QSKIP("BIDS BrainVision test data not available");

    BVHeader hdr;
    QVERIFY(parseBVHeader(vhdrPath, hdr));
    QCOMPARE(hdr.numberOfChannels, 32);
    QCOMPARE(hdr.dataFormat, QString("BINARY"));
    QCOMPARE(hdr.binaryFormat, QString("INT_16"));

    // Parse markers
    QString vmrkPath = m_sResourcePath + "BIDS/sub-01/ses-01/ieeg/sub-01_ses-01_task-rest_ieeg.vmrk";
    if (QFile::exists(vmrkPath)) {
        QVERIFY(parseBVMarkers(vmrkPath, hdr));
        QVERIFY(hdr.markers.size() > 0);
    }

    // Read actual data
    QFileInfo fi(vhdrPath);
    QString dataPath = fi.path() + "/" + hdr.dataFile;
    if (QFile::exists(dataPath)) {
        MatrixXd data;
        QVERIFY(readBVData(dataPath, hdr, data));
        QCOMPARE(data.rows(), 32);
        QVERIFY(data.cols() > 0);
    }
}

void TestToolConversionFunctions::testBrainVisionRoundTrip()
{
    // Write a BV header, parse it, verify all fields
    QString vhdrPath = m_tempDir.path() + "/roundtrip.vhdr";
    {
        QFile file(vhdrPath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "[Common Infos]\n"
            << "DataFile=roundtrip.eeg\n"
            << "DataFormat=BINARY\n"
            << "DataOrientation=VECTORIZED\n"
            << "NumberOfChannels=4\n"
            << "SamplingInterval=500\n"
            << "[Binary Infos]\n"
            << "BinaryFormat=IEEE_FLOAT_32\n"
            << "[Channel Infos]\n"
            << "Ch1=EEG1,,0.5,µV\n"
            << "Ch2=EEG2,,1.0,µV\n"
            << "Ch3=EOG,,2.0,µV\n"
            << "Ch4=STI,,1.0,V\n";
        file.close();
    }

    BVHeader hdr;
    QVERIFY(parseBVHeader(vhdrPath, hdr));
    QCOMPARE(hdr.dataOrientation, 1);  // VECTORIZED
    QCOMPARE(hdr.binaryFormat, QString("IEEE_FLOAT_32"));
    QCOMPARE(hdr.samplingInterval, 500.0);
    QCOMPARE(hdr.channels.size(), 4);
    QCOMPARE(hdr.channels[3].unit, QString("V"));
}

//=============================================================================================================
// Digitizer conversion tests
//=============================================================================================================

void TestToolConversionFunctions::testReadWriteHptsRoundTrip()
{
    // Create hpts file
    QString inputPath = m_tempDir.path() + "/test.hpts";
    {
        QFile file(inputPath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "cardinal 1 0.0 80.0 0.0\n"      // Nasion (mm)
            << "cardinal 2 -70.0 0.0 0.0\n"     // LPA
            << "cardinal 3 70.0 0.0 0.0\n"      // RPA
            << "hpi 1 0.0 60.0 50.0\n"
            << "eeg 1 -30.0 50.0 80.0\n"
            << "extra 1 10.0 10.0 90.0\n";
        file.close();
    }

    QList<DigPt> points = readHpts(inputPath);
    QCOMPARE(points.size(), 6);

    // Write back
    QString outputPath = m_tempDir.path() + "/test_out.hpts";
    QVERIFY(writeHpts(outputPath, points));

    // Read again and verify
    QList<DigPt> points2 = readHpts(outputPath);
    QCOMPARE(points2.size(), 6);

    for (int i = 0; i < points.size(); i++) {
        QCOMPARE(points2[i].kind, points[i].kind);
        QCOMPARE(points2[i].ident, points[i].ident);
        for (int j = 0; j < 3; j++) {
            QVERIFY(qAbs(points2[i].r[j] - points[i].r[j]) < 1e-3f);
        }
    }
}

void TestToolConversionFunctions::testReadHptsCategories()
{
    QString path = m_tempDir.path() + "/categories.hpts";
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "cardinal 1 10.0 20.0 30.0\n"
            << "fiducial 2 40.0 50.0 60.0\n"
            << "hpi 1 70.0 80.0 90.0\n"
            << "eeg 1 100.0 110.0 120.0\n"
            << "extra 1 130.0 140.0 150.0\n"
            << "other 1 0.0 0.0 0.0\n";
        file.close();
    }

    QList<DigPt> points = readHpts(path);
    QCOMPARE(points.size(), 6);
    QCOMPARE(points[0].kind, FIFFV_POINT_CARDINAL);
    QCOMPARE(points[1].kind, FIFFV_POINT_CARDINAL);  // fiducial = cardinal
    QCOMPARE(points[2].kind, FIFFV_POINT_HPI);
    QCOMPARE(points[3].kind, FIFFV_POINT_EEG);
    QCOMPARE(points[4].kind, FIFFV_POINT_EXTRA);
    QCOMPARE(points[5].kind, FIFFV_POINT_EXTRA);  // unknown → extra
}

void TestToolConversionFunctions::testReadHptsCoordinateConversion()
{
    // hpts coordinates are in mm, internal storage is in meters
    QString path = m_tempDir.path() + "/coords.hpts";
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "cardinal 1 100.0 200.0 300.0\n";
        file.close();
    }

    QList<DigPt> points = readHpts(path);
    QCOMPARE(points.size(), 1);
    QVERIFY(qAbs(points[0].r[0] - 0.100f) < 1e-5f);  // 100 mm → 0.1 m
    QVERIFY(qAbs(points[0].r[1] - 0.200f) < 1e-5f);
    QVERIFY(qAbs(points[0].r[2] - 0.300f) < 1e-5f);
}

void TestToolConversionFunctions::testWriteHptsFormat()
{
    QList<DigPt> points;
    DigPt p;
    p.kind = FIFFV_POINT_CARDINAL;
    p.ident = 1;
    p.r[0] = 0.1f; p.r[1] = 0.2f; p.r[2] = 0.3f;
    points.append(p);

    QString outPath = m_tempDir.path() + "/format_test.hpts";
    QVERIFY(writeHpts(outPath, points));

    // Verify output format
    QFile file(outPath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString line = QTextStream(&file).readLine().trimmed();
    file.close();
    // Should be: "cardinal 1 100.0 200.0 300.0" (back to mm)
    QVERIFY(line.startsWith("cardinal"));
    QVERIFY(line.contains("100.0"));
}

void TestToolConversionFunctions::testReadWriteFifDigRoundTrip()
{
    // Create digitization points
    QList<DigPt> original;
    for (int i = 0; i < 5; i++) {
        DigPt p;
        p.kind = (i < 3) ? FIFFV_POINT_CARDINAL : FIFFV_POINT_EEG;
        p.ident = i + 1;
        p.r[0] = 0.01f * (i + 1);
        p.r[1] = 0.02f * (i + 1);
        p.r[2] = 0.03f * (i + 1);
        original.append(p);
    }

    // Write FIFF
    QString fifPath = m_tempDir.path() + "/dig.fif";
    QVERIFY(writeFifDig(fifPath, original));

    // Read back
    QList<DigPt> loaded = readFifDig(fifPath);
    QCOMPARE(loaded.size(), original.size());
    for (int i = 0; i < original.size(); i++) {
        QCOMPARE(loaded[i].kind, original[i].kind);
        QCOMPARE(loaded[i].ident, original[i].ident);
        for (int j = 0; j < 3; j++) {
            QVERIFY(qAbs(loaded[i].r[j] - original[i].r[j]) < 1e-6f);
        }
    }
}

void TestToolConversionFunctions::testReadHptsEmpty()
{
    QString path = m_tempDir.path() + "/empty.hpts";
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "# This file contains only comments\n"
            << "# No actual data\n";
        file.close();
    }

    QList<DigPt> points = readHpts(path);
    QCOMPARE(points.size(), 0);
}

void TestToolConversionFunctions::testReadHptsComments()
{
    QString path = m_tempDir.path() + "/comments.hpts";
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "# Header comment\n"
            << "cardinal 1 10.0 20.0 30.0\n"
            << "# Middle comment\n"
            << "eeg 1 40.0 50.0 60.0\n"
            << "\n"  // Empty line
            << "extra 1 70.0 80.0 90.0\n";
        file.close();
    }

    QList<DigPt> points = readHpts(path);
    QCOMPARE(points.size(), 3);
}

//=============================================================================================================
// Surface conversion tests
//=============================================================================================================

void TestToolConversionFunctions::testDetectFormatFif()
{
    QCOMPARE(detectFormat("test.fif"), FORMAT_FIF);
    QCOMPARE(detectFormat("/path/to/surface.fif"), FORMAT_FIF);
}

void TestToolConversionFunctions::testDetectFormatTri()
{
    QCOMPARE(detectFormat("test.tri"), FORMAT_TRI);
}

void TestToolConversionFunctions::testDetectFormatSmf()
{
    QCOMPARE(detectFormat("model.smf"), FORMAT_SMF);
}

void TestToolConversionFunctions::testDetectFormatFreeSurfer()
{
    QCOMPARE(detectFormat("lh.white"), FORMAT_FREESURFER);
    QCOMPARE(detectFormat("rh.pial"), FORMAT_FREESURFER);
    QCOMPARE(detectFormat("/path/to/lh.inflated"), FORMAT_FREESURFER);
}

void TestToolConversionFunctions::testDetectFormatUnknown()
{
    QCOMPARE(detectFormat("unknown.xyz"), FORMAT_UNKNOWN);
    QCOMPARE(detectFormat("noextension"), FORMAT_UNKNOWN);
}

void TestToolConversionFunctions::testTriFileRoundTrip()
{
    // Create a simple tetrahedron
    MatrixX3f rr(4, 3);
    rr << 0, 0, 0,
          1, 0, 0,
          0, 1, 0,
          0, 0, 1;

    MatrixX3i tris(4, 3);
    tris << 0, 1, 2,
            0, 1, 3,
            0, 2, 3,
            1, 2, 3;

    QString triPath = m_tempDir.path() + "/test.tri";
    QVERIFY(writeTriFile(triPath, rr, tris));
    QVERIFY(QFile::exists(triPath));

    MatrixX3f rr2;
    MatrixX3i tris2;
    QVERIFY(readTriFile(triPath, rr2, tris2));

    QCOMPARE(rr2.rows(), rr.rows());
    QCOMPARE(tris2.rows(), tris.rows());

    // Verify vertices
    for (int i = 0; i < rr.rows(); i++) {
        for (int j = 0; j < 3; j++) {
            QVERIFY(qAbs(rr2(i, j) - rr(i, j)) < 1e-4f);
        }
    }

    // Verify triangles
    for (int i = 0; i < tris.rows(); i++) {
        for (int j = 0; j < 3; j++) {
            QCOMPARE(tris2(i, j), tris(i, j));
        }
    }
}

void TestToolConversionFunctions::testTriFileIndexing()
{
    // Verify tri files use 1-based indexing externally, 0-based internally
    QString triPath = m_tempDir.path() + "/index_test.tri";
    {
        QFile file(triPath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "3\n"
            << "1 0.0 0.0 0.0\n"
            << "2 1.0 0.0 0.0\n"
            << "3 0.0 1.0 0.0\n"
            << "1\n"
            << "1 1 2 3\n";  // 1-based: vertices 1,2,3
        file.close();
    }

    MatrixX3f rr;
    MatrixX3i tris;
    QVERIFY(readTriFile(triPath, rr, tris));
    QCOMPARE(rr.rows(), (Eigen::Index)3);
    QCOMPARE(tris.rows(), (Eigen::Index)1);
    // Internal: 0-based
    QCOMPARE(tris(0, 0), 0);
    QCOMPARE(tris(0, 1), 1);
    QCOMPARE(tris(0, 2), 2);
}

void TestToolConversionFunctions::testSmfFileRoundTrip()
{
    MatrixX3f rr(3, 3);
    rr << 0.0f, 0.0f, 0.0f,
          1.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f;

    MatrixX3i tris(1, 3);
    tris << 0, 1, 2;

    QString smfPath = m_tempDir.path() + "/test.smf";
    QVERIFY(writeSmfFile(smfPath, rr, tris));

    MatrixX3f rr2;
    MatrixX3i tris2;
    QVERIFY(readSmfFile(smfPath, rr2, tris2));

    QCOMPARE(rr2.rows(), rr.rows());
    QCOMPARE(tris2.rows(), tris.rows());
    for (int i = 0; i < rr.rows(); i++) {
        for (int j = 0; j < 3; j++) {
            QVERIFY(qAbs(rr2(i, j) - rr(i, j)) < 1e-4f);
        }
    }
}

void TestToolConversionFunctions::testFifSurfaceRoundTrip()
{
    // Create a tetrahedron - a closed surface required by MNEBem reader
    MatrixX3f rr(4, 3);
    rr << 0.01f, 0.01f, 0.01f,
          0.05f, 0.01f, 0.01f,
          0.03f, 0.05f, 0.01f,
          0.03f, 0.03f, 0.05f;

    MatrixX3i tris(4, 3);
    tris << 0, 1, 2,
            0, 1, 3,
            0, 2, 3,
            1, 2, 3;

    QString fifPath = m_tempDir.path() + "/surf.fif";
    QVERIFY(writeFifSurface(fifPath, rr, tris, FIFFV_BEM_SURF_ID_BRAIN));

    MatrixX3f rr2;
    MatrixX3i tris2;
    QVERIFY(readFifSurface(fifPath, rr2, tris2));

    QCOMPARE(rr2.rows(), rr.rows());
    QCOMPARE(tris2.rows(), tris.rows());
}

void TestToolConversionFunctions::testFifSurfaceWithRealBem()
{
    QString bemPath = m_sResourcePath + "subjects/sample/bem/sample-5120-bem.fif";
    if (!QFile::exists(bemPath))
        QSKIP("BEM test data not available");

    MatrixX3f rr;
    MatrixX3i tris;
    QVERIFY(readFifSurface(bemPath, rr, tris));
    QVERIFY(rr.rows() > 0);
    QVERIFY(tris.rows() > 0);
    // BEM surface with 2562 nodes (sample BEM file)
    QCOMPARE(rr.rows(), (Eigen::Index)2562);
}

void TestToolConversionFunctions::testConvertTriToSmf()
{
    // Write as tri, read, write as smf, read, verify
    MatrixX3f rr(4, 3);
    rr << 0, 0, 0,   1, 0, 0,   0, 1, 0,   0, 0, 1;
    MatrixX3i tris(4, 3);
    tris << 0, 1, 2,   0, 1, 3,   0, 2, 3,   1, 2, 3;

    QString triPath = m_tempDir.path() + "/convert.tri";
    QVERIFY(writeTriFile(triPath, rr, tris));

    MatrixX3f rr_tri;
    MatrixX3i tris_tri;
    QVERIFY(readTriFile(triPath, rr_tri, tris_tri));

    QString smfPath = m_tempDir.path() + "/convert.smf";
    QVERIFY(writeSmfFile(smfPath, rr_tri, tris_tri));

    MatrixX3f rr_smf;
    MatrixX3i tris_smf;
    QVERIFY(readSmfFile(smfPath, rr_smf, tris_smf));

    QCOMPARE(rr_smf.rows(), rr.rows());
    QCOMPARE(tris_smf.rows(), tris.rows());
}

void TestToolConversionFunctions::testSurfaceWithLargeMesh()
{
    // Create an icosahedron-like mesh (12 vertices, 20 faces)
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
    MatrixX3f rr(12, 3);
    rr.row(0)  = Vector3f(-1,  phi, 0).normalized() * 0.1f;
    rr.row(1)  = Vector3f( 1,  phi, 0).normalized() * 0.1f;
    rr.row(2)  = Vector3f(-1, -phi, 0).normalized() * 0.1f;
    rr.row(3)  = Vector3f( 1, -phi, 0).normalized() * 0.1f;
    rr.row(4)  = Vector3f(0, -1,  phi).normalized() * 0.1f;
    rr.row(5)  = Vector3f(0,  1,  phi).normalized() * 0.1f;
    rr.row(6)  = Vector3f(0, -1, -phi).normalized() * 0.1f;
    rr.row(7)  = Vector3f(0,  1, -phi).normalized() * 0.1f;
    rr.row(8)  = Vector3f( phi, 0, -1).normalized() * 0.1f;
    rr.row(9)  = Vector3f( phi, 0,  1).normalized() * 0.1f;
    rr.row(10) = Vector3f(-phi, 0, -1).normalized() * 0.1f;
    rr.row(11) = Vector3f(-phi, 0,  1).normalized() * 0.1f;

    MatrixX3i tris(20, 3);
    tris <<  0,11,5,  0,5,1,  0,1,7,  0,7,10,  0,10,11,
             1,5,9,   5,11,4, 11,10,2, 10,7,6,  7,1,8,
             3,9,4,   3,4,2,  3,2,6,   3,6,8,   3,8,9,
             4,9,5,   2,4,11, 6,2,10,  8,6,7,   9,8,1;

    // Round-trip through all formats
    // TRI
    QString triPath = m_tempDir.path() + "/ico.tri";
    QVERIFY(writeTriFile(triPath, rr, tris));
    MatrixX3f rrT; MatrixX3i trisT;
    QVERIFY(readTriFile(triPath, rrT, trisT));
    QCOMPARE(rrT.rows(), (Eigen::Index)12);
    QCOMPARE(trisT.rows(), (Eigen::Index)20);

    // SMF
    QString smfPath = m_tempDir.path() + "/ico.smf";
    QVERIFY(writeSmfFile(smfPath, rr, tris));
    MatrixX3f rrS; MatrixX3i trisS;
    QVERIFY(readSmfFile(smfPath, rrS, trisS));
    QCOMPARE(rrS.rows(), (Eigen::Index)12);

    // FIF
    QString fifPath = m_tempDir.path() + "/ico.fif";
    QVERIFY(writeFifSurface(fifPath, rr, tris, FIFFV_BEM_SURF_ID_HEAD));
    MatrixX3f rrF; MatrixX3i trisF;
    QVERIFY(readFifSurface(fifPath, rrF, trisF));
    QCOMPARE(rrF.rows(), (Eigen::Index)12);
}

//=============================================================================================================

void TestToolConversionFunctions::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestToolConversionFunctions)
#include "test_tool_conversion_functions.moc"
