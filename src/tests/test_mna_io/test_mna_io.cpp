//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mna_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for MNA I/O, Project, and data model serialization (JSON/CBOR).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mna/mna_io.h>
#include <mne/mna/mna_project.h>
#include <mne/mna/mna_node.h>
#include <mne/mna/mna_port.h>
#include <mne/mna/mna_types.h>

#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCborArray>
#include <QCborMap>
#include <QCborValue>
#include <QDateTime>
#include <QFile>
#include <QtTest>
#include <QObject>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
/**
 * DECLARE CLASS TestMnaIo
 *
 * @brief The TestMnaIo class provides tests for MNA I/O and data model serialization.
 */
class TestMnaIo : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // MnaProject construction
    void testProjectDefault();
    void testProjectFieldAssignment();

    // MnaSubject / MnaSession / MnaRecording / MnaFileRef
    void testSubjectConstruction();
    void testSessionConstruction();
    void testRecordingConstruction();
    void testFileRefConstruction();
    void testFileRoleEnumRoundTrip();

    // MnaNode (pipeline node)
    void testNodeConstruction();

    // JSON serialization
    void testProjectJsonRoundTrip();
    void testSubjectJsonRoundTrip();
    void testSessionJsonRoundTrip();
    void testRecordingJsonRoundTrip();
    void testFileRefJsonRoundTrip();
    void testNodeJsonRoundTrip();

    // CBOR serialization
    void testProjectCborRoundTrip();

    // File I/O — .mna (JSON) and .mnx (CBOR)
    void testWriteReadJsonFile();
    void testWriteReadCborFile();
    void testMnaIoStaticRead();
    void testMnaIoStaticWrite();

    // Edge cases
    void testEmptyProject();
    void testReadNonexistentFile();
    void testWriteInvalidPath();

    // Compatibility with files written and read by Qt
    void testCborFileIsReadableByQt();
    void testReadsCborWrittenByQt();
    void testEmbeddedDataJsonIsStandardBase64();

    void cleanupTestCase();

private:
    MnaProject createSampleProject();
    std::filesystem::path tempPath(const QString& name) const
    {
        return path_from_utf8(m_tempDir.filePath(name).toStdString());
    }
    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestMnaIo::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================

MnaProject TestMnaIo::createSampleProject()
{
    MnaProject project;
    project.name = "TestProject";
    project.description = "A test MNA project for unit testing";
    project.mna_version = MnaProject::CURRENT_SCHEMA_VERSION;
    project.created = mne::UtcTime(std::chrono::milliseconds(1'800'000'000'000));
    project.modified = mne::UtcTime(std::chrono::milliseconds(1'800'000'000'000));

    // Add a subject with a session and recording
    MnaFileRef fileRef;
    fileRef.role = MnaFileRole::Raw;
    fileRef.path = "sub-01/ses-01/meg/sub-01_task-rest_meg.fif";
    fileRef.format = "fif";
    fileRef.size_bytes = 123456;

    MnaRecording recording;
    recording.id = "rec-01";
    recording.files.push_back(fileRef);

    MnaSession session;
    session.id = "ses-01";
    session.recordings.push_back(recording);

    MnaSubject subject;
    subject.id = "sub-01";
    subject.freesurfer_dir = "/data/subjects/sub-01";
    subject.sessions.push_back(session);

    project.subjects.push_back(subject);

    // Add a pipeline node
    MnaNode node;
    node.id = "step-01";
    node.op_type = "mne_compute_raw_inverse";
    node.tool_version = "2.2.0";
    node.attributes.insert("method", "dSPM");
    node.attributes.insert("snr", 3.0);

    MnaPort inPort;
    inPort.name = "raw";
    inPort.data_kind = MnaDataKind::Matrix;
    inPort.direction = MnaPortDir::Input;
    node.inputs.push_back(inPort);

    MnaPort outPort;
    outPort.name = "stc";
    outPort.data_kind = MnaDataKind::Matrix;
    outPort.direction = MnaPortDir::Output;
    node.outputs.push_back(outPort);

    node.executed_at = mne::UtcTime(std::chrono::milliseconds(1'800'000'000'000));

    project.pipeline.push_back(node);

    return project;
}

//=============================================================================================================

void TestMnaIo::testProjectDefault()
{
    MnaProject project;
    QVERIFY(project.name.empty());
    QVERIFY(project.subjects.empty());
    QVERIFY(project.pipeline.empty());
}

//=============================================================================================================

void TestMnaIo::testProjectFieldAssignment()
{
    MnaProject project;
    project.name = "MyProject";
    project.description = "Test description";
    QCOMPARE(project.name, std::string("MyProject"));
    QCOMPARE(project.description, std::string("Test description"));
}

//=============================================================================================================

void TestMnaIo::testSubjectConstruction()
{
    MnaSubject subject;
    subject.id = "sub-01";
    subject.freesurfer_dir = "/path/to/fs";
    QCOMPARE(subject.id, std::string("sub-01"));
    QVERIFY(subject.sessions.empty());
}

//=============================================================================================================

void TestMnaIo::testSessionConstruction()
{
    MnaSession session;
    session.id = "ses-01";
    QCOMPARE(session.id, std::string("ses-01"));
    QVERIFY(session.recordings.empty());
}

//=============================================================================================================

void TestMnaIo::testRecordingConstruction()
{
    MnaRecording recording;
    recording.id = "rec-01";
    QCOMPARE(recording.id, std::string("rec-01"));
    QVERIFY(recording.files.empty());
}

//=============================================================================================================

void TestMnaIo::testFileRefConstruction()
{
    MnaFileRef ref;
    ref.role = MnaFileRole::Forward;
    ref.path = "test.fif";
    ref.sha256 = "abc123";
    ref.format = "fif";
    ref.size_bytes = 42;

    QCOMPARE(ref.role, MnaFileRole::Forward);
    QCOMPARE(ref.path, std::string("test.fif"));
    QCOMPARE(ref.size_bytes, std::int64_t(42));
    QVERIFY(!ref.embedded);
}

//=============================================================================================================

void TestMnaIo::testFileRoleEnumRoundTrip()
{
    // Test enum ↔ string conversion
    const std::vector<MnaFileRole> roles = {
        MnaFileRole::Raw, MnaFileRole::Forward, MnaFileRole::Inverse,
        MnaFileRole::Covariance, MnaFileRole::SourceEstimate,
        MnaFileRole::Bem, MnaFileRole::Surface, MnaFileRole::Annotation,
        MnaFileRole::Custom};

    for (MnaFileRole role : roles) {
        const std::string_view str = to_string(role);
        QVERIFY(!str.empty());
        MnaFileRole restored = file_role_from_string(str);
        QCOMPARE(restored, role);
    }
}

//=============================================================================================================

void TestMnaIo::testNodeConstruction()
{
    MnaNode node;
    node.id = "step-01";
    node.op_type = "filter";
    node.tool_version = "1.0";
    node.attributes.insert("lowpass", 40.0);

    MnaPort inPort;
    inPort.name = "raw";
    inPort.data_kind = MnaDataKind::Matrix;
    inPort.direction = MnaPortDir::Input;
    node.inputs.push_back(inPort);

    MnaPort outPort;
    outPort.name = "filtered";
    outPort.data_kind = MnaDataKind::Matrix;
    outPort.direction = MnaPortDir::Output;
    node.outputs.push_back(outPort);

    QCOMPARE(node.id, std::string("step-01"));
    QCOMPARE(node.attributes["lowpass"].to_double(), 40.0);
    QCOMPARE(node.inputs.size(), std::size_t(1));
    QCOMPARE(node.outputs.size(), std::size_t(1));
}

//=============================================================================================================

void TestMnaIo::testProjectJsonRoundTrip()
{
    MnaProject original = createSampleProject();
    JsonObject json = original.to_json();
    MnaProject restored = MnaProject::from_json(json);

    QCOMPARE(restored.name, original.name);
    QCOMPARE(restored.description, original.description);
    QCOMPARE(restored.mna_version, original.mna_version);
    QCOMPARE(restored.subjects.size(), original.subjects.size());
    QCOMPARE(restored.pipeline.size(), original.pipeline.size());

    // Verify nested data survived
    QCOMPARE(restored.subjects[0].id, original.subjects[0].id);
    QCOMPARE(restored.subjects[0].sessions[0].id, original.subjects[0].sessions[0].id);
    QCOMPARE(restored.pipeline[0].op_type, original.pipeline[0].op_type);
}

//=============================================================================================================

void TestMnaIo::testSubjectJsonRoundTrip()
{
    MnaSubject original;
    original.id = "sub-02";
    original.freesurfer_dir = "/data/fs/sub-02";

    JsonObject json = original.to_json();
    MnaSubject restored = MnaSubject::from_json(json);

    QCOMPARE(restored.id, original.id);
    QCOMPARE(restored.freesurfer_dir, original.freesurfer_dir);
}

//=============================================================================================================

void TestMnaIo::testSessionJsonRoundTrip()
{
    MnaSession original;
    original.id = "ses-03";

    MnaRecording rec;
    rec.id = "rec-01";
    original.recordings.push_back(rec);

    JsonObject json = original.to_json();
    MnaSession restored = MnaSession::from_json(json);

    QCOMPARE(restored.id, original.id);
    QCOMPARE(restored.recordings.size(), std::size_t(1));
}

//=============================================================================================================

void TestMnaIo::testRecordingJsonRoundTrip()
{
    MnaRecording original;
    original.id = "rec-02";

    MnaFileRef ref;
    ref.role = MnaFileRole::Covariance;
    ref.path = "cov.fif";
    original.files.push_back(ref);

    JsonObject json = original.to_json();
    MnaRecording restored = MnaRecording::from_json(json);

    QCOMPARE(restored.id, original.id);
    QCOMPARE(restored.files.size(), std::size_t(1));
    QCOMPARE(restored.files[0].role, MnaFileRole::Covariance);
}

//=============================================================================================================

void TestMnaIo::testFileRefJsonRoundTrip()
{
    MnaFileRef original;
    original.role = MnaFileRole::SourceEstimate;
    original.path = "stc.stc";
    original.sha256 = "deadbeef";
    original.format = "stc";
    original.size_bytes = 999999;
    original.embedded = false;

    JsonObject json = original.to_json();
    MnaFileRef restored = MnaFileRef::from_json(json);

    QCOMPARE(restored.role, original.role);
    QCOMPARE(restored.path, original.path);
    QCOMPARE(restored.sha256, original.sha256);
    QCOMPARE(restored.size_bytes, original.size_bytes);
}

//=============================================================================================================

void TestMnaIo::testNodeJsonRoundTrip()
{
    MnaNode original;
    original.id = "step-02";
    original.op_type = "csd_compute";
    original.tool_version = "2.2.0";
    original.attributes.insert("fmin", 7.0);
    original.attributes.insert("fmax", 30.0);

    MnaPort in1;
    in1.name = "epochs";
    in1.data_kind = MnaDataKind::Epochs;
    in1.direction = MnaPortDir::Input;
    original.inputs.push_back(in1);

    MnaPort in2;
    in2.name = "noise_cov";
    in2.data_kind = MnaDataKind::Matrix;
    in2.direction = MnaPortDir::Input;
    original.inputs.push_back(in2);

    MnaPort out;
    out.name = "csd";
    out.data_kind = MnaDataKind::Custom;
    out.direction = MnaPortDir::Output;
    original.outputs.push_back(out);

    original.executed_at = mne::UtcTime(std::chrono::milliseconds(1'800'000'000'000));

    JsonObject json = original.to_json();
    MnaNode restored = MnaNode::from_json(json);

    QCOMPARE(restored.id, original.id);
    QCOMPARE(restored.op_type, original.op_type);
    QCOMPARE(restored.inputs.size(), std::size_t(2));
    QCOMPARE(restored.outputs.size(), std::size_t(1));
    QCOMPARE(restored.attributes["fmin"].to_double(), 7.0);
}

//=============================================================================================================

void TestMnaIo::testProjectCborRoundTrip()
{
    MnaProject original = createSampleProject();
    CborMap cbor = original.to_cbor();
    MnaProject restored = MnaProject::from_cbor(cbor);

    QCOMPARE(restored.name, original.name);
    QCOMPARE(restored.description, original.description);
    QCOMPARE(restored.subjects.size(), original.subjects.size());
    QCOMPARE(restored.pipeline.size(), original.pipeline.size());
}

//=============================================================================================================

void TestMnaIo::testWriteReadJsonFile()
{
    MnaProject original = createSampleProject();
    const std::filesystem::path filePath = tempPath("test_project.mna");

    QVERIFY(original.write(filePath));
    QVERIFY(std::filesystem::exists(filePath));

    const MnaProject restored = MnaProject::read(filePath).value();
    QCOMPARE(restored.name, original.name);
    QCOMPARE(restored.subjects.size(), original.subjects.size());
}

//=============================================================================================================

void TestMnaIo::testWriteReadCborFile()
{
    MnaProject original = createSampleProject();
    const std::filesystem::path filePath = tempPath("test_project.mnx");

    QVERIFY(original.write(filePath));
    QVERIFY(std::filesystem::exists(filePath));

    const MnaProject restored = MnaProject::read(filePath).value();
    QCOMPARE(restored.name, original.name);
    QCOMPARE(restored.subjects.size(), original.subjects.size());
}

//=============================================================================================================

void TestMnaIo::testMnaIoStaticRead()
{
    // Write with MnaProject, read with MnaIO
    MnaProject original = createSampleProject();
    const std::filesystem::path filePath = tempPath("test_io_read.mna");
    QVERIFY(original.write(filePath));

    const MnaProject restored = MnaIO::read(filePath).value();
    QCOMPARE(restored.name, original.name);
}

//=============================================================================================================

void TestMnaIo::testMnaIoStaticWrite()
{
    // Write with MnaIO, read with MnaProject
    MnaProject original = createSampleProject();
    const std::filesystem::path filePath = tempPath("test_io_write.mna");

    QVERIFY(MnaIO::write(original, filePath));

    const MnaProject restored = MnaProject::read(filePath).value();
    QCOMPARE(restored.name, original.name);
}

//=============================================================================================================

void TestMnaIo::testEmptyProject()
{
    MnaProject empty;
    JsonObject json = empty.to_json();
    MnaProject restored = MnaProject::from_json(json);

    QVERIFY(restored.subjects.empty());
    QVERIFY(restored.pipeline.empty());
}

//=============================================================================================================

void TestMnaIo::testReadNonexistentFile()
{
    const Expected<MnaProject> project = MnaIO::read("/nonexistent/path/file.mna");
    QVERIFY(!project);
    QCOMPARE(project.error().code, ErrorCode::Io);
}

//=============================================================================================================

void TestMnaIo::testWriteInvalidPath()
{
    MnaProject project = createSampleProject();
    // Writing to an invalid path should return false
    QVERIFY(!MnaIO::write(project, "/nonexistent/directory/file.mna"));
}

//=============================================================================================================

void TestMnaIo::testCborFileIsReadableByQt()
{
    MnaProject project = createSampleProject();
    project.created = parse_iso8601("2026-11-15T09:30:05.250Z");
    MnaFileRef& ref = project.subjects[0].sessions[0].recordings[0].files[0];
    ref.embedded = true;
    ref.data = {std::byte{0}, std::byte{0xFF}, std::byte{'M'}};
    const std::filesystem::path path = tempPath("qt_reads.mnx");
    QVERIFY(project.write(path));

    QFile file(QString::fromStdString(utf8_from_path(path)));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.read(4), QByteArray("MNX1"));
    QCborParserError error;
    const QCborMap root = QCborValue::fromCbor(file.readAll(), &error).toMap();
    QCOMPARE(error.error, QCborError::NoError);
    QCOMPARE(root[QLatin1String("name")].toString(), QStringLiteral("TestProject"));
    QVERIFY(root[QLatin1String("created")].isDateTime());
    QCOMPARE(root[QLatin1String("created")].toDateTime(), QDateTime::fromString("2026-11-15T09:30:05.250Z", Qt::ISODateWithMs));
    const QCborValue data = root[QLatin1String("subjects")][0][QLatin1String("sessions")][0][QLatin1String("recordings")][0][QLatin1String("files")][0][QLatin1String("data")];
    QVERIFY(data.isByteArray());
    QCOMPARE(data.toByteArray(), QByteArray("\x00\xFFM", 3));
}

//=============================================================================================================

void TestMnaIo::testReadsCborWrittenByQt()
{
    QCborMap file{{QLatin1String("role"), QStringLiteral("raw")}, {QLatin1String("path"), QStringLiteral("raw.fif")}, {QLatin1String("embedded"), true}, {QLatin1String("data"), QByteArray("\x01\x02", 2)}, {QLatin1String("size_bytes"), 2}};
    QCborMap recording{{QLatin1String("id"), QStringLiteral("run-01")}, {QLatin1String("files"), QCborArray{file}}};
    QCborMap session{{QLatin1String("id"), QStringLiteral("ses-01")}, {QLatin1String("recordings"), QCborArray{recording}}};
    QCborMap subject{{QLatin1String("id"), QStringLiteral("sub-01")}, {QLatin1String("sessions"), QCborArray{session}}};
    QCborMap root{{QLatin1String("mna_version"), QStringLiteral("1.0")},
                  {QLatin1String("name"), QStringLiteral("from Qt")},
                  {QLatin1String("created"), QCborValue(QDateTime::fromString("2025-01-02T03:04:05.006Z", Qt::ISODateWithMs))},
                  {QLatin1String("subjects"), QCborArray{subject}},
                  {QLatin1String("future_key"), 42}};
    const std::filesystem::path path = tempPath("qt_wrote.mnx");
    QFile out(QString::fromStdString(utf8_from_path(path)));
    QVERIFY(out.open(QIODevice::WriteOnly));
    out.write("MNX1");
    out.write(QCborValue(root).toCbor());
    out.close();

    const MnaProject project = MnaProject::read(path).value();
    QCOMPARE(project.name, std::string("from Qt"));
    QCOMPARE(project.created, parse_iso8601("2025-01-02T03:04:05.006Z"));
    QCOMPARE(project.extras["future_key"].to_int(), 42);
    const MnaFileRef& ref = project.subjects.at(0).sessions.at(0).recordings.at(0).files.at(0);
    QCOMPARE(ref.path, std::string("raw.fif"));
    QVERIFY(ref.data == (std::vector<std::byte>{std::byte{1}, std::byte{2}}));
}

//=============================================================================================================

void TestMnaIo::testEmbeddedDataJsonIsStandardBase64()
{
    MnaFileRef ref;
    ref.embedded = true;
    for (int i = 0; i < 64; ++i) {
        ref.data.push_back(static_cast<std::byte>(250 + i)); // bytes that differ between base64 and base64url
    }
    const QByteArray qtBytes(reinterpret_cast<const char*>(ref.data.data()), qsizetype(ref.data.size()));
    QCOMPARE(std::string(ref.to_json()["data"].to_string()), qtBytes.toBase64().toStdString());
    QVERIFY(MnaFileRef::from_json(ref.to_json()).data == ref.data);
    QVERIFY(MnaFileRef::from_cbor(ref.to_cbor()).data == ref.data);
}

//=============================================================================================================

void TestMnaIo::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMnaIo)
#include "test_mna_io.moc"
