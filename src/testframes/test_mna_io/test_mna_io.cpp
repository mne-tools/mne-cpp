//=============================================================================================================
/**
 * @file     test_mna_io.cpp
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
 * @brief    Tests for MNA I/O, Project, and data model serialization (JSON/CBOR).
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mna/mna_io.h>
#include <mna/mna_project.h>
#include <mna/mna_node.h>
#include <mna/mna_port.h>
#include <mna/mna_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QTemporaryFile>
#include <QTemporaryDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

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

    void cleanupTestCase();

private:
    MnaProject createSampleProject();
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
    project.mnaVersion = MnaProject::CURRENT_SCHEMA_VERSION;
    project.created = QDateTime::currentDateTimeUtc();
    project.modified = QDateTime::currentDateTimeUtc();

    // Add a subject with a session and recording
    MnaFileRef fileRef;
    fileRef.role = MnaFileRole::Raw;
    fileRef.path = "sub-01/ses-01/meg/sub-01_task-rest_meg.fif";
    fileRef.format = "fif";
    fileRef.sizeBytes = 123456;

    MnaRecording recording;
    recording.id = "rec-01";
    recording.files.append(fileRef);

    MnaSession session;
    session.id = "ses-01";
    session.recordings.append(recording);

    MnaSubject subject;
    subject.id = "sub-01";
    subject.freeSurferDir = "/data/subjects/sub-01";
    subject.sessions.append(session);

    project.subjects.append(subject);

    // Add a pipeline node
    MnaNode node;
    node.id = "step-01";
    node.opType = "mne_compute_raw_inverse";
    node.toolVersion = "2.2.0";
    node.attributes.insert("method", "dSPM");
    node.attributes.insert("snr", 3.0);

    MnaPort inPort;
    inPort.name = "raw";
    inPort.dataKind = MnaDataKind::Matrix;
    inPort.direction = MnaPortDir::Input;
    node.inputs.append(inPort);

    MnaPort outPort;
    outPort.name = "stc";
    outPort.dataKind = MnaDataKind::Matrix;
    outPort.direction = MnaPortDir::Output;
    node.outputs.append(outPort);

    node.executedAt = QDateTime::currentDateTimeUtc();

    project.pipeline.append(node);

    return project;
}

//=============================================================================================================

void TestMnaIo::testProjectDefault()
{
    MnaProject project;
    QVERIFY(project.name.isEmpty());
    QVERIFY(project.subjects.isEmpty());
    QVERIFY(project.pipeline.isEmpty());
}

//=============================================================================================================

void TestMnaIo::testProjectFieldAssignment()
{
    MnaProject project;
    project.name = "MyProject";
    project.description = "Test description";
    QCOMPARE(project.name, QString("MyProject"));
    QCOMPARE(project.description, QString("Test description"));
}

//=============================================================================================================

void TestMnaIo::testSubjectConstruction()
{
    MnaSubject subject;
    subject.id = "sub-01";
    subject.freeSurferDir = "/path/to/fs";
    QCOMPARE(subject.id, QString("sub-01"));
    QVERIFY(subject.sessions.isEmpty());
}

//=============================================================================================================

void TestMnaIo::testSessionConstruction()
{
    MnaSession session;
    session.id = "ses-01";
    QCOMPARE(session.id, QString("ses-01"));
    QVERIFY(session.recordings.isEmpty());
}

//=============================================================================================================

void TestMnaIo::testRecordingConstruction()
{
    MnaRecording recording;
    recording.id = "rec-01";
    QCOMPARE(recording.id, QString("rec-01"));
    QVERIFY(recording.files.isEmpty());
}

//=============================================================================================================

void TestMnaIo::testFileRefConstruction()
{
    MnaFileRef ref;
    ref.role = MnaFileRole::Forward;
    ref.path = "test.fif";
    ref.sha256 = "abc123";
    ref.format = "fif";
    ref.sizeBytes = 42;

    QCOMPARE(ref.role, MnaFileRole::Forward);
    QCOMPARE(ref.path, QString("test.fif"));
    QCOMPARE(ref.sizeBytes, static_cast<qint64>(42));
    QVERIFY(!ref.embedded);
}

//=============================================================================================================

void TestMnaIo::testFileRoleEnumRoundTrip()
{
    // Test enum ↔ string conversion
    QList<MnaFileRole> roles = {
        MnaFileRole::Raw, MnaFileRole::Forward, MnaFileRole::Inverse,
        MnaFileRole::Covariance, MnaFileRole::SourceEstimate,
        MnaFileRole::Bem, MnaFileRole::Surface, MnaFileRole::Annotation,
        MnaFileRole::Custom
    };

    for (MnaFileRole role : roles) {
        QString str = mnaFileRoleToString(role);
        QVERIFY(!str.isEmpty());
        MnaFileRole restored = mnaFileRoleFromString(str);
        QCOMPARE(restored, role);
    }
}

//=============================================================================================================

void TestMnaIo::testNodeConstruction()
{
    MnaNode node;
    node.id = "step-01";
    node.opType = "filter";
    node.toolVersion = "1.0";
    node.attributes.insert("lowpass", 40.0);

    MnaPort inPort;
    inPort.name = "raw";
    inPort.dataKind = MnaDataKind::Matrix;
    inPort.direction = MnaPortDir::Input;
    node.inputs.append(inPort);

    MnaPort outPort;
    outPort.name = "filtered";
    outPort.dataKind = MnaDataKind::Matrix;
    outPort.direction = MnaPortDir::Output;
    node.outputs.append(outPort);

    QCOMPARE(node.id, QString("step-01"));
    QCOMPARE(node.attributes.value("lowpass").toDouble(), 40.0);
    QCOMPARE(node.inputs.size(), 1);
    QCOMPARE(node.outputs.size(), 1);
}

//=============================================================================================================

void TestMnaIo::testProjectJsonRoundTrip()
{
    MnaProject original = createSampleProject();
    QJsonObject json = original.toJson();
    MnaProject restored = MnaProject::fromJson(json);

    QCOMPARE(restored.name, original.name);
    QCOMPARE(restored.description, original.description);
    QCOMPARE(restored.mnaVersion, original.mnaVersion);
    QCOMPARE(restored.subjects.size(), original.subjects.size());
    QCOMPARE(restored.pipeline.size(), original.pipeline.size());

    // Verify nested data survived
    QCOMPARE(restored.subjects[0].id, original.subjects[0].id);
    QCOMPARE(restored.subjects[0].sessions[0].id, original.subjects[0].sessions[0].id);
    QCOMPARE(restored.pipeline[0].opType, original.pipeline[0].opType);
}

//=============================================================================================================

void TestMnaIo::testSubjectJsonRoundTrip()
{
    MnaSubject original;
    original.id = "sub-02";
    original.freeSurferDir = "/data/fs/sub-02";

    QJsonObject json = original.toJson();
    MnaSubject restored = MnaSubject::fromJson(json);

    QCOMPARE(restored.id, original.id);
    QCOMPARE(restored.freeSurferDir, original.freeSurferDir);
}

//=============================================================================================================

void TestMnaIo::testSessionJsonRoundTrip()
{
    MnaSession original;
    original.id = "ses-03";

    MnaRecording rec;
    rec.id = "rec-01";
    original.recordings.append(rec);

    QJsonObject json = original.toJson();
    MnaSession restored = MnaSession::fromJson(json);

    QCOMPARE(restored.id, original.id);
    QCOMPARE(restored.recordings.size(), 1);
}

//=============================================================================================================

void TestMnaIo::testRecordingJsonRoundTrip()
{
    MnaRecording original;
    original.id = "rec-02";

    MnaFileRef ref;
    ref.role = MnaFileRole::Covariance;
    ref.path = "cov.fif";
    original.files.append(ref);

    QJsonObject json = original.toJson();
    MnaRecording restored = MnaRecording::fromJson(json);

    QCOMPARE(restored.id, original.id);
    QCOMPARE(restored.files.size(), 1);
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
    original.sizeBytes = 999999;
    original.embedded = false;

    QJsonObject json = original.toJson();
    MnaFileRef restored = MnaFileRef::fromJson(json);

    QCOMPARE(restored.role, original.role);
    QCOMPARE(restored.path, original.path);
    QCOMPARE(restored.sha256, original.sha256);
    QCOMPARE(restored.sizeBytes, original.sizeBytes);
}

//=============================================================================================================

void TestMnaIo::testNodeJsonRoundTrip()
{
    MnaNode original;
    original.id = "step-02";
    original.opType = "csd_compute";
    original.toolVersion = "2.2.0";
    original.attributes.insert("fmin", 7.0);
    original.attributes.insert("fmax", 30.0);

    MnaPort in1;
    in1.name = "epochs";
    in1.dataKind = MnaDataKind::Epochs;
    in1.direction = MnaPortDir::Input;
    original.inputs.append(in1);

    MnaPort in2;
    in2.name = "noise_cov";
    in2.dataKind = MnaDataKind::Matrix;
    in2.direction = MnaPortDir::Input;
    original.inputs.append(in2);

    MnaPort out;
    out.name = "csd";
    out.dataKind = MnaDataKind::Custom;
    out.direction = MnaPortDir::Output;
    original.outputs.append(out);

    original.executedAt = QDateTime::currentDateTimeUtc();

    QJsonObject json = original.toJson();
    MnaNode restored = MnaNode::fromJson(json);

    QCOMPARE(restored.id, original.id);
    QCOMPARE(restored.opType, original.opType);
    QCOMPARE(restored.inputs.size(), 2);
    QCOMPARE(restored.outputs.size(), 1);
    QCOMPARE(restored.attributes.value("fmin").toDouble(), 7.0);
}

//=============================================================================================================

void TestMnaIo::testProjectCborRoundTrip()
{
    MnaProject original = createSampleProject();
    QCborMap cbor = original.toCbor();
    MnaProject restored = MnaProject::fromCbor(cbor);

    QCOMPARE(restored.name, original.name);
    QCOMPARE(restored.description, original.description);
    QCOMPARE(restored.subjects.size(), original.subjects.size());
    QCOMPARE(restored.pipeline.size(), original.pipeline.size());
}

//=============================================================================================================

void TestMnaIo::testWriteReadJsonFile()
{
    MnaProject original = createSampleProject();
    QString filePath = m_tempDir.filePath("test_project.mna");

    QVERIFY(MnaProject::write(original, filePath));
    QVERIFY(QFile::exists(filePath));

    MnaProject restored = MnaProject::read(filePath);
    QCOMPARE(restored.name, original.name);
    QCOMPARE(restored.subjects.size(), original.subjects.size());
}

//=============================================================================================================

void TestMnaIo::testWriteReadCborFile()
{
    MnaProject original = createSampleProject();
    QString filePath = m_tempDir.filePath("test_project.mnx");

    QVERIFY(MnaProject::write(original, filePath));
    QVERIFY(QFile::exists(filePath));

    MnaProject restored = MnaProject::read(filePath);
    QCOMPARE(restored.name, original.name);
    QCOMPARE(restored.subjects.size(), original.subjects.size());
}

//=============================================================================================================

void TestMnaIo::testMnaIoStaticRead()
{
    // Write with MnaProject, read with MnaIO
    MnaProject original = createSampleProject();
    QString filePath = m_tempDir.filePath("test_io_read.mna");
    MnaProject::write(original, filePath);

    MnaProject restored = MnaIO::read(filePath);
    QCOMPARE(restored.name, original.name);
}

//=============================================================================================================

void TestMnaIo::testMnaIoStaticWrite()
{
    // Write with MnaIO, read with MnaProject
    MnaProject original = createSampleProject();
    QString filePath = m_tempDir.filePath("test_io_write.mna");

    QVERIFY(MnaIO::write(original, filePath));

    MnaProject restored = MnaProject::read(filePath);
    QCOMPARE(restored.name, original.name);
}

//=============================================================================================================

void TestMnaIo::testEmptyProject()
{
    MnaProject empty;
    QJsonObject json = empty.toJson();
    MnaProject restored = MnaProject::fromJson(json);

    QVERIFY(restored.subjects.isEmpty());
    QVERIFY(restored.pipeline.isEmpty());
}

//=============================================================================================================

void TestMnaIo::testReadNonexistentFile()
{
    MnaProject project = MnaIO::read("/nonexistent/path/file.mna");
    // Should return an empty project without crashing
    QVERIFY(project.name.isEmpty());
}

//=============================================================================================================

void TestMnaIo::testWriteInvalidPath()
{
    MnaProject project = createSampleProject();
    // Writing to an invalid path should return false
    QVERIFY(!MnaIO::write(project, "/nonexistent/directory/file.mna"));
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
