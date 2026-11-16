//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mna_serialization.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Serialization tests for all MNA data model classes: enum round-trips,
 *           embedded file data, MnaScript, MnaVerification, MnaParamBinding,
 *           MnaParamTree, MnaOpSchema, MnaGraph, and edge cases.
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

#include <base/json.h>
#include <base/cbor.h>
#include <base/string_utils.h>

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

namespace
{

UtcTime utc(int y, unsigned mo, unsigned d, int h, int mi, int sec, int ms = 0)
{
    using namespace std::chrono;
    return sys_days{year{y} / month{mo} / day{d}} + hours{h} + minutes{mi} + seconds{sec} + milliseconds{ms};
}

UtcTime now()
{
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
}

std::vector<std::byte> bytes(std::initializer_list<unsigned char> values)
{
    std::vector<std::byte> out;
    out.reserve(values.size());
    for (const unsigned char v : values)
        out.push_back(static_cast<std::byte>(v));
    return out;
}

std::filesystem::path fs(const QString& path)
{
    return path_from_utf8(path.toStdString());
}

} // namespace

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
    void testVerificationRoundTripsEveryField();

    // ── MnaNode with IPC/Script/Verification ─────────────────────────
    void testNodeExecModeIpcJson();
    void testNodeExecModeScriptJson();
    void testNodeExecModeCbor_data();
    void testNodeExecModeCbor();
    void testNodeWithVerificationRoundTrip();
    void testNodeRoundTripsEveryField_data();
    void testNodeRoundTripsEveryField();

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
    void testParamTreeEvaluateBindings();
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

    QTest::newRow("Raw") << static_cast<int>(MnaFileRole::Raw) << "raw";
    QTest::newRow("Forward") << static_cast<int>(MnaFileRole::Forward) << "forward";
    QTest::newRow("Inverse") << static_cast<int>(MnaFileRole::Inverse) << "inverse";
    QTest::newRow("Covariance") << static_cast<int>(MnaFileRole::Covariance) << "covariance";
    QTest::newRow("SourceEstimate") << static_cast<int>(MnaFileRole::SourceEstimate) << "source_estimate";
    QTest::newRow("Bem") << static_cast<int>(MnaFileRole::Bem) << "bem";
    QTest::newRow("Surface") << static_cast<int>(MnaFileRole::Surface) << "surface";
    QTest::newRow("Annotation") << static_cast<int>(MnaFileRole::Annotation) << "annotation";
    QTest::newRow("Digitizer") << static_cast<int>(MnaFileRole::Digitizer) << "digitizer";
    QTest::newRow("Transform") << static_cast<int>(MnaFileRole::Transform) << "transform";
    QTest::newRow("SourceSpace") << static_cast<int>(MnaFileRole::SourceSpace) << "source_space";
    QTest::newRow("Evoked") << static_cast<int>(MnaFileRole::Evoked) << "evoked";
    QTest::newRow("Custom") << static_cast<int>(MnaFileRole::Custom) << "custom";
}

void TestMnaSerialization::testFileRoleToString()
{
    QFETCH(int, role);
    QFETCH(QString, expected);
    QCOMPARE(to_string(static_cast<MnaFileRole>(role)), expected.toStdString());
}

void TestMnaSerialization::testFileRoleFromString_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("expected");

    QTest::newRow("Raw") << "raw" << static_cast<int>(MnaFileRole::Raw);
    QTest::newRow("Forward") << "forward" << static_cast<int>(MnaFileRole::Forward);
    QTest::newRow("Inverse") << "inverse" << static_cast<int>(MnaFileRole::Inverse);
    QTest::newRow("Covariance") << "covariance" << static_cast<int>(MnaFileRole::Covariance);
    QTest::newRow("SourceEstimate") << "source_estimate" << static_cast<int>(MnaFileRole::SourceEstimate);
    QTest::newRow("Bem") << "bem" << static_cast<int>(MnaFileRole::Bem);
    QTest::newRow("Surface") << "surface" << static_cast<int>(MnaFileRole::Surface);
    QTest::newRow("Annotation") << "annotation" << static_cast<int>(MnaFileRole::Annotation);
    QTest::newRow("Digitizer") << "digitizer" << static_cast<int>(MnaFileRole::Digitizer);
    QTest::newRow("Transform") << "transform" << static_cast<int>(MnaFileRole::Transform);
    QTest::newRow("SourceSpace") << "source_space" << static_cast<int>(MnaFileRole::SourceSpace);
    QTest::newRow("Evoked") << "evoked" << static_cast<int>(MnaFileRole::Evoked);
    QTest::newRow("Custom") << "custom" << static_cast<int>(MnaFileRole::Custom);
}

void TestMnaSerialization::testFileRoleFromString()
{
    QFETCH(QString, input);
    QFETCH(int, expected);
    QCOMPARE(static_cast<int>(file_role_from_string(input.toStdString())), expected);
}

void TestMnaSerialization::testFileRoleFromStringUnknown()
{
    QCOMPARE(file_role_from_string("NonExistentRole"), MnaFileRole::Custom);
}

//=============================================================================================================
// MnaDataKind enum — data-driven
//=============================================================================================================

void TestMnaSerialization::testDataKindToString_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<QString>("expected");

    QTest::newRow("FiffRaw") << static_cast<int>(MnaDataKind::FiffRaw) << "FiffRaw";
    QTest::newRow("Forward") << static_cast<int>(MnaDataKind::Forward) << "Forward";
    QTest::newRow("Inverse") << static_cast<int>(MnaDataKind::Inverse) << "Inverse";
    QTest::newRow("Covariance") << static_cast<int>(MnaDataKind::Covariance) << "Covariance";
    QTest::newRow("SourceEstimate") << static_cast<int>(MnaDataKind::SourceEstimate) << "SourceEstimate";
    QTest::newRow("Epochs") << static_cast<int>(MnaDataKind::Epochs) << "Epochs";
    QTest::newRow("Evoked") << static_cast<int>(MnaDataKind::Evoked) << "Evoked";
    QTest::newRow("Matrix") << static_cast<int>(MnaDataKind::Matrix) << "Matrix";
    QTest::newRow("Volume") << static_cast<int>(MnaDataKind::Volume) << "Volume";
    QTest::newRow("Surface") << static_cast<int>(MnaDataKind::Surface) << "Surface";
    QTest::newRow("Bem") << static_cast<int>(MnaDataKind::Bem) << "Bem";
    QTest::newRow("Annotation") << static_cast<int>(MnaDataKind::Annotation) << "Annotation";
    QTest::newRow("Label") << static_cast<int>(MnaDataKind::Label) << "Label";
    QTest::newRow("RealTimeStream") << static_cast<int>(MnaDataKind::RealTimeStream) << "RealTimeStream";
    QTest::newRow("Custom") << static_cast<int>(MnaDataKind::Custom) << "Custom";
}

void TestMnaSerialization::testDataKindToString()
{
    QFETCH(int, kind);
    QFETCH(QString, expected);
    QCOMPARE(to_string(static_cast<MnaDataKind>(kind)), expected.toStdString());
}

void TestMnaSerialization::testDataKindFromString_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("expected");

    QTest::newRow("FiffRaw") << "FiffRaw" << static_cast<int>(MnaDataKind::FiffRaw);
    QTest::newRow("Forward") << "Forward" << static_cast<int>(MnaDataKind::Forward);
    QTest::newRow("Inverse") << "Inverse" << static_cast<int>(MnaDataKind::Inverse);
    QTest::newRow("Covariance") << "Covariance" << static_cast<int>(MnaDataKind::Covariance);
    QTest::newRow("SourceEstimate") << "SourceEstimate" << static_cast<int>(MnaDataKind::SourceEstimate);
    QTest::newRow("Epochs") << "Epochs" << static_cast<int>(MnaDataKind::Epochs);
    QTest::newRow("Evoked") << "Evoked" << static_cast<int>(MnaDataKind::Evoked);
    QTest::newRow("Matrix") << "Matrix" << static_cast<int>(MnaDataKind::Matrix);
    QTest::newRow("Volume") << "Volume" << static_cast<int>(MnaDataKind::Volume);
    QTest::newRow("Surface") << "Surface" << static_cast<int>(MnaDataKind::Surface);
    QTest::newRow("Bem") << "Bem" << static_cast<int>(MnaDataKind::Bem);
    QTest::newRow("Annotation") << "Annotation" << static_cast<int>(MnaDataKind::Annotation);
    QTest::newRow("Label") << "Label" << static_cast<int>(MnaDataKind::Label);
    QTest::newRow("RealTimeStream") << "RealTimeStream" << static_cast<int>(MnaDataKind::RealTimeStream);
    QTest::newRow("Custom") << "Custom" << static_cast<int>(MnaDataKind::Custom);
}

void TestMnaSerialization::testDataKindFromString()
{
    QFETCH(QString, input);
    QFETCH(int, expected);
    QCOMPARE(static_cast<int>(data_kind_from_string(input.toStdString())), expected);
}

void TestMnaSerialization::testDataKindFromStringUnknown()
{
    QCOMPARE(data_kind_from_string("NonExistentKind"), MnaDataKind::Custom);
    for (const char* other : {"FiffInfo", "FilePath", "DirectoryPath"})
        QCOMPARE(data_kind_from_string(other), MnaDataKind::Custom);

    // Every role and kind survives its string form; kinds also read back from their snake_case spelling
    for (int r = static_cast<int>(MnaFileRole::Raw); r <= static_cast<int>(MnaFileRole::Custom); ++r) {
        const auto role = static_cast<MnaFileRole>(r);
        QCOMPARE(file_role_from_string(to_string(role)), role);
    }
    for (int k = static_cast<int>(MnaDataKind::FiffRaw); k <= static_cast<int>(MnaDataKind::Custom); ++k) {
        const auto kind = static_cast<MnaDataKind>(k);
        QCOMPARE(data_kind_from_string(to_string(kind)), kind);
        QCOMPARE(data_kind_from_string(data_kind_key(kind)), kind);
    }
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
    original.size_bytes = 12345;
    original.embedded = true;
    original.data = bytes({0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD});
    original.sha256 = "abc123def456";

    JsonObject json = original.to_json();
    QVERIFY(json.contains("data"));
    QVERIFY(json["embedded"].to_bool());

    MnaFileRef restored = MnaFileRef::from_json(json);
    QCOMPARE(restored.role, original.role);
    QCOMPARE(restored.path, original.path);
    QCOMPARE(restored.embedded, true);
    QCOMPARE(restored.data, original.data);
    QCOMPARE(restored.size_bytes, original.size_bytes);
    QCOMPARE(restored.sha256, original.sha256);
}

void TestMnaSerialization::testFileRefEmbeddedCborRoundTrip()
{
    MnaFileRef original;
    original.role = MnaFileRole::Bem;
    original.path = "sub-01/bem/sample-bem.fif";
    original.format = "fif";
    original.size_bytes = 99999;
    original.embedded = true;
    original.data.assign(1024, std::byte{'X'}); // 1KB of 'X'

    CborMap cbor = original.to_cbor();
    MnaFileRef restored = MnaFileRef::from_cbor(cbor);
    QCOMPARE(restored.role, original.role);
    QCOMPARE(restored.path, original.path);
    QCOMPARE(restored.embedded, true);
    QCOMPARE(restored.data.size(), std::size_t(1024));
    QCOMPARE(restored.data, original.data);
}

void TestMnaSerialization::testFileRefEmbeddedLargeData()
{
    // Test with 100KB of random-ish data to verify base64/cbor handling at scale
    std::vector<std::byte> largeData(100 * 1024);
    for (std::size_t i = 0; i < largeData.size(); ++i)
        largeData[i] = static_cast<std::byte>(i % 256);

    MnaFileRef ref;
    ref.role = MnaFileRole::Raw;
    ref.path = "large_file.fif";
    ref.embedded = true;
    ref.data = largeData;
    ref.size_bytes = static_cast<std::int64_t>(largeData.size());

    // JSON round-trip
    JsonObject json = ref.to_json();
    MnaFileRef fromJ = MnaFileRef::from_json(json);
    QCOMPARE(fromJ.data.size(), largeData.size());
    QCOMPARE(fromJ.data, largeData);

    // CBOR round-trip
    CborMap cbor = ref.to_cbor();
    MnaFileRef fromC = MnaFileRef::from_cbor(cbor);
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

    JsonObject json = ref.to_json();
    QVERIFY(!json.contains("data"));

    MnaFileRef restored = MnaFileRef::from_json(json);
    QCOMPARE(restored.embedded, false);
    QVERIFY(restored.data.empty());
}

//=============================================================================================================
// MnaPort — stream and cached result fields
//=============================================================================================================

void TestMnaSerialization::testPortStreamFieldsJson()
{
    MnaPort port;
    port.name = "raw_stream";
    port.data_kind = MnaDataKind::RealTimeStream;
    port.direction = MnaPortDir::Input;
    port.stream_protocol = "lsl";
    port.stream_endpoint = "type='EEG'";
    port.stream_buffer_ms = 5000;

    JsonObject json = port.to_json();
    MnaPort restored = MnaPort::from_json(json);

    QCOMPARE(restored.stream_protocol, "lsl");
    QCOMPARE(restored.stream_endpoint, "type='EEG'");
    QCOMPARE(restored.stream_buffer_ms, 5000);
}

void TestMnaSerialization::testPortStreamFieldsCbor()
{
    MnaPort port;
    port.name = "fiff_rt";
    port.data_kind = MnaDataKind::RealTimeStream;
    port.direction = MnaPortDir::Output;
    port.stream_protocol = "fiff-rt";
    port.stream_endpoint = "localhost:4217";
    port.stream_buffer_ms = 2000;

    CborMap cbor = port.to_cbor();
    MnaPort restored = MnaPort::from_cbor(cbor);

    QCOMPARE(restored.stream_protocol, "fiff-rt");
    QCOMPARE(restored.stream_endpoint, "localhost:4217");
    QCOMPARE(restored.stream_buffer_ms, 2000);
}

void TestMnaSerialization::testPortCachedResultFields()
{
    MnaPort port;
    port.name = "inverse";
    port.data_kind = MnaDataKind::Inverse;
    port.direction = MnaPortDir::Output;
    port.cached_result_path = "/tmp/inv_cache.fif";
    port.cached_result_hash = "sha256:aabbccdd";

    JsonObject json = port.to_json();
    MnaPort restored = MnaPort::from_json(json);

    QCOMPARE(restored.cached_result_path, "/tmp/inv_cache.fif");
    QCOMPARE(restored.cached_result_hash, "sha256:aabbccdd");
}

//=============================================================================================================
// MnaScript
//=============================================================================================================

void TestMnaSerialization::testScriptDefaults()
{
    MnaScript script;
    QVERIFY(script.language.empty());
    QVERIFY(script.interpreter.empty());
    QVERIFY(script.interpreter_args.empty());
    QVERIFY(script.code.empty());
    QVERIFY(script.source_uri.empty());
    QVERIFY(script.code_sha256.empty());
    QCOMPARE(script.keep_temp_file, false);
}

void TestMnaSerialization::testScriptJsonRoundTrip()
{
    MnaScript original;
    original.language = "python";
    original.interpreter = "python3";
    original.interpreter_args = {"-u", "--no-site"};
    original.code = "import mne\nprint('hello')";
    original.source_uri = "file:///lab/scripts/hello.py";
    original.code_sha256 = "deadbeef1234";
    original.keep_temp_file = true;

    JsonObject json = original.to_json();
    MnaScript restored = MnaScript::from_json(json);

    QCOMPARE(restored.language, original.language);
    QCOMPARE(restored.interpreter, original.interpreter);
    QCOMPARE(restored.interpreter_args, original.interpreter_args);
    QCOMPARE(restored.code, original.code);
    QCOMPARE(restored.source_uri, original.source_uri);
    QCOMPARE(restored.code_sha256, original.code_sha256);
    QCOMPARE(restored.keep_temp_file, true);
}

void TestMnaSerialization::testScriptCborRoundTrip()
{
    MnaScript original;
    original.language = "shell";
    original.interpreter = "/bin/bash";
    original.code = "echo 'test'";
    original.keep_temp_file = false;

    CborMap cbor = original.to_cbor();
    MnaScript restored = MnaScript::from_cbor(cbor);

    QCOMPARE(restored.language, original.language);
    QCOMPARE(restored.interpreter, original.interpreter);
    QCOMPARE(restored.code, original.code);
    QCOMPARE(restored.keep_temp_file, false);
}

void TestMnaSerialization::testScriptWithAllFields()
{
    MnaScript script;
    script.language = "r";
    script.interpreter = "Rscript";
    script.interpreter_args = {"--vanilla"};
    script.code = "library(tidyverse)\nprint('R script')";
    script.source_uri = "file:///scripts/analysis.R";
    script.code_sha256 = "abcdef0123456789";
    script.keep_temp_file = true;

    // JSON
    JsonObject json = script.to_json();
    MnaScript rJ = MnaScript::from_json(json);
    QCOMPARE(rJ.language, "r");
    QCOMPARE(rJ.interpreter_args.size(), std::size_t(1));
    QCOMPARE(rJ.interpreter_args.front(), "--vanilla");

    // CBOR
    CborMap cbor = script.to_cbor();
    MnaScript rC = MnaScript::from_cbor(cbor);
    QCOMPARE(rC.language, "r");
    QCOMPARE(rC.code_sha256, "abcdef0123456789");
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
    check.on_fail = "Recompute noise covariance with longer data";

    JsonObject json = check.to_json();
    MnaVerificationCheck restored = MnaVerificationCheck::from_json(json);

    QCOMPARE(restored.id, check.id);
    QCOMPARE(restored.description, check.description);
    QCOMPARE(restored.phase, "post");
    QCOMPARE(restored.expression, check.expression);
    QCOMPARE(restored.severity, "error");
    QCOMPARE(restored.on_fail, check.on_fail);
}

void TestMnaSerialization::testVerificationCheckCborRoundTrip()
{
    MnaVerificationCheck check;
    check.id = "snr_threshold";
    check.description = "SNR must be above 3";
    check.phase = "post";
    check.expression = "snr >= 3.0";
    check.severity = "warning";

    CborMap cbor = check.to_cbor();
    MnaVerificationCheck restored = MnaVerificationCheck::from_cbor(cbor);

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

    JsonObject json = check.to_json();
    MnaVerificationCheck restored = MnaVerificationCheck::from_json(json);

    QCOMPARE(restored.script.language, "python");
    QCOMPARE(restored.script.code, "import sys; sys.exit(0)");
}

//=============================================================================================================
// MnaVerificationResult
//=============================================================================================================

void TestMnaSerialization::testVerificationResultJsonRoundTrip()
{
    MnaVerificationResult result;
    result.check_id = "cov_posdef";
    result.passed = true;
    result.severity = "error";
    result.message = "PASS: covariance is positive-definite";
    result.actual_value = 64;
    result.evaluated_at = utc(2026, 4, 19, 12, 0, 0);

    JsonObject json = result.to_json();
    MnaVerificationResult restored = MnaVerificationResult::from_json(json);

    QCOMPARE(restored.check_id, "cov_posdef");
    QCOMPARE(restored.passed, true);
    QCOMPARE(restored.severity, "error");
    QCOMPARE(restored.message, result.message);
    QCOMPARE(restored.actual_value.to_int(), 64);
}

void TestMnaSerialization::testVerificationResultCborRoundTrip()
{
    MnaVerificationResult result;
    result.check_id = "snr_check";
    result.passed = false;
    result.severity = "warning";
    result.message = "FAIL [warning]: SNR = 1.5 < 3.0";
    result.actual_value = 1.5;
    result.evaluated_at = now();

    CborMap cbor = result.to_cbor();
    MnaVerificationResult restored = MnaVerificationResult::from_cbor(cbor);

    QCOMPARE(restored.check_id, "snr_check");
    QCOMPARE(restored.passed, false);
    QCOMPARE(restored.severity, "warning");
}

//=============================================================================================================
// MnaProvenance
//=============================================================================================================

void TestMnaSerialization::testProvenanceJsonRoundTrip()
{
    MnaProvenance prov;
    prov.mne_cpp_version = "2.2.0";
    prov.compiler_info = "AppleClang 16.0.0";
    prov.os_info = "macOS 15.4 arm64";
    prov.host_name = "lab-workstation";
    prov.wall_time_ms = 12345;
    prov.peak_memory_bytes = 256 * 1024 * 1024;
    prov.random_seed = 42;
    prov.started_at = utc(2026, 4, 19, 10, 0, 0);
    prov.finished_at = utc(2026, 4, 19, 10, 0, 12);

    prov.input_hashes["raw"] = "sha256:aabb";
    prov.input_hashes["noise_cov"] = "sha256:ccdd";
    prov.resolved_attributes["lambda2"] = 0.111;
    prov.resolved_attributes["method"] = "dSPM";

    JsonObject json = prov.to_json();
    MnaProvenance restored = MnaProvenance::from_json(json);

    QCOMPARE(restored.mne_cpp_version, "2.2.0");
    QCOMPARE(restored.compiler_info, "AppleClang 16.0.0");
    QCOMPARE(restored.host_name, "lab-workstation");
    QCOMPARE(restored.wall_time_ms, std::int64_t(12345));
    QCOMPARE(restored.peak_memory_bytes, std::int64_t(256 * 1024 * 1024));
    QCOMPARE(restored.random_seed, std::int64_t(42));
    QCOMPARE(restored.input_hashes.size(), std::size_t(2));
    QCOMPARE(restored.input_hashes["raw"], "sha256:aabb");
    QCOMPARE(restored.resolved_attributes["method"].to_std_string(), std::string("dSPM"));
}

void TestMnaSerialization::testProvenanceCborRoundTrip()
{
    MnaProvenance prov;
    prov.mne_cpp_version = "2.2.0";
    prov.wall_time_ms = 9876;
    prov.external_tool_version = "FreeSurfer 7.4.1";

    CborMap cbor = prov.to_cbor();
    MnaProvenance restored = MnaProvenance::from_cbor(cbor);

    QCOMPARE(restored.mne_cpp_version, "2.2.0");
    QCOMPARE(restored.wall_time_ms, std::int64_t(9876));
    QCOMPARE(restored.external_tool_version, "FreeSurfer 7.4.1");
}

void TestMnaSerialization::testProvenanceAllFields()
{
    MnaProvenance prov;
    prov.mne_cpp_version = "2.2.0";
    prov.compiler_info = "GCC 13.2";
    prov.os_info = "Ubuntu 24.04 x86_64";
    prov.host_name = "cluster-node-01";
    prov.external_tool_version = "Python 3.11.5";
    prov.started_at = now();
    prov.finished_at = now() + std::chrono::seconds(60);
    prov.wall_time_ms = 60000;
    prov.peak_memory_bytes = 1024 * 1024 * 1024;
    prov.random_seed = 12345;
    prov.input_hashes["epochs"] = "sha256:1111";
    prov.resolved_attributes["n_components"] = 20;

    // JSON round-trip
    JsonObject json = prov.to_json();
    MnaProvenance rJ = MnaProvenance::from_json(json);
    QCOMPARE(rJ.host_name, "cluster-node-01");
    QCOMPARE(rJ.external_tool_version, "Python 3.11.5");
    QCOMPARE(rJ.peak_memory_bytes, std::int64_t(1024 * 1024 * 1024));

    // CBOR round-trip
    CborMap cbor = prov.to_cbor();
    MnaProvenance rC = MnaProvenance::from_cbor(cbor);
    QCOMPARE(rC.os_info, "Ubuntu 24.04 x86_64");
    QCOMPARE(rC.random_seed, std::int64_t(12345));
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
    verif.checks.push_back(check1);

    MnaVerificationResult preRes;
    preRes.check_id = "cov_check";
    preRes.passed = true;
    preRes.severity = "error";
    preRes.message = "PASS";
    verif.pre_results.push_back(preRes);

    verif.provenance.mne_cpp_version = "2.2.0";
    verif.provenance.wall_time_ms = 5000;

    JsonObject json = verif.to_json();
    MnaVerification restored = MnaVerification::from_json(json);

    QCOMPARE(restored.explanation, "Compute dSPM inverse solution");
    QCOMPARE(restored.checks.size(), std::size_t(1));
    QCOMPARE(restored.checks[0].id, "cov_check");
    QCOMPARE(restored.pre_results.size(), std::size_t(1));
    QCOMPARE(restored.pre_results[0].passed, true);
    QCOMPARE(restored.provenance.mne_cpp_version, "2.2.0");
}

void TestMnaSerialization::testVerificationCborRoundTrip()
{
    MnaVerification verif;
    verif.explanation = "BEM solution check";

    MnaVerificationCheck check;
    check.id = "mesh_closed";
    check.phase = "post";
    check.severity = "error";
    verif.checks.push_back(check);

    MnaVerificationResult postRes;
    postRes.check_id = "mesh_closed";
    postRes.passed = true;
    postRes.severity = "error";
    verif.post_results.push_back(postRes);

    CborMap cbor = verif.to_cbor();
    MnaVerification restored = MnaVerification::from_cbor(cbor);

    QCOMPARE(restored.explanation, "BEM solution check");
    QCOMPARE(restored.checks.size(), std::size_t(1));
    QCOMPARE(restored.post_results.size(), std::size_t(1));
}

void TestMnaSerialization::testVerificationRoundTripsEveryField()
{
    const UtcTime t0 = utc(2026, 10, 12, 9, 30, 15, 250);
    MnaVerification full;
    full.explanation = "Inverse operator checks";
    MnaVerificationCheck check;
    check.id = "cov_posdef";
    check.description = "Covariance matrix must be positive-definite";
    check.phase = "pre";
    check.expression = "rank > 0";
    check.script.language = "python";
    check.script.interpreter = "python3";
    check.script.interpreter_args = {"-u"};
    check.script.code = "import sys; sys.exit(0)";
    check.script.source_uri = "file:///lab/check.py";
    check.script.code_sha256 = "abc123";
    check.script.keep_temp_file = true;
    check.severity = "warning";
    check.on_fail = "Regularize the covariance";
    full.checks = {check, MnaVerificationCheck()};
    MnaVerificationResult pre;
    pre.check_id = "cov_posdef";
    pre.passed = true;
    pre.severity = "warning";
    pre.message = "PASS: rank > 0";
    pre.actual_value = 305;
    pre.evaluated_at = t0;
    MnaVerificationResult post;
    post.check_id = "sol_finite";
    post.message = "FAIL [error]: not finite";
    post.actual_value = "nan";
    full.pre_results = {pre};
    full.post_results = {post, MnaVerificationResult()};
    MnaProvenance& prov = full.provenance;
    prov.input_hashes = {{"fwd", "sha256:1"}, {"cov", "sha256:2"}};
    prov.resolved_attributes = {{"snr", 3.0}, {"method", "dSPM"}, {"pick_ori", nullptr}};
    prov.mne_cpp_version = "2.4.0";
    prov.compiler_info = "AppleClang 17";
    prov.os_info = "macOS 26 arm64";
    prov.host_name = "lab-01";
    prov.external_tool_version = "mne 1.11.0";
    prov.started_at = t0;
    prov.finished_at = t0 + std::chrono::milliseconds(1500);
    prov.wall_time_ms = 1500;
    prov.peak_memory_bytes = 123456789;
    prov.random_seed = 0;

    const auto same = [](const MnaVerification& a, const MnaVerification& b) {
        QCOMPARE(b.explanation, a.explanation);
        QCOMPARE(b.checks.size(), a.checks.size());
        for (std::size_t i = 0; i < a.checks.size(); ++i) {
            const MnaVerificationCheck &x = a.checks[i], &y = b.checks[i];
            QCOMPARE(y.id, x.id);
            QCOMPARE(y.description, x.description);
            QCOMPARE(y.phase, x.phase);
            QCOMPARE(y.expression, x.expression);
            QCOMPARE(y.severity, x.severity);
            QCOMPARE(y.on_fail, x.on_fail);
            QCOMPARE(y.script.language, x.script.language);
            QCOMPARE(y.script.interpreter, x.script.interpreter);
            QCOMPARE(y.script.interpreter_args, x.script.interpreter_args);
            QCOMPARE(y.script.code, x.script.code);
            QCOMPARE(y.script.source_uri, x.script.source_uri);
            QCOMPARE(y.script.code_sha256, x.script.code_sha256);
            QCOMPARE(y.script.keep_temp_file, x.script.keep_temp_file);
        }
        for (const auto& [as, bs] : {std::pair{&a.pre_results, &b.pre_results}, std::pair{&a.post_results, &b.post_results}}) {
            QCOMPARE(bs->size(), as->size());
            for (std::size_t i = 0; i < as->size(); ++i) {
                const MnaVerificationResult &x = as->at(i), &y = bs->at(i);
                QCOMPARE(y.check_id, x.check_id);
                QCOMPARE(y.passed, x.passed);
                QCOMPARE(y.severity, x.severity);
                QCOMPARE(y.message, x.message);
                QCOMPARE(y.actual_value, x.actual_value);
                QCOMPARE(y.evaluated_at, x.evaluated_at);
            }
        }
        const MnaProvenance &x = a.provenance, &y = b.provenance;
        QCOMPARE(y.input_hashes, x.input_hashes);
        QCOMPARE(y.resolved_attributes, x.resolved_attributes);
        QCOMPARE(y.mne_cpp_version, x.mne_cpp_version);
        QCOMPARE(y.compiler_info, x.compiler_info);
        QCOMPARE(y.os_info, x.os_info);
        QCOMPARE(y.host_name, x.host_name);
        QCOMPARE(y.external_tool_version, x.external_tool_version);
        QCOMPARE(y.started_at, x.started_at);
        QCOMPARE(y.finished_at, x.finished_at);
        QCOMPARE(y.wall_time_ms, x.wall_time_ms);
        QCOMPARE(y.peak_memory_bytes, x.peak_memory_bytes);
        QCOMPARE(y.random_seed, x.random_seed);
    };
    for (const MnaVerification& v : {full, MnaVerification()}) {
        same(v, MnaVerification::from_json(v.to_json()));
        if (QTest::currentTestFailed())
            return;
        same(v, MnaVerification::from_cbor(v.to_cbor()));
        if (QTest::currentTestFailed())
            return;
    }
}

//=============================================================================================================
// MnaNode with IPC / Script exec modes and verification
//=============================================================================================================

void TestMnaSerialization::testNodeExecModeIpcJson()
{
    MnaNode node;
    node.id = "freesurfer_recon";
    node.op_type = "recon_all";
    node.exec_mode = MnaNodeExecMode::Ipc;
    node.ipc_command = "recon-all";
    node.ipc_args = {"-s", "sub-01", "-all"};
    node.ipc_work_dir = "/data/freesurfer";
    node.ipc_transport = "subprocess";

    JsonObject json = node.to_json();
    MnaNode restored = MnaNode::from_json(json);

    QCOMPARE(restored.exec_mode, MnaNodeExecMode::Ipc);
    QCOMPARE(restored.ipc_command, "recon-all");
    QCOMPARE(restored.ipc_args.size(), std::size_t(3));
    QCOMPARE(restored.ipc_work_dir, "/data/freesurfer");
    QCOMPARE(restored.ipc_transport, "subprocess");
}

void TestMnaSerialization::testNodeExecModeScriptJson()
{
    MnaNode node;
    node.id = "py_bandpass";
    node.op_type = "custom_filter";
    node.exec_mode = MnaNodeExecMode::Script;
    node.script.language = "python";
    node.script.interpreter = "python3";
    node.script.code = "import mne\nraw.filter(1, 40)";

    JsonObject json = node.to_json();
    MnaNode restored = MnaNode::from_json(json);

    QCOMPARE(restored.exec_mode, MnaNodeExecMode::Script);
    QCOMPARE(restored.script.language, "python");
    QCOMPARE(restored.script.code, "import mne\nraw.filter(1, 40)");
}

void TestMnaSerialization::testNodeExecModeCbor_data()
{
    QTest::addColumn<int>("exec_mode");
    QTest::addColumn<QString>("label");

    QTest::newRow("Batch") << static_cast<int>(MnaNodeExecMode::Batch) << "Batch";
    QTest::newRow("Stream") << static_cast<int>(MnaNodeExecMode::Stream) << "Stream";
    QTest::newRow("Ipc") << static_cast<int>(MnaNodeExecMode::Ipc) << "Ipc";
    QTest::newRow("Script") << static_cast<int>(MnaNodeExecMode::Script) << "Script";
}

void TestMnaSerialization::testNodeExecModeCbor()
{
    QFETCH(int, exec_mode);

    MnaNode node;
    node.id = "test_node";
    node.op_type = "test_op";
    node.exec_mode = static_cast<MnaNodeExecMode>(exec_mode);

    CborMap cbor = node.to_cbor();
    MnaNode restored = MnaNode::from_cbor(cbor);

    QCOMPARE(static_cast<int>(restored.exec_mode), exec_mode);
}

void TestMnaSerialization::testNodeRoundTripsEveryField_data()
{
    QTest::addColumn<int>("exec_mode");
    QTest::addColumn<bool>("cbor");

    const std::pair<const char*, MnaNodeExecMode> modes[] = {{"batch", MnaNodeExecMode::Batch},
                                                             {"stream", MnaNodeExecMode::Stream},
                                                             {"ipc", MnaNodeExecMode::Ipc},
                                                             {"script", MnaNodeExecMode::Script}};
    for (const auto& [name, mode] : modes) {
        QTest::addRow("%s-json", name) << static_cast<int>(mode) << false;
        QTest::addRow("%s-cbor", name) << static_cast<int>(mode) << true;
    }
}

void TestMnaSerialization::testNodeRoundTripsEveryField()
{
    QFETCH(int, exec_mode);
    QFETCH(bool, cbor);

    MnaNode node;
    node.id = "node";
    node.op_type = "op";
    node.exec_mode = static_cast<MnaNodeExecMode>(exec_mode);
    node.attributes = {{"lambda", 0.1}, {"method", "dSPM"}};
    node.ipc_command = "tool";
    node.ipc_args = {"--in", "{{input}}"};
    node.ipc_work_dir = "/tmp";
    node.ipc_transport = "stdio";
    node.script.language = "python";
    node.script.interpreter = "python3";
    node.script.code = "print(1)";
    node.tool_version = "2.4.0";
    node.executed_at = utc(2026, 10, 19, 12, 30, 0);
    node.dirty = false;
    node.extras.insert("x_custom", 42);

    for (int kind = 0; kind <= static_cast<int>(MnaDataKind::Custom); ++kind) {
        MnaPort port;
        port.name = std::format("p{}", kind);
        port.data_kind = static_cast<MnaDataKind>(kind);
        port.direction = kind % 2 ? MnaPortDir::Output : MnaPortDir::Input;
        port.source_node_id = "upstream";
        port.source_port_name = "out";
        port.stream_protocol = "lsl";
        port.stream_endpoint = "type='EEG'";
        port.stream_buffer_ms = 100 + kind;
        port.cached_result_path = "cache.fif";
        port.cached_result_hash = "sha256:abc";
        port.extras.insert("x_port", kind);
        (kind % 2 ? node.outputs : node.inputs).push_back(port);
    }

    const MnaNode restored = cbor ? MnaNode::from_cbor(node.to_cbor()) : MnaNode::from_json(node.to_json());

    // Fields that are only written for the matching exec mode do not survive the others.
    const bool ipc = node.exec_mode == MnaNodeExecMode::Ipc;
    QCOMPARE(restored.ipc_args, ipc ? node.ipc_args : StringList());
    QCOMPARE(restored.script.code, node.exec_mode == MnaNodeExecMode::Script ? node.script.code : std::string());
    QCOMPARE(restored.executed_at, node.executed_at);
    QCOMPARE(restored.dirty, false);
    QCOMPARE(restored.extras["x_custom"].to_int(), 42);
    QCOMPARE(restored.attributes, node.attributes);
    QCOMPARE(restored.inputs.size() + restored.outputs.size(), static_cast<std::size_t>(MnaDataKind::Custom) + 1);
    for (const std::vector<MnaPort>* ports : {&node.inputs, &node.outputs}) {
        const std::vector<MnaPort>& got = ports == &node.inputs ? restored.inputs : restored.outputs;
        for (std::size_t i = 0; i < ports->size(); ++i)
            QCOMPARE(got[i].to_json(), (*ports)[i].to_json());
    }
}

void TestMnaSerialization::testNodeWithVerificationRoundTrip()
{
    MnaNode node;
    node.id = "inverse_compute";
    node.op_type = "mne_compute_mne";
    node.tool_version = "2.2.0";
    node.executed_at = now();
    node.dirty = true;

    // Add verification
    MnaVerificationCheck check;
    check.id = "inv_rank";
    check.phase = "post";
    check.expression = "rank > 0";
    check.severity = "error";
    node.verification.checks.push_back(check);
    node.verification.explanation = "MNE inverse computation";
    node.verification.provenance.mne_cpp_version = "2.2.0";

    // JSON
    JsonObject json = node.to_json();
    MnaNode rJ = MnaNode::from_json(json);
    QCOMPARE(rJ.verification.explanation, "MNE inverse computation");
    QCOMPARE(rJ.verification.checks.size(), std::size_t(1));
    QCOMPARE(rJ.dirty, true);

    // CBOR
    CborMap cbor = node.to_cbor();
    MnaNode rC = MnaNode::from_cbor(cbor);
    QCOMPARE(rC.verification.checks[0].id, "inv_rank");
    QCOMPARE(rC.verification.provenance.mne_cpp_version, "2.2.0");
}

//=============================================================================================================
// MnaParamBinding
//=============================================================================================================

void TestMnaSerialization::testParamBindingDefaults()
{
    MnaParamBinding binding;
    QVERIFY(binding.target_path.empty());
    QVERIFY(binding.expression.empty());
    QVERIFY(binding.trigger.empty());
    QCOMPARE(binding.period_ms, 0);
    QVERIFY(binding.dependencies.empty());
}

void TestMnaSerialization::testParamBindingJsonRoundTrip()
{
    MnaParamBinding binding;
    binding.target_path = "inverse_compute/lambda2";
    binding.expression = "clamp(ref('noise_est/snr') * 0.1, 0.01, 1.0)";
    binding.trigger = "on_change";
    binding.period_ms = 0;
    binding.dependencies = {"noise_est/snr"};

    JsonObject json = binding.to_json();
    MnaParamBinding restored = MnaParamBinding::from_json(json);

    QCOMPARE(restored.target_path, binding.target_path);
    QCOMPARE(restored.expression, binding.expression);
    QCOMPARE(restored.trigger, "on_change");
    QCOMPARE(restored.dependencies.size(), std::size_t(1));
    QCOMPARE(restored.dependencies.front(), "noise_est/snr");
}

void TestMnaSerialization::testParamBindingCborRoundTrip()
{
    MnaParamBinding binding;
    binding.target_path = "filter/cutoff_hz";
    binding.expression = "40.0";
    binding.trigger = "periodic";
    binding.period_ms = 1000;
    binding.dependencies = {"input/sfreq", "input/nfft"};

    CborMap cbor = binding.to_cbor();
    MnaParamBinding restored = MnaParamBinding::from_cbor(cbor);

    QCOMPARE(restored.target_path, "filter/cutoff_hz");
    QCOMPARE(restored.trigger, "periodic");
    QCOMPARE(restored.period_ms, 1000);
    QCOMPARE(restored.dependencies.size(), std::size_t(2));

    // Both encodings store a period only for a periodic trigger with one, and dependencies only when there are some
    binding.dependencies.clear();
    for (const std::string trigger : {"periodic", "on_change"}) {
        binding.trigger = trigger;
        for (const int period : {0, 250}) {
            binding.period_ms = period;
            const int stored = trigger == "periodic" ? period : 0;
            QCOMPARE(binding.to_cbor().contains("period_ms"), stored > 0);
            QCOMPARE(binding.to_json().contains("period_ms"), stored > 0);
            QCOMPARE(MnaParamBinding::from_cbor(binding.to_cbor()).period_ms, stored);
            QCOMPARE(MnaParamBinding::from_json(binding.to_json()).period_ms, stored);
            QVERIFY(!binding.to_cbor().contains("dependencies"));
            QVERIFY(!binding.to_json().contains("dependencies"));
        }
    }
}

//=============================================================================================================
// MnaParamTree
//=============================================================================================================

void TestMnaSerialization::testParamTreeSetGet()
{
    MnaParamTree tree;
    tree.set_param("inverse/lambda2", 0.111);
    tree.set_param("inverse/method", "dSPM");
    tree.set_param("filter/lfreq", 1.0);

    QCOMPARE(tree.param("inverse/lambda2").to_double(), 0.111);
    QCOMPARE(tree.param("inverse/method").to_std_string(), std::string("dSPM"));
    QCOMPARE(tree.param("filter/lfreq").to_double(), 1.0);
    QVERIFY(tree.param("nonexistent").is_undefined());
}

void TestMnaSerialization::testParamTreeHasParam()
{
    MnaParamTree tree;
    QVERIFY(!tree.has_param("x"));
    tree.set_param("x", 42);
    QVERIFY(tree.has_param("x"));
}

void TestMnaSerialization::testParamTreeAllPaths()
{
    MnaParamTree tree;
    tree.set_param("a/b", 1);
    tree.set_param("a/c", 2);
    tree.set_param("d", 3);

    QCOMPARE(tree.all_paths(), (StringList{"a/b", "a/c", "d"}));
}

void TestMnaSerialization::testParamTreeBindings()
{
    MnaParamTree tree;

    MnaParamBinding b1;
    b1.target_path = "node1/attr1";
    b1.expression = "42";
    b1.trigger = "manual";
    tree.add_binding(b1);

    QVERIFY(tree.has_binding("node1/attr1"));
    QCOMPARE(tree.bindings().size(), 1);

    tree.remove_binding("node1/attr1");
    QVERIFY(!tree.has_binding("node1/attr1"));
    QCOMPARE(tree.bindings().size(), 0);
}

void TestMnaSerialization::testParamTreeEvaluateExpression()
{
    MnaParamTree tree;

    // ref() expression that looks up from results map
    JsonObject results;
    results["step1::output"] = 42;

    const JsonValue val = tree.evaluate_expression("ref('step1::output')", results);
    QCOMPARE(val.to_int(), 42);

    // An unresolved ref stays unresolved inside functions instead of evaluating as 0
    QVERIFY(tree.evaluate_expression("clamp(ref('missing::out') * 2, 1, 5)", results).is_undefined());

    tree.set_param("p/gain", 2.0);
    results["a::x"] = 3.0;
    results["zero"] = 0.0;
    auto eval = [&](std::string_view expr) {
        return tree.evaluate_expression(expr, results);
    };

    const std::pair<std::string_view, bool> comparisons[] = {
        {"1 < 2", true}, {"4 < 3", false}, {"2 <= 2", true}, {"3 >= 4", false}, {"2 == 2", true}, {"2 != 2", false}, {"5 > 4", true}, {"ref('a::x') > 2", true}, {"p/gain >= 2.5", false}, {" true ", true}, {"false", false}};
    for (const auto& c : comparisons) {
        const JsonValue v = eval(c.first);
        QVERIFY(v.is_bool());
        QCOMPARE(v.to_bool(), c.second);
    }
    QVERIFY(eval("ref('nope') > 1").is_undefined());
    QVERIFY(eval("1 < ref('nope')").is_undefined());

    const std::pair<std::string_view, double> numeric[] = {
        {"ref('p/gain')", 2.0}, {"clamp(ref('a::x'), 0, 1)", 1.0}, {"clamp(-5, 0, 1)", 0.0}, {"clamp(0.5, 0, 1)", 0.5}, {"scale(ref('a::x'), 0.5)", 1.5}, {"threshold(ref('a::x'), 2.5, 10, 20)", 10.0}, {"threshold(ref('a::x'), 3.5, 10, 20)", 20.0}, {"lerp(0, 10, 0.25)", 2.5}, {" 1e-3 ", 1e-3}, {"p/gain", 2.0}, {"a::x", 3.0}, {"ref('a::x') * 4", 12.0}, {"1 / (ref('a::x') + 1)", 0.25}, {"ref('a::x') + 0.5", 3.5}};
    for (const auto& n : numeric) {
        const JsonValue v = eval(n.first);
        QVERIFY2(v.is_number(), n.first.data());
        QCOMPARE(v.to_double(), n.second);
    }

    const std::string_view invalid[] = {"scale(ref('nope'), 2)", "threshold(ref('nope'), 1, 2, 3)", "lerp(0, 10, ref('nope'))", "unknown_name",
                                        "ref('nope') * 4", "2 / (ref('zero'))", "2 / (ref('nope'))", "ref('nope') + 1", "foo bar"};
    for (const std::string_view expr : invalid) {
        QVERIFY2(eval(expr).is_undefined(), expr.data());
    }
}

void TestMnaSerialization::testParamTreeEvaluateBindings()
{
    MnaParamTree tree;
    tree.set_param("out/d", 5.0);

    const std::vector<StringList> specs = {{"out/a", "ref('a::x') * 2", "on_change"},
                                           {"out/b", "ref('nope')", "on_change"},
                                           {"out/c", "7", "manual"},
                                           {"out/d", "5", "periodic"}};
    for (const StringList& s : specs) {
        MnaParamBinding b;
        b.target_path = s[0];
        b.expression = s[1];
        b.trigger = s[2];
        tree.add_binding(b);
    }

    JsonObject results;
    results["a::x"] = 3.0;
    QCOMPARE(tree.evaluate(results), StringList{"out/a"});
    QCOMPARE(tree.param("out/a").to_double(), 6.0);
    QVERIFY(!tree.has_param("out/b"));
    QVERIFY(!tree.has_param("out/c"));
    QCOMPARE(tree.param("out/d").to_double(), 5.0);
    QVERIFY(tree.evaluate(results).empty());
}

void TestMnaSerialization::testParamTreeJsonRoundTrip()
{
    MnaParamTree tree;
    tree.set_param("inverse/lambda2", 0.111);
    tree.set_param("inverse/method", "dSPM");

    MnaParamBinding b;
    b.target_path = "inverse/snr";
    b.expression = "3.0";
    b.trigger = "manual";
    tree.add_binding(b);

    JsonObject json = tree.to_json();
    MnaParamTree restored = MnaParamTree::from_json(json);

    // Unbound params survive round-trip
    QCOMPARE(restored.param("inverse/lambda2").to_double(), 0.111);
    QCOMPARE(restored.param("inverse/method").to_std_string(), std::string("dSPM"));
    // Binding survives round-trip
    QVERIFY(restored.has_binding("inverse/snr"));
}

//=============================================================================================================
// MnaOpSchema
//=============================================================================================================

void TestMnaSerialization::testOpSchemaValidateValidNode()
{
    MnaOpSchema schema;
    schema.op_type = "my_filter";

    MnaOpSchemaPort inPort;
    inPort.name = "raw";
    inPort.data_kind = MnaDataKind::FiffRaw;
    inPort.required = true;
    schema.input_ports.push_back(inPort);

    MnaOpSchemaPort outPort;
    outPort.name = "filtered";
    outPort.data_kind = MnaDataKind::FiffRaw;
    outPort.required = true;
    schema.output_ports.push_back(outPort);

    MnaOpSchemaAttr attr;
    attr.name = "cutoff_hz";
    attr.type = MnaAttrType::Double;
    attr.required = true;
    schema.attributes.push_back(attr);

    // Build a valid node
    MnaNode node;
    node.op_type = "my_filter";
    MnaPort in;
    in.name = "raw";
    in.data_kind = MnaDataKind::FiffRaw;
    in.direction = MnaPortDir::Input;
    MnaPort out;
    out.name = "filtered";
    out.data_kind = MnaDataKind::FiffRaw;
    out.direction = MnaPortDir::Output;
    node.inputs.push_back(in);
    node.outputs.push_back(out);
    node.attributes["cutoff_hz"] = 40.0;

    QCOMPARE(schema.validate(node), StringList());
}

void TestMnaSerialization::testOpSchemaValidateMissingInput()
{
    MnaOpSchema schema;
    schema.op_type = "my_op";

    MnaOpSchemaPort reqPort;
    reqPort.name = "epochs";
    reqPort.data_kind = MnaDataKind::Epochs;
    reqPort.required = true;
    schema.input_ports.push_back(reqPort);

    MnaNode node;
    node.op_type = "my_op";
    // No inputs

    QVERIFY(!schema.validate(node).empty());
}

void TestMnaSerialization::testOpSchemaValidateMissingAttribute()
{
    MnaOpSchema schema;
    schema.op_type = "my_op";

    MnaOpSchemaAttr reqAttr;
    reqAttr.name = "n_components";
    reqAttr.type = MnaAttrType::Int;
    reqAttr.required = true;
    schema.attributes.push_back(reqAttr);

    MnaNode node;
    node.op_type = "my_op";
    // Missing required attribute

    QVERIFY(!schema.validate(node).empty());
}

void TestMnaSerialization::testOpSchemaValidateExtraInputOk()
{
    // Extra inputs beyond schema should not fail validation
    MnaOpSchema schema;
    schema.op_type = "simple_op";

    MnaNode node;
    node.op_type = "simple_op";
    MnaPort extra;
    extra.name = "extra_input";
    extra.direction = MnaPortDir::Input;
    node.inputs.push_back(extra);

    QVERIFY(schema.validate(node).empty());
}

//=============================================================================================================
// MnaGraph serialization
//=============================================================================================================

void TestMnaSerialization::testGraphJsonRoundTrip()
{
    MnaGraph graph;

    MnaNode n1;
    n1.id = "read_raw";
    n1.op_type = "fiff_read_raw";
    MnaPort n1out;
    n1out.name = "raw";
    n1out.data_kind = MnaDataKind::FiffRaw;
    n1out.direction = MnaPortDir::Output;
    n1.outputs.push_back(n1out);
    graph.add_node(n1);

    MnaNode n2;
    n2.id = "filter";
    n2.op_type = "bandpass_filter";
    MnaPort n2in;
    n2in.name = "raw";
    n2in.data_kind = MnaDataKind::FiffRaw;
    n2in.direction = MnaPortDir::Input;
    n2.inputs.push_back(n2in);
    MnaPort n2out;
    n2out.name = "filtered";
    n2out.data_kind = MnaDataKind::FiffRaw;
    n2out.direction = MnaPortDir::Output;
    n2.outputs.push_back(n2out);
    graph.add_node(n2);

    graph.connect("read_raw", "raw", "filter", "raw");

    JsonObject json = graph.to_json();
    MnaGraph restored = MnaGraph::from_json(json);

    QCOMPARE(restored.nodes().size(), 2);
    QVERIFY(restored.has_node("read_raw"));
    QVERIFY(restored.has_node("filter"));
}

void TestMnaSerialization::testGraphCborRoundTrip()
{
    MnaGraph graph;

    MnaNode n1;
    n1.id = "src";
    n1.op_type = "source_op";
    MnaPort n1out;
    n1out.name = "data";
    n1out.direction = MnaPortDir::Output;
    n1.outputs.push_back(n1out);
    graph.add_node(n1);

    CborMap cbor = graph.to_cbor();
    MnaGraph restored = MnaGraph::from_cbor(cbor);

    QCOMPARE(restored.nodes().size(), 1);
    QVERIFY(restored.has_node("src"));
}

void TestMnaSerialization::testGraphWithParamTree()
{
    MnaGraph graph;
    graph.param_tree.set_param("global/sfreq", 1000.0);
    graph.param_tree.set_param("global/n_channels", 306);

    MnaNode n1;
    n1.id = "node1";
    n1.op_type = "op1";
    graph.add_node(n1);

    MnaPort in;
    in.name = "raw";
    graph.graph_inputs.push_back(in);
    MnaPort out;
    out.name = "stc";
    out.direction = MnaPortDir::Output;
    graph.graph_outputs.push_back(out);

    // The parameter tree and the graph ports survive both encodings
    for (const MnaGraph& restored : {MnaGraph::from_json(graph.to_json()), MnaGraph::from_cbor(graph.to_cbor())}) {
        QCOMPARE(restored.param_tree.param("global/sfreq").to_double(), 1000.0);
        QCOMPARE(restored.param_tree.param("global/n_channels").to_int(), 306);
        QCOMPARE(restored.graph_inputs.size(), std::size_t(1));
        QCOMPARE(restored.graph_inputs[0].name, std::string("raw"));
        QCOMPARE(restored.graph_outputs.size(), std::size_t(1));
        QCOMPARE(restored.graph_outputs[0].name, std::string("stc"));
    }
}

//=============================================================================================================
// Full project with embedded data → file round-trip
//=============================================================================================================

void TestMnaSerialization::testProjectEmbeddedMnxRoundTrip()
{
    MnaProject proj;
    proj.name = "Embedded Test";

    MnaFileRef ref;
    ref.role = MnaFileRole::Surface;
    ref.path = "sub-01/anat/lh.pial";
    ref.format = "pial";
    ref.embedded = true;
    ref.data.assign(512, std::byte{0xAA});
    ref.size_bytes = static_cast<std::int64_t>(ref.data.size());

    MnaRecording rec;
    rec.id = "rec-01";
    rec.files.push_back(ref);

    MnaSession sess;
    sess.id = "ses-01";
    sess.recordings.push_back(rec);

    MnaSubject subj;
    subj.id = "sample";
    subj.sessions.push_back(sess);
    proj.subjects.push_back(subj);

    // Write as .mnx
    const QString mnxPath = m_tempDir.filePath("embedded_test.mnx");
    QVERIFY(MnaIO::write(proj, fs(mnxPath)));

    // Read back
    MnaProject restored = MnaIO::read(fs(mnxPath)).value();
    QCOMPARE(restored.name, "Embedded Test");
    QCOMPARE(restored.subjects.size(), std::size_t(1));
    QCOMPARE(restored.subjects[0].sessions[0].recordings[0].files.size(), std::size_t(1));

    const MnaFileRef& rRef = restored.subjects[0].sessions[0].recordings[0].files[0];
    QCOMPARE(rRef.embedded, true);
    QCOMPARE(rRef.data, std::vector<std::byte>(512, std::byte{0xAA}));
    QCOMPARE(rRef.role, MnaFileRole::Surface);
}

void TestMnaSerialization::testProjectEmbeddedMnaRoundTrip()
{
    MnaProject proj;
    proj.name = "JSON Embedded Test";

    MnaFileRef ref;
    ref.role = MnaFileRole::Annotation;
    ref.path = "lh.aparc.annot";
    ref.embedded = true;
    ref.data = bytes({0x00, 0x01, 0x02, 0x03, 0x04, 0x05});
    ref.size_bytes = static_cast<std::int64_t>(ref.data.size());

    MnaRecording rec;
    rec.id = "rec-01";
    rec.files.push_back(ref);

    MnaSession sess;
    sess.id = "ses-01";
    sess.recordings.push_back(rec);

    MnaSubject subj;
    subj.id = "s01";
    subj.sessions.push_back(sess);
    proj.subjects.push_back(subj);

    const QString mnaPath = m_tempDir.filePath("embedded_test.mna");
    QVERIFY(MnaIO::write(proj, fs(mnaPath)));

    MnaProject restored = MnaIO::read(fs(mnaPath)).value();
    QCOMPARE(restored.subjects[0].sessions[0].recordings[0].files[0].embedded, true);
    QCOMPARE(restored.subjects[0].sessions[0].recordings[0].files[0].data, bytes({0x00, 0x01, 0x02, 0x03, 0x04, 0x05}));
}

void TestMnaSerialization::testProjectMultiSubjectRoundTrip()
{
    MnaProject proj;
    proj.name = "Multi-Subject";

    // Create 3 subjects, each with 2 sessions, each with 2 files
    for (int si = 0; si < 3; ++si) {
        MnaSubject subj;
        subj.id = std::format("sub-{:02}", si + 1);

        for (int sei = 0; sei < 2; ++sei) {
            MnaSession sess;
            sess.id = std::format("ses-{:02}", sei + 1);

            MnaRecording rec;
            rec.id = "rec-01";

            MnaFileRef raw;
            raw.role = MnaFileRole::Raw;
            raw.path = std::format("sub-{:02}/ses-{:02}/meg/raw.fif", si + 1, sei + 1);
            raw.embedded = true;
            raw.data.assign(64, static_cast<std::byte>(si * 10 + sei));
            raw.size_bytes = static_cast<std::int64_t>(raw.data.size());
            rec.files.push_back(raw);

            MnaFileRef stc;
            stc.role = MnaFileRole::SourceEstimate;
            stc.path = std::format("sub-{:02}/ses-{:02}/source/result.stc", si + 1, sei + 1);
            stc.embedded = true;
            stc.data.assign(32, static_cast<std::byte>(si * 10 + sei + 100));
            stc.size_bytes = static_cast<std::int64_t>(stc.data.size());
            rec.files.push_back(stc);

            sess.recordings.push_back(rec);
            subj.sessions.push_back(sess);
        }
        proj.subjects.push_back(subj);
    }

    // MNX round-trip
    const QString mnxPath = m_tempDir.filePath("multi_subj.mnx");
    QVERIFY(MnaIO::write(proj, fs(mnxPath)));
    MnaProject restored = MnaIO::read(fs(mnxPath)).value();

    QCOMPARE(restored.subjects.size(), std::size_t(3));
    for (std::size_t si = 0; si < 3; ++si) {
        QCOMPARE(restored.subjects[si].sessions.size(), std::size_t(2));
        for (std::size_t sei = 0; sei < 2; ++sei) {
            QCOMPARE(restored.subjects[si].sessions[sei].recordings[0].files.size(), std::size_t(2));
            const auto& files = restored.subjects[si].sessions[sei].recordings[0].files;
            QCOMPARE(files[0].role, MnaFileRole::Raw);
            QCOMPARE(files[0].data.size(), std::size_t(64));
            QCOMPARE(files[1].role, MnaFileRole::SourceEstimate);
            QCOMPARE(files[1].data.size(), std::size_t(32));
        }
    }
}

//=============================================================================================================
// Edge cases — empty structs
//=============================================================================================================

void TestMnaSerialization::testEmptyScriptRoundTrip()
{
    MnaScript empty;
    JsonObject json = empty.to_json();
    MnaScript restored = MnaScript::from_json(json);
    QVERIFY(restored.language.empty());
    QVERIFY(restored.code.empty());
    QCOMPARE(restored.keep_temp_file, false);

    CborMap cbor = empty.to_cbor();
    MnaScript rC = MnaScript::from_cbor(cbor);
    QVERIFY(rC.language.empty());
}

void TestMnaSerialization::testEmptyVerificationRoundTrip()
{
    MnaVerification empty;
    JsonObject json = empty.to_json();
    MnaVerification restored = MnaVerification::from_json(json);
    QVERIFY(restored.explanation.empty());
    QVERIFY(restored.checks.empty());
    QVERIFY(restored.pre_results.empty());
    QVERIFY(restored.post_results.empty());
}

void TestMnaSerialization::testEmptyProvenanceRoundTrip()
{
    MnaProvenance empty;
    JsonObject json = empty.to_json();
    MnaProvenance restored = MnaProvenance::from_json(json);
    QVERIFY(restored.mne_cpp_version.empty());
    QCOMPARE(restored.wall_time_ms, std::int64_t(0));
    QCOMPARE(restored.peak_memory_bytes, std::int64_t(0));
    QCOMPARE(restored.random_seed, std::int64_t(-1));
}

void TestMnaSerialization::testNodeWithAllExecModes_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<QString>("modeStr");

    QTest::newRow("Batch") << static_cast<int>(MnaNodeExecMode::Batch) << "Batch";
    QTest::newRow("Stream") << static_cast<int>(MnaNodeExecMode::Stream) << "Stream";
    QTest::newRow("Ipc") << static_cast<int>(MnaNodeExecMode::Ipc) << "Ipc";
    QTest::newRow("Script") << static_cast<int>(MnaNodeExecMode::Script) << "Script";
}

void TestMnaSerialization::testNodeWithAllExecModes()
{
    QFETCH(int, mode);

    MnaNode node;
    node.id = "test_node";
    node.op_type = "test_op";
    node.exec_mode = static_cast<MnaNodeExecMode>(mode);

    // JSON
    JsonObject json = node.to_json();
    MnaNode rJ = MnaNode::from_json(json);
    QCOMPARE(static_cast<int>(rJ.exec_mode), mode);

    // CBOR
    CborMap cbor = node.to_cbor();
    MnaNode rC = MnaNode::from_cbor(cbor);
    QCOMPARE(static_cast<int>(rC.exec_mode), mode);
}

//=============================================================================================================
// TEST MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMnaSerialization)
#include "test_mna_serialization.moc"
