//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_bids.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
 * @brief    Data-driven tests for the BIDS library.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <bids/bids_path.h>
#include <bids/bids_channel.h>
#include <bids/bids_electrode.h>
#include <bids/bids_event.h>
#include <bids/bids_coordinate_system.h>
#include <bids/bids_dataset_description.h>
#include <bids/bids_raw_data.h>
#include <bids/bids_global.h>
#include <bids/readers/bids_brain_vision_reader.h>
#include <bids/readers/bids_edf_reader.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;

//=============================================================================================================
/**
 * @brief Data-driven test suite for the BIDS library.
 *
 * Tests cover:
 *   - BIDSPath construction and path generation
 *   - Round-trip I/O for channels, electrodes, events, coordinate systems, dataset_description
 *   - BidsRawData::read() with real BrainVision and EDF test data
 *   - BidsRawData::write() round-trip
 */
class TestBids : public QObject
{
    Q_OBJECT

public:
    TestBids() = default;

private:
    QString bidsRoot() const; /**< Path to BIDS test fixtures. */
    QString dataPath() const; /**< Base path for mne-cpp-test-data. */

private slots:
    void initTestCase();

    // BIDSPath
    void testPathConstruction();
    void testPathSidecarDerivation();

    // channels.tsv
    void testChannelsReadTsv();
    void testChannelsWriteRoundTrip();

    // electrodes.tsv
    void testElectrodesReadTsv();
    void testElectrodesWriteRoundTrip();

    // events.tsv
    void testEventsReadTsv();
    void testEventsWriteRoundTrip();

    // coordsystem.json
    void testCoordinateSystemReadJson();
    void testCoordinateSystemWriteRoundTrip();

    // dataset_description.json
    void testDatasetDescriptionRead();
    void testDatasetDescriptionWriteRoundTrip();

    // BidsRawData::read — BrainVision (sub-01)
    void testReadBrainVision();

    // BidsRawData::read — EDF (sub-02)
    void testReadEdf();

    // BidsRawData::write round-trip
    void testWriteRoundTrip();

    // BrainVisionReader against mne.io.read_raw_brainvision
    void testBrainVisionReaderMatchesPython();

    // BIDSPath setter/getter coverage
    void testPathSettersGetters();
    void testPathValidation();
    void testPathEquality();
    void testBidsGlobalBuildInfo();

    void cleanupTestCase();
};

//=============================================================================================================
// HELPERS
//=============================================================================================================

QString TestBids::dataPath() const
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/");
}

QString TestBids::bidsRoot() const
{
    return dataPath() + QStringLiteral("BIDS");
}

//=============================================================================================================
// initTestCase
//=============================================================================================================

void TestBids::initTestCase()
{
    // Verify fixture data exists
    QVERIFY2(QDir(bidsRoot()).exists(),
             qPrintable("BIDS test fixture directory not found: " + bidsRoot()));
    QVERIFY(QFileInfo::exists(bidsRoot() + "/dataset_description.json"));
}

//=============================================================================================================
// BIDSPath tests
//=============================================================================================================

void TestBids::testPathConstruction()
{
    BIDSPath path(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");

    QCOMPARE(path.root(), bidsRoot());
    QCOMPARE(path.subject(), QStringLiteral("01"));
    QCOMPARE(path.session(), QStringLiteral("01"));
    QCOMPARE(path.task(), QStringLiteral("rest"));
    QCOMPARE(path.datatype(), QStringLiteral("ieeg"));
    QCOMPARE(path.suffix(), QStringLiteral("ieeg"));
    QCOMPARE(path.extension(), QStringLiteral(".vhdr"));

    // basename: sub-01_ses-01_task-rest_ieeg.vhdr
    QCOMPARE(path.basename(), QStringLiteral("sub-01_ses-01_task-rest_ieeg.vhdr"));

    // filePath should exist on disk
    QVERIFY2(QFileInfo::exists(path.filePath()),
             qPrintable("Expected file not found: " + path.filePath()));
}

void TestBids::testPathSidecarDerivation()
{
    BIDSPath path(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");

    BIDSPath chPath = path.channelsTsvPath();
    QVERIFY(chPath.basename().endsWith("_channels.tsv"));
    QVERIFY(QFileInfo::exists(chPath.filePath()));

    BIDSPath elPath = path.electrodesTsvPath();
    QVERIFY(elPath.basename().endsWith("_electrodes.tsv"));
    QVERIFY(QFileInfo::exists(elPath.filePath()));

    BIDSPath evPath = path.eventsTsvPath();
    QVERIFY(evPath.basename().endsWith("_events.tsv"));
    QVERIFY(QFileInfo::exists(evPath.filePath()));

    BIDSPath csPath = path.coordsystemJsonPath();
    QVERIFY(csPath.basename().endsWith("_coordsystem.json"));
    QVERIFY(QFileInfo::exists(csPath.filePath()));

    BIDSPath sjPath = path.sidecarJsonPath();
    QVERIFY(sjPath.basename().endsWith("_ieeg.json"));
    QVERIFY(QFileInfo::exists(sjPath.filePath()));
}

//=============================================================================================================
// channels.tsv tests
//=============================================================================================================

void TestBids::testChannelsReadTsv()
{
    BIDSPath path(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    QList<BidsChannel> channels = BidsChannel::readTsv(path.channelsTsvPath().filePath());

    // BrainVision test.vhdr has 32 channels
    QCOMPARE(channels.size(), 32);

    // First channel
    QCOMPARE(channels[0].name, QStringLiteral("FP1"));
    QCOMPARE(channels[0].type, QStringLiteral("EEG"));
    QCOMPARE(channels[0].status, QStringLiteral("good"));

    // P4 (index 7) is marked bad in the fixture
    QCOMPARE(channels[7].name, QStringLiteral("P4"));
    QCOMPARE(channels[7].status, QStringLiteral("bad"));

    // Last channel
    QCOMPARE(channels[31].name, QStringLiteral("ReRef"));

    // Sampling frequency
    QCOMPARE(channels[0].samplingFreq, QStringLiteral("1000"));
}

void TestBids::testChannelsWriteRoundTrip()
{
    // Read original
    BIDSPath path(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    QList<BidsChannel> original = BidsChannel::readTsv(path.channelsTsvPath().filePath());

    // Write to temp
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString tmpPath = tmpDir.path() + "/channels.tsv";
    QVERIFY(BidsChannel::writeTsv(tmpPath, original));

    // Read back
    QList<BidsChannel> roundTripped = BidsChannel::readTsv(tmpPath);
    QCOMPARE(roundTripped.size(), original.size());

    for (int i = 0; i < original.size(); ++i) {
        QCOMPARE(roundTripped[i].name, original[i].name);
        QCOMPARE(roundTripped[i].type, original[i].type);
        QCOMPARE(roundTripped[i].units, original[i].units);
        QCOMPARE(roundTripped[i].status, original[i].status);
    }
}

//=============================================================================================================
// electrodes.tsv tests
//=============================================================================================================

void TestBids::testElectrodesReadTsv()
{
    BIDSPath path(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    QList<BidsElectrode> electrodes = BidsElectrode::readTsv(path.electrodesTsvPath().filePath());

    QCOMPARE(electrodes.size(), 10);

    // First electrode
    QCOMPARE(electrodes[0].name, QStringLiteral("FP1"));
    QVERIFY(!electrodes[0].x.isEmpty());
    QVERIFY(electrodes[0].x != "n/a");

    // Check a known position
    float x = electrodes[0].x.toFloat();
    QVERIFY(std::abs(x - (-0.0254f)) < 0.001f);
}

void TestBids::testElectrodesWriteRoundTrip()
{
    BIDSPath path(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    QList<BidsElectrode> original = BidsElectrode::readTsv(path.electrodesTsvPath().filePath());

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString tmpPath = tmpDir.path() + "/electrodes.tsv";
    QVERIFY(BidsElectrode::writeTsv(tmpPath, original));

    QList<BidsElectrode> roundTripped = BidsElectrode::readTsv(tmpPath);
    QCOMPARE(roundTripped.size(), original.size());

    for (int i = 0; i < original.size(); ++i) {
        QCOMPARE(roundTripped[i].name, original[i].name);
        QCOMPARE(roundTripped[i].x, original[i].x);
        QCOMPARE(roundTripped[i].y, original[i].y);
        QCOMPARE(roundTripped[i].z, original[i].z);
    }
}

//=============================================================================================================
// events.tsv tests
//=============================================================================================================

void TestBids::testEventsReadTsv()
{
    BIDSPath path(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    QList<BidsEvent> events = BidsEvent::readTsv(path.eventsTsvPath().filePath());

    QCOMPARE(events.size(), 4);

    // First event
    QCOMPARE(events[0].onset, 0.0f);
    QCOMPARE(events[0].sample, 0);
    QCOMPARE(events[0].value, 1);
    QCOMPARE(events[0].trialType, QStringLiteral("stimulus"));

    // Second event at 1.5 s
    QVERIFY(std::abs(events[1].onset - 1.5f) < 0.001f);
    QCOMPARE(events[1].sample, 1500);
    QCOMPARE(events[1].trialType, QStringLiteral("response"));
}

void TestBids::testEventsWriteRoundTrip()
{
    BIDSPath path(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    QList<BidsEvent> original = BidsEvent::readTsv(path.eventsTsvPath().filePath());

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString tmpPath = tmpDir.path() + "/events.tsv";
    QVERIFY(BidsEvent::writeTsv(tmpPath, original));

    QList<BidsEvent> roundTripped = BidsEvent::readTsv(tmpPath);
    QCOMPARE(roundTripped.size(), original.size());

    for (int i = 0; i < original.size(); ++i) {
        QVERIFY(std::abs(roundTripped[i].onset - original[i].onset) < 0.001f);
        QCOMPARE(roundTripped[i].sample, original[i].sample);
        QCOMPARE(roundTripped[i].value, original[i].value);
        QCOMPARE(roundTripped[i].trialType, original[i].trialType);
    }
}

//=============================================================================================================
// coordsystem.json tests
//=============================================================================================================

void TestBids::testCoordinateSystemReadJson()
{
    BIDSPath path(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    BidsCoordinateSystem cs = BidsCoordinateSystem::readJson(path.coordsystemJsonPath().filePath());

    QCOMPARE(cs.system, QStringLiteral("ACPC"));
    QCOMPARE(cs.units, QStringLiteral("m"));
}

void TestBids::testCoordinateSystemWriteRoundTrip()
{
    BIDSPath path(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    BidsCoordinateSystem original = BidsCoordinateSystem::readJson(path.coordsystemJsonPath().filePath());

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString tmpPath = tmpDir.path() + "/coordsystem.json";
    QVERIFY(BidsCoordinateSystem::writeJson(tmpPath, original));

    BidsCoordinateSystem roundTripped = BidsCoordinateSystem::readJson(tmpPath);
    QCOMPARE(roundTripped.system, original.system);
    QCOMPARE(roundTripped.units, original.units);
}

//=============================================================================================================
// dataset_description.json tests
//=============================================================================================================

void TestBids::testDatasetDescriptionRead()
{
    BidsDatasetDescription desc = BidsDatasetDescription::read(bidsRoot() + "/dataset_description.json");

    QCOMPARE(desc.name, QStringLiteral("MNE-CPP Test Dataset"));
    QCOMPARE(desc.bidsVersion, QStringLiteral("1.9.0"));
    QCOMPARE(desc.datasetType, QStringLiteral("raw"));
    QCOMPARE(desc.license, QStringLiteral("CC0"));
}

void TestBids::testDatasetDescriptionWriteRoundTrip()
{
    BidsDatasetDescription original = BidsDatasetDescription::read(bidsRoot() + "/dataset_description.json");

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString tmpPath = tmpDir.path() + "/dataset_description.json";
    QVERIFY(BidsDatasetDescription::write(tmpPath, original));

    BidsDatasetDescription roundTripped = BidsDatasetDescription::read(tmpPath);
    QCOMPARE(roundTripped.name, original.name);
    QCOMPARE(roundTripped.bidsVersion, original.bidsVersion);
    QCOMPARE(roundTripped.datasetType, original.datasetType);
    QCOMPARE(roundTripped.license, original.license);
}

//=============================================================================================================
// BidsRawData::read — BrainVision (sub-01)
//=============================================================================================================

void TestBids::testReadBrainVision()
{
    BIDSPath path(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    BidsRawData data = BidsRawData::read(path);

    QVERIFY2(data.isValid(), "BidsRawData::read failed for BrainVision file");

    // Channel count: 32 channels from BrainVision test.vhdr
    QCOMPARE(data.raw.info.nchan, 32);

    // Sampling frequency: SamplingInterval=1000 µs → 1000 Hz
    QVERIFY(std::abs(data.raw.info.sfreq - 1000.0f) < 1.0f);

    // Bad channels: P4 marked bad in channels.tsv
    QVERIFY(data.raw.info.bads.contains(QStringLiteral("P4")));
    QCOMPARE(data.raw.info.bads.size(), 1);

    // Events
    QCOMPARE(data.events.size(), 4);
    QCOMPARE(data.events[0].trialType, QStringLiteral("stimulus"));

    // Event ID map
    QVERIFY(data.eventIdMap.contains(QStringLiteral("stimulus")));
    QVERIFY(data.eventIdMap.contains(QStringLiteral("response")));

    // Electrodes
    QCOMPARE(data.electrodes.size(), 10);
    QCOMPARE(data.electrodes[0].name, QStringLiteral("FP1"));

    // Coordinate system
    QCOMPARE(data.coordinateSystem.system, QStringLiteral("ACPC"));
    QCOMPARE(data.coordinateSystem.units, QStringLiteral("m"));

    // Digitization points loaded
    QVERIFY(data.raw.info.dig.size() >= 10);

    // Sidecar metadata
    QCOMPARE(data.ieegReference, QStringLiteral("Cz"));
    QCOMPARE(data.manufacturer, QStringLiteral("BrainProducts"));
    QCOMPARE(data.recordingType, QStringLiteral("continuous"));

    // Line frequency from sidecar
    QVERIFY(std::abs(data.raw.info.linefreq - 50.0f) < 0.1f);

    // Reader should be alive
    QVERIFY(data.reader != nullptr);
}

//=============================================================================================================
// BidsRawData::read — EDF (sub-02)
//=============================================================================================================

void TestBids::testReadEdf()
{
    BIDSPath path(bidsRoot(), "02", "01", "rest", "eeg", "eeg", ".edf");
    BidsRawData data = BidsRawData::read(path);

    QVERIFY2(data.isValid(), "BidsRawData::read failed for EDF file");

    // Channel count: 25 channels in the EDF
    QCOMPARE(data.raw.info.nchan, 25);

    // Sampling frequency: 128 Hz
    QVERIFY(std::abs(data.raw.info.sfreq - 128.0f) < 1.0f);

    // Bad channels: EEG O1 marked bad in channels.tsv (EDF uses full label)
    QVERIFY(data.raw.info.bads.contains(QStringLiteral("EEG O1")));

    // Events
    QCOMPARE(data.events.size(), 3);

    // Reader should be alive
    QVERIFY(data.reader != nullptr);

    // Samples in volts, as mne.io.read_raw_edf(...).get_data()
    QCOMPARE(data.reader->getSampleCount(), 1228L);
    QCOMPARE(data.raw.first_samp, 0);
    QCOMPARE(data.raw.last_samp, 1227); // mne raw.last_samp is inclusive
    const Eigen::MatrixXd samples = data.reader->readRawSegment(0, 1228).cast<double>();
    QCOMPARE(samples.rows(), Eigen::Index(25));
    QVERIFY(std::abs(samples(0, 0) - 0.175940656291) < 1e-7);
    QVERIFY(std::abs(samples(5, 101) - 0.175486234485) < 1e-7);
    QVERIFY(std::abs(samples.row(1).cwiseAbs().sum() - 267.274902297948) < 1e-3);

    // Each channel is scaled from its own physical dimension (uV, mV, V), like mne read_raw_edf
    QTemporaryDir dir;
    const auto field = [](const QString& s, int width) {
        return s.leftJustified(width, QLatin1Char(' '), true).toLatin1();
    };
    // One data record of 4 samples for 3 signals
    const auto header = [&field](const QByteArray& version, const QString& reserved, const QStringList& labels, const QStringList& dims,
                                 const QStringList& physMin, const QStringList& physMax, const QString& digMin, const QString& digMax) {
        QByteArray bytes = version + field("X X X X", 80) + field("Startdate 01-JAN-2026 X X X", 80) + field("01.01.26", 8) + field("12.00.00", 8) + field("1024", 8) + field(reserved, 44) + field("1", 8) + field("1", 8) + field("3", 4);
        // Per-signal fields: label, transducer, dimension, physical min/max, digital min/max, prefilter, samples/record, reserved
        const QList<std::pair<QStringList, int>> columns{{labels, 16}, {QStringList(3), 80}, {dims, 8}, {physMin, 8}, {physMax, 8}, {QStringList(3, digMin), 8}, {QStringList(3, digMax), 8}, {QStringList(3), 80}, {QStringList(3, "4"), 8}, {QStringList(3), 32}};
        for (const auto& [column, width] : columns) {
            for (const QString& value : column) {
                bytes += field(value, width);
            }
        }
        return bytes;
    };
    const QStringList labels{QStringLiteral("EEG Cz"), QStringLiteral("EEG Pz"), QStringLiteral("Temp")};
    QByteArray edfBytes = header(field("0", 8), QString(), labels, {"uV", "mV", "V"}, {"-500.0", "-2.0", "-1.0"}, {"500.0", "2.0", "1.0"}, "-32768", "32767");
    QVERIFY(edfBytes.size() == 1024);
    for (const qint16 value : {-32768, -1000, 0, 32767, 100, 200, -300, 400, -16384, 0, 16384, 32767}) {
        edfBytes.append(char(value & 0xff)).append(char((value >> 8) & 0xff));
    }
    QFile mixed(dir.filePath(QStringLiteral("mixed.edf")));
    QVERIFY(mixed.open(QIODevice::WriteOnly));
    mixed.write(edfBytes);
    mixed.close();
    EDFReader mixedReader;
    QVERIFY(mixedReader.open(mixed.fileName()));
    const Eigen::MatrixXd mixedData = mixedReader.readRawSegment(0, 4).cast<double>();
    Eigen::MatrixXd expected(3, 4);
    expected << -5e-4, -1.525139238575e-05, 7.629510948334e-09, 5e-4,
        6.134126802472e-06, 1.223773556115e-05, -1.828030823224e-05, 2.444495307851e-05,
        -4.999923704891e-01, 1.525902189670e-05, 5.000228885328e-01, 1.0;
    for (int k = 0; k < 3; ++k) {
        QVERIFY2((mixedData.row(k) - expected.row(k)).cwiseAbs().maxCoeff() < 1e-4 * expected.row(k).cwiseAbs().maxCoeff(), qPrintable(labels[k])); // float32 reader
    }

    // Like mne read_raw_edf: unrecognised labels are EEG; "Status" is an unscaled stim channel
    EDFReader reduced;
    QVERIFY(reduced.open(dataPath() + QStringLiteral("EEG/test_reduced.edf")));
    const FIFFLIB::FiffInfo reducedInfo = reduced.getInfo();
    const int a4 = reducedInfo.ch_names.indexOf(QStringLiteral("A4"));
    const int status = reducedInfo.ch_names.indexOf(QStringLiteral("Status"));
    QVERIFY(a4 < 0 && status >= 0); // A4 is sampled below the highest rate
    const int a10 = reducedInfo.ch_names.indexOf(QStringLiteral("A10"));
    QCOMPARE(reducedInfo.chs[a10].kind, FIFFV_EEG_CH);
    QCOMPARE(reducedInfo.chs[a10].unit, FIFF_UNIT_V);
    QCOMPARE(reducedInfo.chs[status].kind, FIFFV_STIM_CH);
    QCOMPARE(reducedInfo.chs[status].unit, FIFF_UNIT_NONE);
    const Eigen::MatrixXf reducedData = reduced.readRawSegment(0, 2);
    QCOMPARE(reducedData(status, 0), 4352.0f);
    QCOMPARE(reducedData(status, 1), 0.0f);

    // BDF stores 24-bit samples; the Status channel keeps the low 17 bits of the raw value (mne read_raw_bdf)
    QByteArray bdfBytes = header(QByteArray("\xff") + field("BIOSEMI", 7), QStringLiteral("24BIT"), {"EEG Cz", "EXG1", "Status"},
                                 {"uV", "mV", "Boolean"}, {"-262144", "-262", "-8388608"}, {"262143", "262", "8388607"}, "-8388608", "8388607");
    for (const qint32 value : {-8388608, -1000, 0, 8388607, 100, -200, 300, -400, 0, -65531, 65539, 8388607}) {
        bdfBytes.append(char(value & 0xff)).append(char((value >> 8) & 0xff)).append(char((value >> 16) & 0xff));
    }
    QFile bdf(dir.filePath(QStringLiteral("synth.bdf")));
    QVERIFY(bdf.open(QIODevice::WriteOnly));
    bdf.write(bdfBytes);
    bdf.close();
    EDFReader bdfReader;
    QVERIFY(bdfReader.open(bdf.fileName()));
    QCOMPARE(bdfReader.getSampleCount(), 4L);
    QCOMPARE(bdfReader.getInfo().chs[2].kind, FIFFV_STIM_CH);
    const Eigen::MatrixXd bdfData = bdfReader.readRawSegment(0, 4).cast<double>();
    Eigen::MatrixXd bdfExpected(3, 4);
    bdfExpected << -0.262144, -3.1734317286867926e-05, -4.843750288709998e-07, 0.262143,
        3.1388999902333236e-06, -6.2309507269447055e-06, 9.385467135018676e-06, -1.2477517871730058e-05,
        0.0, 65541.0, 65539.0, 131071.0;
    for (int k = 0; k < 3; ++k) {
        QVERIFY2((bdfData.row(k) - bdfExpected.row(k)).cwiseAbs().maxCoeff() < 1e-6 * bdfExpected.row(k).cwiseAbs().maxCoeff(), qPrintable(QString::number(k)));
    }
}

//=============================================================================================================
// BidsRawData::write round-trip
//=============================================================================================================

void TestBids::testWriteRoundTrip()
{
    // Read original BrainVision dataset
    BIDSPath srcPath(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    BidsRawData original = BidsRawData::read(srcPath);
    QVERIFY(original.isValid());

    // Write to a temporary BIDS directory
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    BIDSPath dstPath(tmpDir.path(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");

    BidsRawData::WriteOptions opts;
    opts.overwrite = true;
    opts.copyData = true;
    opts.datasetName = "TestRoundTrip";

    BIDSPath written = original.write(dstPath, srcPath.filePath(), opts);
    // A failed write returns BIDSPath(), whose filePath() is "." and therefore never empty
    QVERIFY(written == dstPath);
    QVERIFY(QFileInfo::exists(written.filePath()));

    // Verify sidecar files were created
    QVERIFY(QFileInfo::exists(dstPath.channelsTsvPath().filePath()));
    QVERIFY(QFileInfo::exists(dstPath.eventsTsvPath().filePath()));
    QVERIFY(QFileInfo::exists(dstPath.sidecarJsonPath().filePath()));

    // Read back from the written location
    BidsRawData readBack = BidsRawData::read(dstPath);
    QVERIFY2(readBack.isValid(), "Failed to read back written BIDS dataset");

    // Verify round-trip fidelity
    QCOMPARE(readBack.raw.info.nchan, original.raw.info.nchan);
    QVERIFY(std::abs(readBack.raw.info.sfreq - original.raw.info.sfreq) < 1.0f);
    QCOMPARE(readBack.events.size(), original.events.size());
    QCOMPARE(readBack.raw.info.bads.size(), original.raw.info.bads.size());

    // Electrode positions stay attached to their names (Cz is the 17th channel but the 9th electrode)
    for (const BidsElectrode& before : original.electrodes) {
        auto after = std::find_if(readBack.electrodes.cbegin(), readBack.electrodes.cend(),
                                  [&](const BidsElectrode& e) { return e.name == before.name; });
        QVERIFY2(after != readBack.electrodes.cend() && std::abs(after->z.toDouble() - before.z.toDouble()) < 1e-6, qPrintable(before.name));
    }

    // Without overwrite, existing sidecars are left alone and the write is refused
    BidsRawData::WriteOptions keep;
    keep.copyData = false;
    QVERIFY(original.write(dstPath, QString(), keep) == BIDSPath());

    // Incomplete paths and an empty recording are refused before anything is written
    QTemporaryDir refusedDir;
    BidsRawData empty;
    QVERIFY(empty.write(BIDSPath(refusedDir.path(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr"), QString(), opts) == BIDSPath());
    QVERIFY(original.write(BIDSPath(QString(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr"), QString(), opts) == BIDSPath());
    QVERIFY(original.write(BIDSPath(refusedDir.path(), QString(), "01", "rest", "ieeg", "ieeg", ".vhdr"), QString(), opts) == BIDSPath());
    QVERIFY(original.write(BIDSPath(refusedDir.path(), "01", "01", QString(), "ieeg", "ieeg", ".vhdr"), QString(), opts) == BIDSPath());
    QVERIFY(original.write(BIDSPath(refusedDir.path(), "01", "01", "rest", QString(), "ieeg", ".vhdr"), QString(), opts) == BIDSPath());
    QVERIFY(QDir(refusedDir.path()).isEmpty());

    // EDF: copied as a single file; events without a trial type get theirs from the event id map
    BIDSPath edfSrc(bidsRoot(), "02", "01", "rest", "eeg", "eeg", ".edf");
    BidsRawData edf = BidsRawData::read(edfSrc);
    QVERIFY(edf.isValid());
    edf.events[1].trialType.clear();
    edf.eventIdMap.insert(QStringLiteral("visual"), edf.events[1].value);
    BIDSPath edfDst(tmpDir.path(), "02", "01", "rest", "eeg", "eeg", ".edf");
    QVERIFY(edf.write(edfDst, edfSrc.filePath(), opts) == edfDst);
    QCOMPARE(QFileInfo(edfDst.filePath()).size(), QFileInfo(edfSrc.filePath()).size());
    QFile sidecar(edfDst.sidecarJsonPath().filePath());
    QVERIFY(sidecar.open(QIODevice::ReadOnly));
    // mne-bids writes raw.times[-1]: 1227 / 128 Hz
    QCOMPARE(QJsonDocument::fromJson(sidecar.readAll()).object().value(QStringLiteral("RecordingDuration")).toDouble(), 9.5859375);
    BidsRawData edfBack = BidsRawData::read(edfDst);
    QVERIFY(edfBack.isValid());
    QCOMPARE(edfBack.raw.info.nchan, 25);
    QCOMPARE(edfBack.events.size(), 3);
    QCOMPARE(edfBack.events[1].trialType, QStringLiteral("visual"));
    QCOMPARE(edfBack.events[2].trialType, QStringLiteral("response"));
    QVERIFY(edfBack.raw.info.bads.contains(QStringLiteral("EEG O1")));

    // channels.tsv units other than volts, as mne_bids.read_raw_bids: known ones set the unit, unknown ones keep it
    QFile channelsTsv(edfDst.channelsTsvPath().filePath());
    QVERIFY(channelsTsv.open(QIODevice::ReadOnly));
    QStringList rows = QString::fromUtf8(channelsTsv.readAll()).split('\n');
    channelsTsv.close();
    const QStringList newUnits{"oC", "S", "foo"};
    for (int i = 0; i < newUnits.size(); ++i) {
        QStringList cols = rows[i + 1].split('\t');
        cols[2] = newUnits[i];
        rows[i + 1] = cols.join('\t');
    }
    QVERIFY(channelsTsv.open(QIODevice::WriteOnly | QIODevice::Truncate));
    channelsTsv.write(rows.join('\n').toUtf8());
    channelsTsv.close();
    BidsRawData units = BidsRawData::read(edfDst);
    QVERIFY(units.isValid());
    QCOMPARE(units.raw.info.chs[0].unit, FIFF_UNIT_CEL);
    QCOMPARE(units.raw.info.chs[1].unit, FIFF_UNIT_MHO);
    QCOMPARE(units.raw.info.chs[2].unit, FIFF_UNIT_V);

    // ... and are written back under their BIDS names
    BIDSPath unitsDst(tmpDir.path(), "03", "01", "rest", "eeg", "eeg", ".edf");
    QVERIFY(units.write(unitsDst, edfDst.filePath(), opts) == unitsDst);
    QFile unitsTsv(unitsDst.channelsTsvPath().filePath());
    QVERIFY(unitsTsv.open(QIODevice::ReadOnly));
    const QStringList writtenRows = QString::fromUtf8(unitsTsv.readAll()).split('\n');
    QCOMPARE(writtenRows[1].split('\t')[2], QStringLiteral("oC"));
    QCOMPARE(writtenRows[2].split('\t')[2], QStringLiteral("S"));
    QCOMPARE(writtenRows[3].split('\t')[2], QStringLiteral("V")); // the unknown unit left the samples' SI unit
}

//=============================================================================================================

void TestBids::testBrainVisionReaderMatchesPython()
{
    // Six channels, four samples; raw values -7 .. 16 channel by channel. Units cover voltage (EEG), a unit
    // MNE-Python scales by 1 (ARU), micro-Siemens and a channel with neither resolution nor unit.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QStringList channels{QStringLiteral("Fp1,,0.1,\u00B5V"), QStringLiteral("Cz,,0.5,mV"), QStringLiteral("Resp,,2,ARU"),
                               QStringLiteral("GSR,,0.25,\u00B5S"), QStringLiteral("Bare,,,"), QStringLiteral("A\\1B,,1,\u00B5V")};
    const auto write = [&](const QString& base, const QString& format, const QString& orientation) {
        QFile vhdr(dir.filePath(base + ".vhdr"));
        QVERIFY(vhdr.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&vhdr);
        out << "Brain Vision Data Exchange Header File Version 1.0\n[Common Infos]\nCodepage=UTF-8\n"
            << "DataFile=" << base << ".eeg\nMarkerFile=" << base << ".vmrk\nDataFormat=BINARY\n"
            << "DataOrientation=" << orientation << "\nNumberOfChannels=6\nSamplingInterval=2000\n"
            << "[Binary Infos]\nBinaryFormat=" << format << "\n[Channel Infos]\n";
        for (int k = 0; k < channels.size(); ++k) {
            out << "Ch" << (k + 1) << "=" << channels[k] << "\n";
        }
        vhdr.close();
        QByteArray data;
        QDataStream bin(&data, QIODevice::WriteOnly);
        bin.setByteOrder(QDataStream::LittleEndian);
        bin.setFloatingPointPrecision(QDataStream::SinglePrecision);
        for (int outer = 0; outer < (orientation == "VECTORIZED" ? 6 : 4); ++outer) {
            for (int inner = 0; inner < (orientation == "VECTORIZED" ? 4 : 6); ++inner) {
                const int value = (orientation == "VECTORIZED" ? outer * 4 + inner : inner * 4 + outer) - 7;
                if (format == "INT_16") {
                    bin << qint16(value);
                } else {
                    bin << float(value * 1.5f);
                }
            }
        }
        QFile eeg(dir.filePath(base + ".eeg"));
        QVERIFY(eeg.open(QIODevice::WriteOnly));
        eeg.write(data);
        eeg.close();
        QFile vmrk(dir.filePath(base + ".vmrk"));
        QVERIFY(vmrk.open(QIODevice::WriteOnly | QIODevice::Text));
        vmrk.write("Brain Vision Data Exchange Marker File, Version 1.0\n[Marker Infos]\n"
                   "Mk1=New Segment,,1,1,0,20260101120000000000\nMk2=Stimulus,S\\1 7,3,2,0\n");
        vmrk.close();
    };
    write(QStringLiteral("mux16"), QStringLiteral("INT_16"), QStringLiteral("MULTIPLEXED"));
    write(QStringLiteral("vec32"), QStringLiteral("IEEE_FLOAT_32"), QStringLiteral("VECTORIZED"));

    // mne.io.read_raw_brainvision(...).get_data() for mux16; vec32 is the same times 1.5
    Eigen::MatrixXd expected(6, 4);
    expected << -7e-7, -6e-7, -5e-7, -4e-7,
        -1.5e-3, -1e-3, -5e-4, 0.0,
        2.0, 4.0, 6.0, 8.0,
        1.25e-6, 1.5e-6, 1.75e-6, 2e-6,
        9e-6, 1e-5, 1.1e-5, 1.2e-5,
        1.3e-5, 1.4e-5, 1.5e-5, 1.6e-5;
    const QList<int> kinds{FIFFV_EEG_CH, FIFFV_EEG_CH, FIFFV_MISC_CH, FIFFV_MISC_CH, FIFFV_EEG_CH, FIFFV_EEG_CH};
    for (const auto& [base, factor] : {std::pair<QString, double>{QStringLiteral("mux16"), 1.0}, {QStringLiteral("vec32"), 1.5}}) {
        BrainVisionReader reader;
        QVERIFY(reader.open(dir.filePath(base + ".vhdr")));
        QCOMPARE(reader.getFrequency(), 500.0f);
        QCOMPARE(reader.getSampleCount(), 4L);
        QCOMPARE(reader.toFiffRawData().last_samp, 3);
        const FIFFLIB::FiffInfo info = reader.getInfo();
        QCOMPARE(info.ch_names.last(), QStringLiteral("A,B"));
        for (int k = 0; k < 6; ++k) {
            QVERIFY2(info.chs[k].kind == kinds[k], qPrintable(info.ch_names[k]));
        }
        const Eigen::MatrixXd data = reader.readRawSegment(0, 4).cast<double>();
        QVERIFY2((data - factor * expected).cwiseAbs().maxCoeff() <= 1e-6 * (factor * expected).cwiseAbs().maxCoeff(),
                 qPrintable(base));
        for (int k = 0; k < 6; ++k) {
            QVERIFY2((data.row(k) - factor * expected.row(k)).cwiseAbs().maxCoeff() <= 1e-6 * (factor * expected.row(k)).cwiseAbs().maxCoeff(),
                     qPrintable(base + " " + info.ch_names[k]));
        }
        // Markers: 1-based positions in the file, \1 decodes to a comma
        const QVector<BrainVisionMarker> markers = reader.getMarkers();
        QCOMPARE(markers.size(), 2);
        QCOMPARE(markers[1].description, QStringLiteral("S, 7"));
        QCOMPARE(markers[1].position, 2L);
        QCOMPARE(markers[1].duration, 2L);
        QCOMPARE(markers[0].date, QDateTime(QDate(2026, 1, 1), QTime(12, 0)));
    }

    // ASCII data is refused rather than misread as INT_16 binary
    QFile mux(dir.filePath(QStringLiteral("mux16.vhdr")));
    QVERIFY(mux.open(QIODevice::ReadOnly | QIODevice::Text));
    QString header = QString::fromUtf8(mux.readAll());
    mux.close();
    QFile ascii(dir.filePath(QStringLiteral("ascii.vhdr")));
    QVERIFY(ascii.open(QIODevice::WriteOnly | QIODevice::Text));
    ascii.write(header.replace(QStringLiteral("DataFormat=BINARY"), QStringLiteral("DataFormat=ASCII")).toUtf8());
    ascii.close();
    BrainVisionReader asciiReader;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Only BINARY data is supported"));
    QVERIFY(!asciiReader.open(ascii.fileName()));
}

//=============================================================================================================

void TestBids::cleanupTestCase()
{
}

//=============================================================================================================

void TestBids::testPathSettersGetters()
{
    BIDSPath path;
    path.setRoot("/tmp/bids");
    path.setSubject("02");
    path.setSession("03");
    path.setTask("motor");
    path.setAcquisition("acq01");
    path.setRun("01");
    path.setProcessing("sss");
    path.setSpace("MNI");
    path.setRecording("ecog");
    path.setSplit("01");
    path.setDescription("filtered");
    path.setDatatype("meg");
    path.setSuffix("meg");
    path.setExtension(".fif");

    QCOMPARE(path.root(), QStringLiteral("/tmp/bids"));
    QCOMPARE(path.subject(), QStringLiteral("02"));
    QCOMPARE(path.session(), QStringLiteral("03"));
    QCOMPARE(path.task(), QStringLiteral("motor"));
    QCOMPARE(path.acquisition(), QStringLiteral("acq01"));
    QCOMPARE(path.run(), QStringLiteral("01"));
    QCOMPARE(path.processing(), QStringLiteral("sss"));
    QCOMPARE(path.space(), QStringLiteral("MNI"));
    QCOMPARE(path.recording(), QStringLiteral("ecog"));
    QCOMPARE(path.split(), QStringLiteral("01"));
    QCOMPARE(path.description(), QStringLiteral("filtered"));
    QCOMPARE(path.datatype(), QStringLiteral("meg"));
    QCOMPARE(path.suffix(), QStringLiteral("meg"));
    QCOMPARE(path.extension(), QStringLiteral(".fif"));

    // mne_bids.BIDSPath(...).basename with the same entities (space CTF, check=False)
    path.setSpace("CTF");
    QCOMPARE(path.basename(), QStringLiteral("sub-02_ses-03_task-motor_acq-acq01_run-01_proc-sss_space-CTF_recording-ecog_split-01_desc-filtered_meg.fif"));

    // match(): every set entity filters, unset ones match anything, files are found below the root
    QTemporaryDir dir;
    for (const QString& file : {QStringLiteral("sub-01/ses-01/ieeg/sub-01_ses-01_task-rest_run-01_ieeg.vhdr"),
                                QStringLiteral("sub-01/ses-01/ieeg/sub-01_ses-01_task-motor_ieeg.vhdr"),
                                QStringLiteral("sub-01/ses-01/ieeg/sub-01_ses-01_task-rest_run-01_ieeg.eeg"),
                                QStringLiteral("sub-02/ses-01/ieeg/sub-02_ses-01_task-rest_ieeg.vhdr")}) {
        QVERIFY(QDir(dir.path()).mkpath(QFileInfo(file).path()));
        QFile f(dir.filePath(file));
        QVERIFY(f.open(QIODevice::WriteOnly));
    }
    const auto describe = [](const QList<BIDSPath>& paths) {
        QStringList out;
        for (const BIDSPath& p : paths) {
            out << QStringList{p.subject(), p.session(), p.task(), p.run(), p.datatype(), p.suffix(), p.extension()}.join(u'|');
        }
        return out;
    };
    // Expected: mne_bids.BIDSPath(root=..., **entities).match()
    QCOMPARE(describe(BIDSPath(dir.path(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr").match()),
             QStringList{"01|01|rest|01|ieeg|ieeg|.vhdr"});
    QCOMPARE(describe(BIDSPath(dir.path(), "01", "01", QString(), "ieeg", "ieeg", ".vhdr").match()),
             (QStringList{"01|01|motor||ieeg|ieeg|.vhdr", "01|01|rest|01|ieeg|ieeg|.vhdr"}));
    QCOMPARE(describe(BIDSPath(dir.path(), QString(), QString(), "rest", "ieeg", "ieeg", ".vhdr").match()),
             (QStringList{"01|01|rest|01|ieeg|ieeg|.vhdr", "02|01|rest||ieeg|ieeg|.vhdr"}));
}

//=============================================================================================================

void TestBids::testPathValidation()
{
    QVERIFY(BIDSPath::isValidEntityValue("abc"));
    QVERIFY(BIDSPath::isValidEntityValue("abc123"));
    QVERIFY(BIDSPath::isValidEntityValue("")); // empty is valid per implementation
    QVERIFY(!BIDSPath::isValidEntityValue("abc-def"));
    QVERIFY(!BIDSPath::isValidEntityValue("abc_def"));
    QVERIFY(!BIDSPath::isValidEntityValue("abc/def"));
}

//=============================================================================================================

void TestBids::testPathEquality()
{
    BIDSPath a(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    BIDSPath b(a); // copy constructor

    QCOMPARE(b.root(), a.root());
    QCOMPARE(b.subject(), a.subject());
    QCOMPARE(b.session(), a.session());
    QCOMPARE(b.task(), a.task());
    QCOMPARE(b.datatype(), a.datatype());
    QCOMPARE(b.suffix(), a.suffix());
    QCOMPARE(b.extension(), a.extension());
    QCOMPARE(b.basename(), a.basename());
}

//=============================================================================================================

void TestBids::testBidsGlobalBuildInfo()
{
    const char* dt = BIDSLIB::buildDateTime();
    QVERIFY(dt != nullptr);
    const char* h = BIDSLIB::buildHash();
    QVERIFY(h != nullptr);
    const char* hl = BIDSLIB::buildHashLong();
    QVERIFY(hl != nullptr);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestBids)
#include "test_bids.moc"
