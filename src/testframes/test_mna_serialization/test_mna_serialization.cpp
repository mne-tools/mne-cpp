//=============================================================================================================
/**
 * @file     test_mna_coverage.cpp
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
 * @brief    Serialization tests for all MNA data model classes: enum round-trips,
 *           embedded file data, MnaScript, MnaVerification, MnaParamBinding,
 *           MnaParamTree, MnaOpSchema, MnaGraph, and edge cases.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mna/mna_io.h>
#include <mna/mna_project.h>
#include <mna/mna_subject.h>
#include <mna/mna_session.h>
#include <mna/mna_recording.h>
#include <mna/mna_file_ref.h>
#include <mna/mna_node.h>
#include <mna/mna_port.h>
#include <mna/mna_script.h>
#include <mna/mna_verification.h>
#include <mna/mna_param_binding.h>
#include <mna/mna_param_tree.h>
#include <mna/mna_op_schema.h>
#include <mna/mna_op_registry.h>
#include <mna/mna_graph.h>
#include <mna/mna_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>
#include <QJsonDocument>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestMnaSerialization
 *
 * @brief Serialization tests for all MNA library data model classes.
 */
class TestMnaSerialization : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // ── MnaFileRole enum ─────────────────────────────────────────────
    void testFileRoleToString_data();
    void testFileRoleToString();
    void testFileRoleFromString_data();
    void testFileRoleFromString();
    void testFileRoleFromStringUnknown();

    // ── MnaDataKind enum ─────────────────────────────────────────────
    void testDataKindToString_data();
    void testDataKindToString();
    void testDataKindFromString_data();
    void testDataKindFromString();
    void testDataKindFromStringUnknown();

    // ── MnaFileRef with embedded data ────────────────────────────────
    void testFileRefEmbeddedJsonRoundTrip();
    void testFileRefEmbeddedCborRoundTrip();
    void testFileRefEmbeddedLargeData();
    void testFileRefNonEmbeddedHasNoData();

    // ── MnaPort with stream fields ───────────────────────────────────
    void testPortStreamFieldsJson();
    void testPortStreamFieldsCbor();
    void testPortCachedResultFields();

    // ── MnaScript ────────────────────────────────────────────────────
    void testScriptDefaults();
    void testScriptJsonRoundTrip();
    void testScriptCborRoundTrip();
    void testScriptWithAllFields();

    // ── MnaVerificationCheck ─────────────────────────────────────────
    void testVerificationCheckJsonRoundTrip();
    void testVerificationCheckCborRoundTrip();
    void testVerificationCheckWithScript();

    // ── MnaVerificationResult ────────────────────────────────────────
    void testVerificationResultJsonRoundTrip();
    void testVerificationResultCborRoundTrip();

    // ── MnaProvenance ────────────────────────────────────────────────
    void testProvenanceJsonRoundTrip();
    void testProvenanceCborRoundTrip();
    void testProvenanceAllFields();

    // ── MnaVerification (composite) ──────────────────────────────────
    void testVerificationJsonRoundTrip();
    void testVerificationCborRoundTrip();

    // ── MnaNode with IPC/Script/Verification ─────────────────────────
    void testNodeExecModeIpcJson();
    void testNodeExecModeScriptJson();
    void testNodeExecModeCbor_data();
    void testNodeExecModeCbor();
    void testNodeWithVerificationRoundTrip();

    // ── MnaParamBinding ──────────────────────────────────────────────
    void testParamBindingDefaults();
    void testParamBindingJsonRoundTrip();
    void testParamBindingCborRoundTrip();

    // ── MnaParamTree ─────────────────────────────────────────────────
    void testParamTreeSetGet();
    void testParamTreeHasParam();
    void testParamTreeAllPaths();
    void testParamTreeBindings();
    void testParamTreeEvaluateExpression();
    void testParamTreeJsonRoundTrip();

    // ── MnaOpSchema ──────────────────────────────────────────────────
    void testOpSchemaValidateValidNode();
    void testOpSchemaValidateMissingInput();
    void testOpSchemaValidateMissingAttribute();
    void testOpSchemaValidateExtraInputOk();

    // ── MnaGraph serialization ───────────────────────────────────────
    void testGraphJsonRoundTrip();
    void testGraphCborRoundTrip();
    void testGraphWithParamTree();

    // ── Full project with embedded data → file round-trip ────────────
    void testProjectEmbeddedMnxRoundTrip();
    void testProjectEmbeddedMnaRoundTrip();
    void testProjectMultiSubjectRoundTrip();

    // ── Edge cases ───────────────────────────────────────────────────
    void testEmptyScriptRoundTrip();
    void testEmptyVerificationRoundTrip();
    void testEmptyProvenanceRoundTrip();
    void testNodeWithAllExecModes_data();
    void testNodeWithAllExecModes();

    void cleanupTestCase();

private:
    QTemporaryDir m_tempDir;
};

//=============================================================================================================
// INIT / CLEANUP
//=============================================================================================================

void TestMnaSerialization::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

void TestMnaSerialization::cleanupTestCase()
{
}

//=============================================================================================================
// MnaFileRole enum — data-driven
//=============================================================================================================

void TestMnaSerialization::testFileRoleToString_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<QString>("expected");

    QTest::newRow("Raw")            << static_cast<int>(MnaFileRole::Raw)            << "raw";
    QTest::newRow("Forward")        << static_cast<int>(MnaFileRole::Forward)        << "forward";
    QTest::newRow("Inverse")        << static_cast<int>(MnaFileRole::Inverse)        << "inverse";
    QTest::newRow("Covariance")     << static_cast<int>(MnaFileRole::Covariance)     << "covariance";
    QTest::newRow("SourceEstimate") << static_cast<int>(MnaFileRole::SourceEstimate) << "source_estimate";
    QTest::newRow("Bem")            << static_cast<int>(MnaFileRole::Bem)            << "bem";
    QTest::newRow("Surface")        << static_cast<int>(MnaFileRole::Surface)        << "surface";
    QTest::newRow("Annotation")     << static_cast<int>(MnaFileRole::Annotation)     << "annotation";
    QTest::newRow("Digitizer")      << static_cast<int>(MnaFileRole::Digitizer)      << "digitizer";
    QTest::newRow("Transform")      << static_cast<int>(MnaFileRole::Transform)      << "transform";
    QTest::newRow("SourceSpace")    << static_cast<int>(MnaFileRole::SourceSpace)    << "source_space";
    QTest::newRow("Evoked")         << static_cast<int>(MnaFileRole::Evoked)         << "evoked";
    QTest::newRow("Custom")         << static_cast<int>(MnaFileRole::Custom)         << "custom";
}

void TestMnaSerialization::testFileRoleToString()
{
    QFETCH(int, role);
    QFETCH(QString, expected);
    QCOMPARE(mnaFileRoleToString(static_cast<MnaFileRole>(role)), expected);
}

void TestMnaSerialization::testFileRoleFromString_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("expected");

    QTest::newRow("Raw")            << "raw"               << static_cast<int>(MnaFileRole::Raw);
    QTest::newRow("Forward")        << "forward"           << static_cast<int>(MnaFileRole::Forward);
    QTest::newRow("Inverse")        << "inverse"           << static_cast<int>(MnaFileRole::Inverse);
    QTest::newRow("Covariance")     << "covariance"        << static_cast<int>(MnaFileRole::Covariance);
    QTest::newRow("SourceEstimate") << "source_estimate"   << static_cast<int>(MnaFileRole::SourceEstimate);
    QTest::newRow("Bem")            << "bem"               << static_cast<int>(MnaFileRole::Bem);
    QTest::newRow("Surface")        << "surface"           << static_cast<int>(MnaFileRole::Surface);
    QTest::newRow("Annotation")     << "annotation"        << static_cast<int>(MnaFileRole::Annotation);
    QTest::newRow("Digitizer")      << "digitizer"         << static_cast<int>(MnaFileRole::Digitizer);
    QTest::newRow("Transform")      << "transform"         << static_cast<int>(MnaFileRole::Transform);
    QTest::newRow("SourceSpace")    << "source_space"      << static_cast<int>(MnaFileRole::SourceSpace);
    QTest::newRow("Evoked")         << "evoked"            << static_cast<int>(MnaFileRole::Evoked);
    QTest::newRow("Custom")         << "custom"            << static_cast<int>(MnaFileRole::Custom);
}

void TestMnaSerialization::testFileRoleFromString()
{
    QFETCH(QString, input);
    QFETCH(int, expected);
    QCOMPARE(static_cast<int>(mnaFileRoleFromString(input)), expected);
}

void TestMnaSerialization::testFileRoleFromStringUnknown()
{
    QCOMPARE(mnaFileRoleFromString("NonExistentRole"), MnaFileRole::Custom);
}

//=============================================================================================================
// MnaDataKind enum — data-driven
//=============================================================================================================

void TestMnaSerialization::testDataKindToString_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<QString>("expected");

    QTest::newRow("FiffRaw")        << static_cast<int>(MnaDataKind::FiffRaw)        << "FiffRaw";
    QTest::newRow("Forward")        << static_cast<int>(MnaDataKind::Forward)        << "Forward";
    QTest::newRow("Inverse")        << static_cast<int>(MnaDataKind::Inverse)        << "Inverse";
    QTest::newRow("Covariance")     << static_cast<int>(MnaDataKind::Covariance)     << "Covariance";
    QTest::newRow("SourceEstimate") << static_cast<int>(MnaDataKind::SourceEstimate) << "SourceEstimate";
    QTest::newRow("Epochs")         << static_cast<int>(MnaDataKind::Epochs)         << "Epochs";
    QTest::newRow("Evoked")         << static_cast<int>(MnaDataKind::Evoked)         << "Evoked";
    QTest::newRow("Matrix")         << static_cast<int>(MnaDataKind::Matrix)         << "Matrix";
    QTest::newRow("Volume")         << static_cast<int>(MnaDataKind::Volume)         << "Volume";
    QTest::newRow("Surface")        << static_cast<int>(MnaDataKind::Surface)        << "Surface";
    QTest::newRow("Bem")            << static_cast<int>(MnaDataKind::Bem)            << "Bem";
    QTest::newRow("Annotation")     << static_cast<int>(MnaDataKind::Annotation)     << "Annotation";
    QTest::newRow("Label")          << static_cast<int>(MnaDataKind::Label)          << "Label";
    QTest::newRow("RealTimeStream") << static_cast<int>(MnaDataKind::RealTimeStream) << "RealTimeStream";
    QTest::newRow("Custom")         << static_cast<int>(MnaDataKind::Custom)         << "Custom";
}

void TestMnaSerialization::testDataKindToString()
{
    QFETCH(int, kind);
    QFETCH(QString, expected);
    QCOMPARE(mnaDataKindToString(static_cast<MnaDataKind>(kind)), expected);
}

void TestMnaSerialization::testDataKindFromString_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("expected");

    QTest::newRow("FiffRaw")        << "FiffRaw"        << static_cast<int>(MnaDataKind::FiffRaw);
    QTest::newRow("Forward")        << "Forward"        << static_cast<int>(MnaDataKind::Forward);
    QTest::newRow("Inverse")        << "Inverse"        << static_cast<int>(MnaDataKind::Inverse);
    QTest::newRow("Covariance")     << "Covariance"     << static_cast<int>(MnaDataKind::Covariance);
    QTest::newRow("SourceEstimate") << "SourceEstimate" << static_cast<int>(MnaDataKind::SourceEstimate);
    QTest::newRow("Epochs")         << "Epochs"         << static_cast<int>(MnaDataKind::Epochs);
    QTest::newRow("Evoked")         << "Evoked"         << static_cast<int>(MnaDataKind::Evoked);
    QTest::newRow("Matrix")         << "Matrix"         << static_cast<int>(MnaDataKind::Matrix);
    QTest::newRow("Volume")         << "Volume"         << static_cast<int>(MnaDataKind::Volume);
    QTest::newRow("Surface")        << "Surface"        << static_cast<int>(MnaDataKind::Surface);
    QTest::newRow("Bem")            << "Bem"            << static_cast<int>(MnaDataKind::Bem);
    QTest::newRow("Annotation")     << "Annotation"     << static_cast<int>(MnaDataKind::Annotation);
    QTest::newRow("Label")          << "Label"          << static_cast<int>(MnaDataKind::Label);
    QTest::newRow("RealTimeStream") << "RealTimeStream" << static_cast<int>(MnaDataKind::RealTimeStream);
    QTest::newRow("Custom")         << "Custom"         << static_cast<int>(MnaDataKind::Custom);
}

void TestMnaSerialization::testDataKindFromString()
{
    QFETCH(QString, input);
    QFETCH(int, expected);
    QCOMPARE(static_cast<int>(mnaDataKindFromString(input)), expected);
}

void TestMnaSerialization::testDataKindFromStringUnknown()
{
    QCOMPARE(mnaDataKindFromString("NonExistentKind"), MnaDataKind::Custom);
}

//=============================================================================================================
// MnaFileRef — embedded data
//=============================================================================================================

void TestMnaSerialization::testFileRefEmbeddedJsonRoundTrip()
{
    MnaFileRef original;
    original.role = MnaFileRole::Surface;
    original.path = "sub-01/anat/lh.pial";
    original.format = "pial";
    original.sizeBytes = 12345;
    original.embedded = true;
    original.data = QByteArrayLiteral("\x00\x01\x02\x03\xFF\xFE\xFD");
    original.sha256 = "abc123def456";

    QJsonObject json = original.toJson();
    QVERIFY(json.contains("data"));
    QVERIFY(json["embedded"].toBool());

    MnaFileRef restored = MnaFileRef::fromJson(json);
    QCOMPARE(restored.role, original.role);
    QCOMPARE(restored.path, original.path);
    QCOMPARE(restored.embedded, true);
    QCOMPARE(restored.data, original.data);
    QCOMPARE(restored.sizeBytes, original.sizeBytes);
    QCOMPARE(restored.sha256, original.sha256);
}

void TestMnaSerialization::testFileRefEmbeddedCborRoundTrip()
{
    MnaFileRef original;
    original.role = MnaFileRole::Bem;
    original.path = "sub-01/bem/sample-bem.fif";
    original.format = "fif";
    original.sizeBytes = 99999;
    original.embedded = true;
    original.data = QByteArray(1024, 'X'); // 1KB of 'X'

    QCborMap cbor = original.toCbor();
    MnaFileRef restored = MnaFileRef::fromCbor(cbor);
    QCOMPARE(restored.role, original.role);
    QCOMPARE(restored.path, original.path);
    QCOMPARE(restored.embedded, true);
    QCOMPARE(restored.data.size(), 1024);
    QCOMPARE(restored.data, original.data);
}

void TestMnaSerialization::testFileRefEmbeddedLargeData()
{
    // Test with 100KB of random-ish data to verify base64/cbor handling at scale
    QByteArray largeData(100 * 1024, '\0');
    for (int i = 0; i < largeData.size(); ++i)
        largeData[i] = static_cast<char>(i % 256);

    MnaFileRef ref;
    ref.role = MnaFileRole::Raw;
    ref.path = "large_file.fif";
    ref.embedded = true;
    ref.data = largeData;
    ref.sizeBytes = largeData.size();

    // JSON round-trip
    QJsonObject json = ref.toJson();
    MnaFileRef fromJ = MnaFileRef::fromJson(json);
    QCOMPARE(fromJ.data.size(), largeData.size());
    QCOMPARE(fromJ.data, largeData);

    // CBOR round-trip
    QCborMap cbor = ref.toCbor();
    MnaFileRef fromC = MnaFileRef::fromCbor(cbor);
    QCOMPARE(fromC.data.size(), largeData.size());
    QCOMPARE(fromC.data, largeData);
}

void TestMnaSerialization::testFileRefNonEmbeddedHasNoData()
{
    MnaFileRef ref;
    ref.role = MnaFileRole::Forward;
    ref.path = "fwd.fif";
    ref.embedded = false;
    ref.data.clear();

    QJsonObject json = ref.toJson();
    QVERIFY(!json.contains("data"));

    MnaFileRef restored = MnaFileRef::fromJson(json);
    QCOMPARE(restored.embedded, false);
    QVERIFY(restored.data.isEmpty());
}

//=============================================================================================================
// MnaPort — stream and cached result fields
//=============================================================================================================

void TestMnaSerialization::testPortStreamFieldsJson()
{
    MnaPort port;
    port.name = "raw_stream";
    port.dataKind = MnaDataKind::RealTimeStream;
    port.direction = MnaPortDir::Input;
    port.streamProtocol = "lsl";
    port.streamEndpoint = "type='EEG'";
    port.streamBufferMs = 5000;

    QJsonObject json = port.toJson();
    MnaPort restored = MnaPort::fromJson(json);

    QCOMPARE(restored.streamProtocol, "lsl");
    QCOMPARE(restored.streamEndpoint, "type='EEG'");
    QCOMPARE(restored.streamBufferMs, 5000);
}

void TestMnaSerialization::testPortStreamFieldsCbor()
{
    MnaPort port;
    port.name = "fiff_rt";
    port.dataKind = MnaDataKind::RealTimeStream;
    port.direction = MnaPortDir::Output;
    port.streamProtocol = "fiff-rt";
    port.streamEndpoint = "localhost:4217";
    port.streamBufferMs = 2000;

    QCborMap cbor = port.toCbor();
    MnaPort restored = MnaPort::fromCbor(cbor);

    QCOMPARE(restored.streamProtocol, "fiff-rt");
    QCOMPARE(restored.streamEndpoint, "localhost:4217");
    QCOMPARE(restored.streamBufferMs, 2000);
}

void TestMnaSerialization::testPortCachedResultFields()
{
    MnaPort port;
    port.name = "inverse";
    port.dataKind = MnaDataKind::Inverse;
    port.direction = MnaPortDir::Output;
    port.cachedResultPath = "/tmp/inv_cache.fif";
    port.cachedResultHash = "sha256:aabbccdd";

    QJsonObject json = port.toJson();
    MnaPort restored = MnaPort::fromJson(json);

    QCOMPARE(restored.cachedResultPath, "/tmp/inv_cache.fif");
    QCOMPARE(restored.cachedResultHash, "sha256:aabbccdd");
}

//=============================================================================================================
// MnaScript
//=============================================================================================================

void TestMnaSerialization::testScriptDefaults()
{
    MnaScript script;
    QVERIFY(script.language.isEmpty());
    QVERIFY(script.interpreter.isEmpty());
    QVERIFY(script.interpreterArgs.isEmpty());
    QVERIFY(script.code.isEmpty());
    QVERIFY(script.sourceUri.isEmpty());
    QVERIFY(script.codeSha256.isEmpty());
    QCOMPARE(script.keepTempFile, false);
}

void TestMnaSerialization::testScriptJsonRoundTrip()
{
    MnaScript original;
    original.language = "python";
    original.interpreter = "python3";
    original.interpreterArgs = QStringList() << "-u" << "--no-site";
    original.code = "import mne\nprint('hello')";
    original.sourceUri = "file:///lab/scripts/hello.py";
    original.codeSha256 = "deadbeef1234";
    original.keepTempFile = true;

    QJsonObject json = original.toJson();
    MnaScript restored = MnaScript::fromJson(json);

    QCOMPARE(restored.language, original.language);
    QCOMPARE(restored.interpreter, original.interpreter);
    QCOMPARE(restored.interpreterArgs, original.interpreterArgs);
    QCOMPARE(restored.code, original.code);
    QCOMPARE(restored.sourceUri, original.sourceUri);
    QCOMPARE(restored.codeSha256, original.codeSha256);
    QCOMPARE(restored.keepTempFile, true);
}

void TestMnaSerialization::testScriptCborRoundTrip()
{
    MnaScript original;
    original.language = "shell";
    original.interpreter = "/bin/bash";
    original.code = "echo 'test'";
    original.keepTempFile = false;

    QCborMap cbor = original.toCbor();
    MnaScript restored = MnaScript::fromCbor(cbor);

    QCOMPARE(restored.language, original.language);
    QCOMPARE(restored.interpreter, original.interpreter);
    QCOMPARE(restored.code, original.code);
    QCOMPARE(restored.keepTempFile, false);
}

void TestMnaSerialization::testScriptWithAllFields()
{
    MnaScript script;
    script.language = "r";
    script.interpreter = "Rscript";
    script.interpreterArgs << "--vanilla";
    script.code = "library(tidyverse)\nprint('R script')";
    script.sourceUri = "file:///scripts/analysis.R";
    script.codeSha256 = "abcdef0123456789";
    script.keepTempFile = true;

    // JSON
    QJsonObject json = script.toJson();
    MnaScript rJ = MnaScript::fromJson(json);
    QCOMPARE(rJ.language, "r");
    QCOMPARE(rJ.interpreterArgs.size(), 1);
    QCOMPARE(rJ.interpreterArgs.first(), "--vanilla");

    // CBOR
    QCborMap cbor = script.toCbor();
    MnaScript rC = MnaScript::fromCbor(cbor);
    QCOMPARE(rC.language, "r");
    QCOMPARE(rC.codeSha256, "abcdef0123456789");
}

//=============================================================================================================
// MnaVerificationCheck
//=============================================================================================================

void TestMnaSerialization::testVerificationCheckJsonRoundTrip()
{
    MnaVerificationCheck check;
    check.id = "cov_posdef";
    check.description = "Covariance matrix must be positive-definite";
    check.phase = "post";
    check.expression = "rank(covariance) > 0";
    check.severity = "error";
    check.onFail = "Recompute noise covariance with longer data";

    QJsonObject json = check.toJson();
    MnaVerificationCheck restored = MnaVerificationCheck::fromJson(json);

    QCOMPARE(restored.id, check.id);
    QCOMPARE(restored.description, check.description);
    QCOMPARE(restored.phase, "post");
    QCOMPARE(restored.expression, check.expression);
    QCOMPARE(restored.severity, "error");
    QCOMPARE(restored.onFail, check.onFail);
}

void TestMnaSerialization::testVerificationCheckCborRoundTrip()
{
    MnaVerificationCheck check;
    check.id = "snr_threshold";
    check.description = "SNR must be above 3";
    check.phase = "post";
    check.expression = "snr >= 3.0";
    check.severity = "warning";

    QCborMap cbor = check.toCbor();
    MnaVerificationCheck restored = MnaVerificationCheck::fromCbor(cbor);

    QCOMPARE(restored.id, "snr_threshold");
    QCOMPARE(restored.severity, "warning");
    QCOMPARE(restored.expression, "snr >= 3.0");
}

void TestMnaSerialization::testVerificationCheckWithScript()
{
    MnaVerificationCheck check;
    check.id = "custom_validation";
    check.phase = "post";
    check.severity = "error";
    check.script.language = "python";
    check.script.code = "import sys; sys.exit(0)";

    QJsonObject json = check.toJson();
    MnaVerificationCheck restored = MnaVerificationCheck::fromJson(json);

    QCOMPARE(restored.script.language, "python");
    QCOMPARE(restored.script.code, "import sys; sys.exit(0)");
}

//=============================================================================================================
// MnaVerificationResult
//=============================================================================================================

void TestMnaSerialization::testVerificationResultJsonRoundTrip()
{
    MnaVerificationResult result;
    result.checkId = "cov_posdef";
    result.passed = true;
    result.severity = "error";
    result.message = "PASS: covariance is positive-definite";
    result.actualValue = 64;
    result.evaluatedAt = QDateTime(QDate(2026, 4, 19), QTime(12, 0, 0), Qt::UTC);

    QJsonObject json = result.toJson();
    MnaVerificationResult restored = MnaVerificationResult::fromJson(json);

    QCOMPARE(restored.checkId, "cov_posdef");
    QCOMPARE(restored.passed, true);
    QCOMPARE(restored.severity, "error");
    QCOMPARE(restored.message, result.message);
    QCOMPARE(restored.actualValue.toInt(), 64);
}

void TestMnaSerialization::testVerificationResultCborRoundTrip()
{
    MnaVerificationResult result;
    result.checkId = "snr_check";
    result.passed = false;
    result.severity = "warning";
    result.message = "FAIL [warning]: SNR = 1.5 < 3.0";
    result.actualValue = 1.5;
    result.evaluatedAt = QDateTime::currentDateTimeUtc();

    QCborMap cbor = result.toCbor();
    MnaVerificationResult restored = MnaVerificationResult::fromCbor(cbor);

    QCOMPARE(restored.checkId, "snr_check");
    QCOMPARE(restored.passed, false);
    QCOMPARE(restored.severity, "warning");
}

//=============================================================================================================
// MnaProvenance
//=============================================================================================================

void TestMnaSerialization::testProvenanceJsonRoundTrip()
{
    MnaProvenance prov;
    prov.mneCppVersion = "2.2.0";
    prov.qtVersion = "6.11.0";
    prov.compilerInfo = "AppleClang 16.0.0";
    prov.osInfo = "macOS 15.4 arm64";
    prov.hostName = "lab-workstation";
    prov.wallTimeMs = 12345;
    prov.peakMemoryBytes = 256 * 1024 * 1024;
    prov.randomSeed = 42;
    prov.startedAt = QDateTime(QDate(2026, 4, 19), QTime(10, 0, 0), Qt::UTC);
    prov.finishedAt = QDateTime(QDate(2026, 4, 19), QTime(10, 0, 12), Qt::UTC);

    prov.inputHashes["raw"] = "sha256:aabb";
    prov.inputHashes["noise_cov"] = "sha256:ccdd";
    prov.resolvedAttributes["lambda2"] = 0.111;
    prov.resolvedAttributes["method"] = "dSPM";

    QJsonObject json = prov.toJson();
    MnaProvenance restored = MnaProvenance::fromJson(json);

    QCOMPARE(restored.mneCppVersion, "2.2.0");
    QCOMPARE(restored.qtVersion, "6.11.0");
    QCOMPARE(restored.compilerInfo, "AppleClang 16.0.0");
    QCOMPARE(restored.hostName, "lab-workstation");
    QCOMPARE(restored.wallTimeMs, qint64(12345));
    QCOMPARE(restored.peakMemoryBytes, qint64(256 * 1024 * 1024));
    QCOMPARE(restored.randomSeed, qint64(42));
    QCOMPARE(restored.inputHashes.size(), 2);
    QCOMPARE(restored.inputHashes["raw"], "sha256:aabb");
    QCOMPARE(restored.resolvedAttributes["method"].toString(), "dSPM");
}

void TestMnaSerialization::testProvenanceCborRoundTrip()
{
    MnaProvenance prov;
    prov.mneCppVersion = "2.2.0";
    prov.wallTimeMs = 9876;
    prov.externalToolVersion = "FreeSurfer 7.4.1";

    QCborMap cbor = prov.toCbor();
    MnaProvenance restored = MnaProvenance::fromCbor(cbor);

    QCOMPARE(restored.mneCppVersion, "2.2.0");
    QCOMPARE(restored.wallTimeMs, qint64(9876));
    QCOMPARE(restored.externalToolVersion, "FreeSurfer 7.4.1");
}

void TestMnaSerialization::testProvenanceAllFields()
{
    MnaProvenance prov;
    prov.mneCppVersion = "2.2.0";
    prov.qtVersion = "6.11.0";
    prov.compilerInfo = "GCC 13.2";
    prov.osInfo = "Ubuntu 24.04 x86_64";
    prov.hostName = "cluster-node-01";
    prov.externalToolVersion = "Python 3.11.5";
    prov.startedAt = QDateTime::currentDateTimeUtc();
    prov.finishedAt = QDateTime::currentDateTimeUtc().addSecs(60);
    prov.wallTimeMs = 60000;
    prov.peakMemoryBytes = 1024 * 1024 * 1024;
    prov.randomSeed = 12345;
    prov.inputHashes["epochs"] = "sha256:1111";
    prov.resolvedAttributes["n_components"] = 20;

    // JSON round-trip
    QJsonObject json = prov.toJson();
    MnaProvenance rJ = MnaProvenance::fromJson(json);
    QCOMPARE(rJ.hostName, "cluster-node-01");
    QCOMPARE(rJ.externalToolVersion, "Python 3.11.5");
    QCOMPARE(rJ.peakMemoryBytes, qint64(1024 * 1024 * 1024));

    // CBOR round-trip
    QCborMap cbor = prov.toCbor();
    MnaProvenance rC = MnaProvenance::fromCbor(cbor);
    QCOMPARE(rC.osInfo, "Ubuntu 24.04 x86_64");
    QCOMPARE(rC.randomSeed, qint64(12345));
}

//=============================================================================================================
// MnaVerification (composite)
//=============================================================================================================

void TestMnaSerialization::testVerificationJsonRoundTrip()
{
    MnaVerification verif;
    verif.explanation = "Compute dSPM inverse solution";

    MnaVerificationCheck check1;
    check1.id = "cov_check";
    check1.phase = "pre";
    check1.expression = "has_cov == true";
    check1.severity = "error";
    verif.checks.append(check1);

    MnaVerificationResult preRes;
    preRes.checkId = "cov_check";
    preRes.passed = true;
    preRes.severity = "error";
    preRes.message = "PASS";
    verif.preResults.append(preRes);

    verif.provenance.mneCppVersion = "2.2.0";
    verif.provenance.wallTimeMs = 5000;

    QJsonObject json = verif.toJson();
    MnaVerification restored = MnaVerification::fromJson(json);

    QCOMPARE(restored.explanation, "Compute dSPM inverse solution");
    QCOMPARE(restored.checks.size(), 1);
    QCOMPARE(restored.checks[0].id, "cov_check");
    QCOMPARE(restored.preResults.size(), 1);
    QCOMPARE(restored.preResults[0].passed, true);
    QCOMPARE(restored.provenance.mneCppVersion, "2.2.0");
}

void TestMnaSerialization::testVerificationCborRoundTrip()
{
    MnaVerification verif;
    verif.explanation = "BEM solution check";

    MnaVerificationCheck check;
    check.id = "mesh_closed";
    check.phase = "post";
    check.severity = "error";
    verif.checks.append(check);

    MnaVerificationResult postRes;
    postRes.checkId = "mesh_closed";
    postRes.passed = true;
    postRes.severity = "error";
    verif.postResults.append(postRes);

    QCborMap cbor = verif.toCbor();
    MnaVerification restored = MnaVerification::fromCbor(cbor);

    QCOMPARE(restored.explanation, "BEM solution check");
    QCOMPARE(restored.checks.size(), 1);
    QCOMPARE(restored.postResults.size(), 1);
}

//=============================================================================================================
// MnaNode with IPC / Script exec modes and verification
//=============================================================================================================

void TestMnaSerialization::testNodeExecModeIpcJson()
{
    MnaNode node;
    node.id = "freesurfer_recon";
    node.opType = "recon_all";
    node.execMode = MnaNodeExecMode::Ipc;
    node.ipcCommand = "recon-all";
    node.ipcArgs = QStringList() << "-s" << "sub-01" << "-all";
    node.ipcWorkDir = "/data/freesurfer";
    node.ipcTransport = "subprocess";

    QJsonObject json = node.toJson();
    MnaNode restored = MnaNode::fromJson(json);

    QCOMPARE(restored.execMode, MnaNodeExecMode::Ipc);
    QCOMPARE(restored.ipcCommand, "recon-all");
    QCOMPARE(restored.ipcArgs.size(), 3);
    QCOMPARE(restored.ipcWorkDir, "/data/freesurfer");
    QCOMPARE(restored.ipcTransport, "subprocess");
}

void TestMnaSerialization::testNodeExecModeScriptJson()
{
    MnaNode node;
    node.id = "py_bandpass";
    node.opType = "custom_filter";
    node.execMode = MnaNodeExecMode::Script;
    node.script.language = "python";
    node.script.interpreter = "python3";
    node.script.code = "import mne\nraw.filter(1, 40)";

    QJsonObject json = node.toJson();
    MnaNode restored = MnaNode::fromJson(json);

    QCOMPARE(restored.execMode, MnaNodeExecMode::Script);
    QCOMPARE(restored.script.language, "python");
    QCOMPARE(restored.script.code, "import mne\nraw.filter(1, 40)");
}

void TestMnaSerialization::testNodeExecModeCbor_data()
{
    QTest::addColumn<int>("execMode");
    QTest::addColumn<QString>("label");

    QTest::newRow("Batch")  << static_cast<int>(MnaNodeExecMode::Batch)  << "Batch";
    QTest::newRow("Stream") << static_cast<int>(MnaNodeExecMode::Stream) << "Stream";
    QTest::newRow("Ipc")    << static_cast<int>(MnaNodeExecMode::Ipc)    << "Ipc";
    QTest::newRow("Script") << static_cast<int>(MnaNodeExecMode::Script) << "Script";
}

void TestMnaSerialization::testNodeExecModeCbor()
{
    QFETCH(int, execMode);

    MnaNode node;
    node.id = "test_node";
    node.opType = "test_op";
    node.execMode = static_cast<MnaNodeExecMode>(execMode);

    QCborMap cbor = node.toCbor();
    MnaNode restored = MnaNode::fromCbor(cbor);

    QCOMPARE(static_cast<int>(restored.execMode), execMode);
}

void TestMnaSerialization::testNodeWithVerificationRoundTrip()
{
    MnaNode node;
    node.id = "inverse_compute";
    node.opType = "mne_compute_mne";
    node.toolVersion = "2.2.0";
    node.executedAt = QDateTime::currentDateTimeUtc();
    node.dirty = true;

    // Add verification
    MnaVerificationCheck check;
    check.id = "inv_rank";
    check.phase = "post";
    check.expression = "rank > 0";
    check.severity = "error";
    node.verification.checks.append(check);
    node.verification.explanation = "MNE inverse computation";
    node.verification.provenance.mneCppVersion = "2.2.0";

    // JSON
    QJsonObject json = node.toJson();
    MnaNode rJ = MnaNode::fromJson(json);
    QCOMPARE(rJ.verification.explanation, "MNE inverse computation");
    QCOMPARE(rJ.verification.checks.size(), 1);
    QCOMPARE(rJ.dirty, true);

    // CBOR
    QCborMap cbor = node.toCbor();
    MnaNode rC = MnaNode::fromCbor(cbor);
    QCOMPARE(rC.verification.checks[0].id, "inv_rank");
    QCOMPARE(rC.verification.provenance.mneCppVersion, "2.2.0");
}

//=============================================================================================================
// MnaParamBinding
//=============================================================================================================

void TestMnaSerialization::testParamBindingDefaults()
{
    MnaParamBinding binding;
    QVERIFY(binding.targetPath.isEmpty());
    QVERIFY(binding.expression.isEmpty());
    QVERIFY(binding.trigger.isEmpty());
    QCOMPARE(binding.periodMs, 0);
    QVERIFY(binding.dependencies.isEmpty());
}

void TestMnaSerialization::testParamBindingJsonRoundTrip()
{
    MnaParamBinding binding;
    binding.targetPath = "inverse_compute/lambda2";
    binding.expression = "clamp(ref('noise_est/snr') * 0.1, 0.01, 1.0)";
    binding.trigger = "on_change";
    binding.periodMs = 0;
    binding.dependencies << "noise_est/snr";

    QJsonObject json = binding.toJson();
    MnaParamBinding restored = MnaParamBinding::fromJson(json);

    QCOMPARE(restored.targetPath, binding.targetPath);
    QCOMPARE(restored.expression, binding.expression);
    QCOMPARE(restored.trigger, "on_change");
    QCOMPARE(restored.dependencies.size(), 1);
    QCOMPARE(restored.dependencies.first(), "noise_est/snr");
}

void TestMnaSerialization::testParamBindingCborRoundTrip()
{
    MnaParamBinding binding;
    binding.targetPath = "filter/cutoff_hz";
    binding.expression = "40.0";
    binding.trigger = "periodic";
    binding.periodMs = 1000;
    binding.dependencies << "input/sfreq" << "input/nfft";

    QCborMap cbor = binding.toCbor();
    MnaParamBinding restored = MnaParamBinding::fromCbor(cbor);

    QCOMPARE(restored.targetPath, "filter/cutoff_hz");
    QCOMPARE(restored.trigger, "periodic");
    QCOMPARE(restored.periodMs, 1000);
    QCOMPARE(restored.dependencies.size(), 2);
}

//=============================================================================================================
// MnaParamTree
//=============================================================================================================

void TestMnaSerialization::testParamTreeSetGet()
{
    MnaParamTree tree;
    tree.setParam("inverse/lambda2", 0.111);
    tree.setParam("inverse/method", "dSPM");
    tree.setParam("filter/lfreq", 1.0);

    QCOMPARE(tree.param("inverse/lambda2").toDouble(), 0.111);
    QCOMPARE(tree.param("inverse/method").toString(), "dSPM");
    QCOMPARE(tree.param("filter/lfreq").toDouble(), 1.0);
    QVERIFY(!tree.param("nonexistent").isValid());
}

void TestMnaSerialization::testParamTreeHasParam()
{
    MnaParamTree tree;
    QVERIFY(!tree.hasParam("x"));
    tree.setParam("x", 42);
    QVERIFY(tree.hasParam("x"));
}

void TestMnaSerialization::testParamTreeAllPaths()
{
    MnaParamTree tree;
    tree.setParam("a/b", 1);
    tree.setParam("a/c", 2);
    tree.setParam("d", 3);

    QStringList paths = tree.allPaths();
    QCOMPARE(paths.size(), 3);
    QVERIFY(paths.contains("a/b"));
    QVERIFY(paths.contains("a/c"));
    QVERIFY(paths.contains("d"));
}

void TestMnaSerialization::testParamTreeBindings()
{
    MnaParamTree tree;

    MnaParamBinding b1;
    b1.targetPath = "node1/attr1";
    b1.expression = "42";
    b1.trigger = "manual";
    tree.addBinding(b1);

    QVERIFY(tree.hasBinding("node1/attr1"));
    QCOMPARE(tree.bindings().size(), 1);

    tree.removeBinding("node1/attr1");
    QVERIFY(!tree.hasBinding("node1/attr1"));
    QCOMPARE(tree.bindings().size(), 0);
}

void TestMnaSerialization::testParamTreeEvaluateExpression()
{
    MnaParamTree tree;

    // ref() expression that looks up from results map
    QMap<QString, QVariant> results;
    results["step1::output"] = 42;

    QVariant val = tree.evaluateExpression("ref('step1::output')", results);
    QCOMPARE(val.toInt(), 42);
}

void TestMnaSerialization::testParamTreeJsonRoundTrip()
{
    MnaParamTree tree;
    tree.setParam("inverse/lambda2", 0.111);
    tree.setParam("inverse/method", "dSPM");

    MnaParamBinding b;
    b.targetPath = "inverse/snr";
    b.expression = "3.0";
    b.trigger = "manual";
    tree.addBinding(b);

    QJsonObject json = tree.toJson();
    MnaParamTree restored = MnaParamTree::fromJson(json);

    // Unbound params survive round-trip
    QCOMPARE(restored.param("inverse/lambda2").toDouble(), 0.111);
    QCOMPARE(restored.param("inverse/method").toString(), "dSPM");
    // Binding survives round-trip
    QVERIFY(restored.hasBinding("inverse/snr"));
}

//=============================================================================================================
// MnaOpSchema
//=============================================================================================================

void TestMnaSerialization::testOpSchemaValidateValidNode()
{
    MnaOpSchema schema;
    schema.opType = "my_filter";

    MnaOpSchemaPort inPort;
    inPort.name = "raw";
    inPort.dataKind = MnaDataKind::FiffRaw;
    inPort.required = true;
    schema.inputPorts.append(inPort);

    MnaOpSchemaPort outPort;
    outPort.name = "filtered";
    outPort.dataKind = MnaDataKind::FiffRaw;
    outPort.required = true;
    schema.outputPorts.append(outPort);

    MnaOpSchemaAttr attr;
    attr.name = "cutoff_hz";
    attr.type = QMetaType::Double;
    attr.required = true;
    schema.attributes.append(attr);

    // Build a valid node
    MnaNode node;
    node.opType = "my_filter";
    MnaPort in; in.name = "raw"; in.dataKind = MnaDataKind::FiffRaw; in.direction = MnaPortDir::Input;
    MnaPort out; out.name = "filtered"; out.dataKind = MnaDataKind::FiffRaw; out.direction = MnaPortDir::Output;
    node.inputs.append(in);
    node.outputs.append(out);
    node.attributes["cutoff_hz"] = 40.0;

    QStringList errors;
    QVERIFY(schema.validate(node, &errors));
    QVERIFY(errors.isEmpty());
}

void TestMnaSerialization::testOpSchemaValidateMissingInput()
{
    MnaOpSchema schema;
    schema.opType = "my_op";

    MnaOpSchemaPort reqPort;
    reqPort.name = "epochs";
    reqPort.dataKind = MnaDataKind::Epochs;
    reqPort.required = true;
    schema.inputPorts.append(reqPort);

    MnaNode node;
    node.opType = "my_op";
    // No inputs

    QStringList errors;
    QVERIFY(!schema.validate(node, &errors));
    QVERIFY(!errors.isEmpty());
}

void TestMnaSerialization::testOpSchemaValidateMissingAttribute()
{
    MnaOpSchema schema;
    schema.opType = "my_op";

    MnaOpSchemaAttr reqAttr;
    reqAttr.name = "n_components";
    reqAttr.type = QMetaType::Int;
    reqAttr.required = true;
    schema.attributes.append(reqAttr);

    MnaNode node;
    node.opType = "my_op";
    // Missing required attribute

    QStringList errors;
    QVERIFY(!schema.validate(node, &errors));
    QVERIFY(!errors.isEmpty());
}

void TestMnaSerialization::testOpSchemaValidateExtraInputOk()
{
    // Extra inputs beyond schema should not fail validation
    MnaOpSchema schema;
    schema.opType = "simple_op";

    MnaNode node;
    node.opType = "simple_op";
    MnaPort extra;
    extra.name = "extra_input";
    extra.direction = MnaPortDir::Input;
    node.inputs.append(extra);

    QStringList errors;
    QVERIFY(schema.validate(node, &errors));
}

//=============================================================================================================
// MnaGraph serialization
//=============================================================================================================

void TestMnaSerialization::testGraphJsonRoundTrip()
{
    MnaGraph graph;

    MnaNode n1;
    n1.id = "read_raw";
    n1.opType = "fiff_read_raw";
    MnaPort n1out;
    n1out.name = "raw";
    n1out.dataKind = MnaDataKind::FiffRaw;
    n1out.direction = MnaPortDir::Output;
    n1.outputs.append(n1out);
    graph.addNode(n1);

    MnaNode n2;
    n2.id = "filter";
    n2.opType = "bandpass_filter";
    MnaPort n2in;
    n2in.name = "raw";
    n2in.dataKind = MnaDataKind::FiffRaw;
    n2in.direction = MnaPortDir::Input;
    n2.inputs.append(n2in);
    MnaPort n2out;
    n2out.name = "filtered";
    n2out.dataKind = MnaDataKind::FiffRaw;
    n2out.direction = MnaPortDir::Output;
    n2.outputs.append(n2out);
    graph.addNode(n2);

    graph.connect("read_raw", "raw", "filter", "raw");

    QJsonObject json = graph.toJson();
    MnaGraph restored = MnaGraph::fromJson(json);

    QCOMPARE(restored.nodes().size(), 2);
    QVERIFY(restored.hasNode("read_raw"));
    QVERIFY(restored.hasNode("filter"));
}

void TestMnaSerialization::testGraphCborRoundTrip()
{
    MnaGraph graph;

    MnaNode n1;
    n1.id = "src";
    n1.opType = "source_op";
    MnaPort n1out;
    n1out.name = "data";
    n1out.direction = MnaPortDir::Output;
    n1.outputs.append(n1out);
    graph.addNode(n1);

    QCborMap cbor = graph.toCbor();
    MnaGraph restored = MnaGraph::fromCbor(cbor);

    QCOMPARE(restored.nodes().size(), 1);
    QVERIFY(restored.hasNode("src"));
}

void TestMnaSerialization::testGraphWithParamTree()
{
    MnaGraph graph;
    graph.paramTree.setParam("global/sfreq", 1000.0);
    graph.paramTree.setParam("global/n_channels", 306);

    MnaNode n1;
    n1.id = "node1";
    n1.opType = "op1";
    graph.addNode(n1);

    QJsonObject json = graph.toJson();
    MnaGraph restored = MnaGraph::fromJson(json);

    QCOMPARE(restored.paramTree.param("global/sfreq").toDouble(), 1000.0);
    QCOMPARE(restored.paramTree.param("global/n_channels").toInt(), 306);
}

//=============================================================================================================
// Full project with embedded data → file round-trip
//=============================================================================================================

void TestMnaSerialization::testProjectEmbeddedMnxRoundTrip()
{
    MnaProject proj;
    proj.name = "Embedded Test";
    proj.mnaVersion = MnaProject::CURRENT_SCHEMA_VERSION;
    proj.modified = QDateTime::currentDateTimeUtc();

    MnaFileRef ref;
    ref.role = MnaFileRole::Surface;
    ref.path = "sub-01/anat/lh.pial";
    ref.format = "pial";
    ref.embedded = true;
    ref.data = QByteArray(512, '\xAA');
    ref.sizeBytes = ref.data.size();

    MnaRecording rec;
    rec.id = "rec-01";
    rec.files.append(ref);

    MnaSession sess;
    sess.id = "ses-01";
    sess.recordings.append(rec);

    MnaSubject subj;
    subj.id = "sample";
    subj.sessions.append(sess);
    proj.subjects.append(subj);

    // Write as .mnx
    const QString mnxPath = m_tempDir.filePath("embedded_test.mnx");
    QVERIFY(MnaIO::write(proj, mnxPath));

    // Read back
    MnaProject restored = MnaIO::read(mnxPath);
    QCOMPARE(restored.name, "Embedded Test");
    QCOMPARE(restored.subjects.size(), 1);
    QCOMPARE(restored.subjects[0].sessions[0].recordings[0].files.size(), 1);

    const MnaFileRef &rRef = restored.subjects[0].sessions[0].recordings[0].files[0];
    QCOMPARE(rRef.embedded, true);
    QCOMPARE(rRef.data.size(), 512);
    QCOMPARE(rRef.data, QByteArray(512, '\xAA'));
    QCOMPARE(rRef.role, MnaFileRole::Surface);
}

void TestMnaSerialization::testProjectEmbeddedMnaRoundTrip()
{
    MnaProject proj;
    proj.name = "JSON Embedded Test";
    proj.mnaVersion = MnaProject::CURRENT_SCHEMA_VERSION;
    proj.modified = QDateTime::currentDateTimeUtc();

    MnaFileRef ref;
    ref.role = MnaFileRole::Annotation;
    ref.path = "lh.aparc.annot";
    ref.embedded = true;
    ref.data = QByteArrayLiteral("\x00\x01\x02\x03\x04\x05");
    ref.sizeBytes = ref.data.size();

    MnaRecording rec;
    rec.id = "rec-01";
    rec.files.append(ref);

    MnaSession sess;
    sess.id = "ses-01";
    sess.recordings.append(rec);

    MnaSubject subj;
    subj.id = "s01";
    subj.sessions.append(sess);
    proj.subjects.append(subj);

    const QString mnaPath = m_tempDir.filePath("embedded_test.mna");
    QVERIFY(MnaIO::write(proj, mnaPath));

    MnaProject restored = MnaIO::read(mnaPath);
    QCOMPARE(restored.subjects[0].sessions[0].recordings[0].files[0].embedded, true);
    QCOMPARE(restored.subjects[0].sessions[0].recordings[0].files[0].data,
             QByteArrayLiteral("\x00\x01\x02\x03\x04\x05"));
}

void TestMnaSerialization::testProjectMultiSubjectRoundTrip()
{
    MnaProject proj;
    proj.name = "Multi-Subject";
    proj.mnaVersion = MnaProject::CURRENT_SCHEMA_VERSION;
    proj.modified = QDateTime::currentDateTimeUtc();

    // Create 3 subjects, each with 2 sessions, each with 2 files
    for (int si = 0; si < 3; ++si) {
        MnaSubject subj;
        subj.id = QString("sub-%1").arg(si + 1, 2, 10, QLatin1Char('0'));

        for (int sei = 0; sei < 2; ++sei) {
            MnaSession sess;
            sess.id = QString("ses-%1").arg(sei + 1, 2, 10, QLatin1Char('0'));

            MnaRecording rec;
            rec.id = "rec-01";

            MnaFileRef raw;
            raw.role = MnaFileRole::Raw;
            raw.path = QString("sub-%1/ses-%2/meg/raw.fif").arg(si + 1, 2, 10, QLatin1Char('0')).arg(sei + 1, 2, 10, QLatin1Char('0'));
            raw.embedded = true;
            raw.data = QByteArray(64, static_cast<char>(si * 10 + sei));
            raw.sizeBytes = raw.data.size();
            rec.files.append(raw);

            MnaFileRef stc;
            stc.role = MnaFileRole::SourceEstimate;
            stc.path = QString("sub-%1/ses-%2/source/result.stc").arg(si + 1, 2, 10, QLatin1Char('0')).arg(sei + 1, 2, 10, QLatin1Char('0'));
            stc.embedded = true;
            stc.data = QByteArray(32, static_cast<char>(si * 10 + sei + 100));
            stc.sizeBytes = stc.data.size();
            rec.files.append(stc);

            sess.recordings.append(rec);
            subj.sessions.append(sess);
        }
        proj.subjects.append(subj);
    }

    // MNX round-trip
    const QString mnxPath = m_tempDir.filePath("multi_subj.mnx");
    QVERIFY(MnaIO::write(proj, mnxPath));
    MnaProject restored = MnaIO::read(mnxPath);

    QCOMPARE(restored.subjects.size(), 3);
    for (int si = 0; si < 3; ++si) {
        QCOMPARE(restored.subjects[si].sessions.size(), 2);
        for (int sei = 0; sei < 2; ++sei) {
            QCOMPARE(restored.subjects[si].sessions[sei].recordings[0].files.size(), 2);
            const auto &files = restored.subjects[si].sessions[sei].recordings[0].files;
            QCOMPARE(files[0].role, MnaFileRole::Raw);
            QCOMPARE(files[0].data.size(), 64);
            QCOMPARE(files[1].role, MnaFileRole::SourceEstimate);
            QCOMPARE(files[1].data.size(), 32);
        }
    }
}

//=============================================================================================================
// Edge cases — empty structs
//=============================================================================================================

void TestMnaSerialization::testEmptyScriptRoundTrip()
{
    MnaScript empty;
    QJsonObject json = empty.toJson();
    MnaScript restored = MnaScript::fromJson(json);
    QVERIFY(restored.language.isEmpty());
    QVERIFY(restored.code.isEmpty());
    QCOMPARE(restored.keepTempFile, false);

    QCborMap cbor = empty.toCbor();
    MnaScript rC = MnaScript::fromCbor(cbor);
    QVERIFY(rC.language.isEmpty());
}

void TestMnaSerialization::testEmptyVerificationRoundTrip()
{
    MnaVerification empty;
    QJsonObject json = empty.toJson();
    MnaVerification restored = MnaVerification::fromJson(json);
    QVERIFY(restored.explanation.isEmpty());
    QVERIFY(restored.checks.isEmpty());
    QVERIFY(restored.preResults.isEmpty());
    QVERIFY(restored.postResults.isEmpty());
}

void TestMnaSerialization::testEmptyProvenanceRoundTrip()
{
    MnaProvenance empty;
    QJsonObject json = empty.toJson();
    MnaProvenance restored = MnaProvenance::fromJson(json);
    QVERIFY(restored.mneCppVersion.isEmpty());
    QCOMPARE(restored.wallTimeMs, qint64(0));
    QCOMPARE(restored.peakMemoryBytes, qint64(0));
    QCOMPARE(restored.randomSeed, qint64(-1));
}

void TestMnaSerialization::testNodeWithAllExecModes_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<QString>("modeStr");

    QTest::newRow("Batch")  << static_cast<int>(MnaNodeExecMode::Batch)  << "Batch";
    QTest::newRow("Stream") << static_cast<int>(MnaNodeExecMode::Stream) << "Stream";
    QTest::newRow("Ipc")    << static_cast<int>(MnaNodeExecMode::Ipc)    << "Ipc";
    QTest::newRow("Script") << static_cast<int>(MnaNodeExecMode::Script) << "Script";
}

void TestMnaSerialization::testNodeWithAllExecModes()
{
    QFETCH(int, mode);

    MnaNode node;
    node.id = "test_node";
    node.opType = "test_op";
    node.execMode = static_cast<MnaNodeExecMode>(mode);

    // JSON
    QJsonObject json = node.toJson();
    MnaNode rJ = MnaNode::fromJson(json);
    QCOMPARE(static_cast<int>(rJ.execMode), mode);

    // CBOR
    QCborMap cbor = node.toCbor();
    MnaNode rC = MnaNode::fromCbor(cbor);
    QCOMPARE(static_cast<int>(rC.execMode), mode);
}

//=============================================================================================================
// TEST MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMnaSerialization)
#include "test_mna_serialization.moc"
