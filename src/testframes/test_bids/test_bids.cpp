//=============================================================================================================
/**
 * @file     test_bids.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Data-driven tests for the BIDS library.
 *
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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QTemporaryDir>

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
    QString bidsRoot() const;    /**< Path to BIDS test fixtures. */
    QString dataPath() const;    /**< Base path for mne-cpp-test-data. */

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
    return QCoreApplication::applicationDirPath()
           + QStringLiteral("/../resources/data/mne-cpp-test-data/");
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

    for(int i = 0; i < original.size(); ++i) {
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

    for(int i = 0; i < original.size(); ++i) {
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

    for(int i = 0; i < original.size(); ++i) {
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
    QVERIFY2(!written.filePath().isEmpty(), "write() returned empty path");
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
}

//=============================================================================================================

void TestBids::testPathValidation()
{
    QVERIFY(BIDSPath::isValidEntityValue("abc"));
    QVERIFY(BIDSPath::isValidEntityValue("abc123"));
    QVERIFY(BIDSPath::isValidEntityValue(""));   // empty is valid per implementation
    QVERIFY(!BIDSPath::isValidEntityValue("abc-def"));
    QVERIFY(!BIDSPath::isValidEntityValue("abc_def"));
    QVERIFY(!BIDSPath::isValidEntityValue("abc/def"));
}

//=============================================================================================================

void TestBids::testPathEquality()
{
    BIDSPath a(bidsRoot(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr");
    BIDSPath b(a);  // copy constructor

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
