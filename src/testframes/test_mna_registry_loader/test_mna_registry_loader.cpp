//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mna_registry_loader.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for MnaRegistryLoader — declarative op schema loading from JSON manifests.
 *
 *           Uses the master registry (resources/mna/mna-registry.json) and a
 *           synthetic drop-in manifest to verify loading, schema population,
 *           round-trip save/load, and drop-in override behavior.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mna/mna_registry_loader.h>
#include <mne/mna/mna_op_registry.h>
#include <mne/mna/mna_op_schema.h>
#include <mne/mna/mna_types.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QCoreApplication>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
/**
 * DECLARE CLASS TestMnaRegistryLoader
 */
class TestMnaRegistryLoader : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testLoadMasterRegistry();
    void testSchemaFields();
    void testRoundTripSaveLoad();
    void testDropInOverride();
    void testLoadNonexistentFile();
    void testEmptyOpsArray();
    void testMissingOps();
    void testLoadDirectory();

    void cleanupTestCase();

private:
    QString findRegistryPath() const;
    static std::filesystem::path fsPath(const QString& path)
    {
        return path_from_utf8(path.toStdString());
    }

    QTemporaryDir m_tempDir;
    QString m_registryPath;
};

//=============================================================================================================

void TestMnaRegistryLoader::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    m_registryPath = findRegistryPath();
    // Registry path may be empty if running outside the source tree;
    // tests that need it will skip gracefully.
}

//=============================================================================================================

QString TestMnaRegistryLoader::findRegistryPath() const
{
    // Search upward from the application directory
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 8; ++i) {
        QString candidate = dir.absoluteFilePath(
            "resources/mna/mna-registry.json");
        if (QFile::exists(candidate))
            return candidate;
        dir.cdUp();
    }
    // Also try current working directory
    if (QFile::exists("resources/mna/mna-registry.json"))
        return QDir::currentPath() + "/resources/mna/mna-registry.json";
    return {};
}

//=============================================================================================================

void TestMnaRegistryLoader::testLoadMasterRegistry()
{
    if (m_registryPath.isEmpty()) {
        QSKIP("mna-registry.json not found — test-data directory absent");
    }

    MnaOpRegistry& registry = MnaOpRegistry::instance();
    int nOps = MnaRegistryLoader::load_file(fsPath(m_registryPath), registry);

    QVERIFY2(nOps > 0, "Master registry should contain at least one op");
    QVERIFY(registry.has_op("load_fiff_raw"));
    QVERIFY(registry.has_op("apply_inverse"));
    QVERIFY(registry.has_op("make_inverse_operator"));

    const StringList ops = registry.registered_ops();
    QVERIFY(ops.size() >= 6);
}

//=============================================================================================================

void TestMnaRegistryLoader::testSchemaFields()
{
    if (m_registryPath.isEmpty()) {
        QSKIP("mna-registry.json not found — test-data directory absent");
    }

    MnaOpRegistry& registry = MnaOpRegistry::instance();
    MnaRegistryLoader::load_file(fsPath(m_registryPath), registry);

    // Check load_fiff_raw schema fields
    const MnaOpSchema& schema = registry.schema("load_fiff_raw");
    QCOMPARE(schema.op_type, std::string("load_fiff_raw"));
    QCOMPARE(schema.binding, std::string("internal"));
    QCOMPARE(schema.category, std::string("io"));
    QCOMPARE(schema.library, std::string("mne_fiff"));
    QVERIFY(!schema.description.empty());
    QVERIFY(!schema.version.empty());
    QVERIFY(schema.output_ports.size() >= 1);

    // Check make_inverse_operator has expected inputs
    const MnaOpSchema& invSchema = registry.schema("make_inverse_operator");
    QCOMPARE(invSchema.category, std::string("source_estimation"));
    QVERIFY(invSchema.input_ports.size() >= 3);
    QVERIFY(invSchema.output_ports.size() >= 1);
    QVERIFY(invSchema.attributes.size() >= 1);
}

//=============================================================================================================

void TestMnaRegistryLoader::testRoundTripSaveLoad()
{
    if (m_registryPath.isEmpty()) {
        QSKIP("mna-registry.json not found — test-data directory absent");
    }

    // Load master registry into the singleton
    MnaOpRegistry& originalRegistry = MnaOpRegistry::instance();
    int nOriginal = MnaRegistryLoader::load_file(fsPath(m_registryPath), originalRegistry);
    QVERIFY(nOriginal > 0);

    // Save to temp file
    QString savedPath = m_tempDir.filePath("saved-registry.json");
    QVERIFY(MnaRegistryLoader::save_file(fsPath(savedPath), "mne-cpp-test", originalRegistry));
    QVERIFY(QFile::exists(savedPath));

    // Load from saved file into the same singleton (ops should re-register without error)
    int nReloaded = MnaRegistryLoader::load_file(fsPath(savedPath), originalRegistry);
    QVERIFY(nReloaded > 0);

    // Verify round-tripped schemas are still present and correct
    for (const std::string& opType : originalRegistry.registered_ops()) {
        QVERIFY2(originalRegistry.has_op(opType), ("Missing op after round-trip: " + opType).c_str());
        const MnaOpSchema& schema = originalRegistry.schema(opType);
        QVERIFY(!schema.op_type.empty());
    }
}

//=============================================================================================================

void TestMnaRegistryLoader::testDropInOverride()
{
    // Create a temporary directory structure: master + drop-in
    QTemporaryDir regDir;
    QVERIFY(regDir.isValid());

    // Write a minimal master manifest
    {
        QJsonObject root;
        root["mna_registry_version"] = "1.0";
        root["provider"] = "test";
        root["description"] = "Test registry";

        QJsonArray ops;
        QJsonObject op;
        op["type"] = "test_op";
        op["version"] = "1.0.0";
        op["binding"] = "internal";
        op["category"] = "test";
        op["description"] = "Original description";
        op["library"] = "mne_test";
        op["inputs"] = QJsonArray();
        op["outputs"] = QJsonArray();
        op["parameters"] = QJsonArray();
        ops.append(op);
        root["ops"] = ops;

        QFile f(regDir.filePath("mna-registry.json"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(root).toJson());
        f.close();
    }

    // Write a drop-in that overrides test_op description and adds a new op
    {
        QDir dropInDir(regDir.filePath("mna-registry.d"));
        QVERIFY(dropInDir.mkpath("."));

        QJsonObject root;
        root["mna_registry_version"] = "1.0";
        root["provider"] = "test-dropin";
        root["description"] = "Drop-in test";

        QJsonArray ops;
        // Override test_op
        QJsonObject overrideOp;
        overrideOp["type"] = "test_op";
        overrideOp["version"] = "2.0.0";
        overrideOp["binding"] = "internal";
        overrideOp["category"] = "test";
        overrideOp["description"] = "Overridden description";
        overrideOp["library"] = "mne_test";
        overrideOp["inputs"] = QJsonArray();
        overrideOp["outputs"] = QJsonArray();
        overrideOp["parameters"] = QJsonArray();
        ops.append(overrideOp);

        // New op
        QJsonObject newOp;
        newOp["type"] = "extra_op";
        newOp["version"] = "1.0.0";
        newOp["binding"] = "cli";
        newOp["category"] = "external";
        newOp["description"] = "Extra from drop-in";
        newOp["library"] = QString();
        newOp["executable"] = "extra_tool";
        newOp["inputs"] = QJsonArray();
        newOp["outputs"] = QJsonArray();
        newOp["parameters"] = QJsonArray();
        ops.append(newOp);
        root["ops"] = ops;

        QFile f(dropInDir.absoluteFilePath("01-dropin.json"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(root).toJson());
        f.close();
    }

    // Load the directory into the singleton
    MnaOpRegistry& registry = MnaOpRegistry::instance();
    int nOps = MnaRegistryLoader::load_directory(fsPath(regDir.path()), registry);

    QVERIFY(nOps >= 2);
    QVERIFY(registry.has_op("test_op"));
    QVERIFY(registry.has_op("extra_op"));

    // Verify override took effect
    const MnaOpSchema& overridden = registry.schema("test_op");
    QCOMPARE(overridden.description, std::string("Overridden description"));
    QCOMPARE(overridden.version, std::string("2.0.0"));

    // Verify new op from drop-in
    const MnaOpSchema& extra = registry.schema("extra_op");
    QCOMPARE(extra.binding, std::string("cli"));
    QCOMPARE(extra.executable, std::string("extra_tool"));
}

//=============================================================================================================

void TestMnaRegistryLoader::testLoadNonexistentFile()
{
    MnaOpRegistry& registry = MnaOpRegistry::instance();
    const std::size_t nBefore = registry.registered_ops().size();
    int nOps = MnaRegistryLoader::load_file(fsPath(
                                                "/nonexistent/path/registry.json"),
                                            registry);
    QVERIFY(nOps <= 0);
    // Existing ops should be unaffected
    QCOMPARE(registry.registered_ops().size(), nBefore);
}

//=============================================================================================================

void TestMnaRegistryLoader::testEmptyOpsArray()
{
    // Write a manifest with no ops
    QJsonObject root;
    root["mna_registry_version"] = "1.0";
    root["provider"] = "empty";
    root["description"] = "Empty registry";
    root["ops"] = QJsonArray();

    QString path = m_tempDir.filePath("empty-registry.json");
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QJsonDocument(root).toJson());
    f.close();

    MnaOpRegistry& registry = MnaOpRegistry::instance();
    const std::size_t nBefore = registry.registered_ops().size();
    int nOps = MnaRegistryLoader::load_file(fsPath(path), registry);
    QCOMPARE(nOps, 0);
    // Existing ops should be unaffected
    QCOMPARE(registry.registered_ops().size(), nBefore);
}

//=============================================================================================================

void TestMnaRegistryLoader::testMissingOps()
{
    MnaOpRegistry& registry = MnaOpRegistry::instance();

    // Register a unique op for this test
    MnaOpSchema schema;
    schema.op_type = "known_test_op_unique";
    registry.register_op(schema);

    // Check missing ops
    const StringList missing = registry.missing_ops({"known_test_op_unique", "definitely_unknown_op_xyz"});
    QCOMPARE(missing, StringList{"definitely_unknown_op_xyz"});
}

//=============================================================================================================

void TestMnaRegistryLoader::testLoadDirectory()
{
    if (m_registryPath.isEmpty()) {
        QSKIP("mna-registry.json not found — test-data directory absent");
    }

    // Load from the actual resources/mna/ directory
    QFileInfo fi(m_registryPath);
    QString regDir = fi.absolutePath();

    MnaOpRegistry& registry = MnaOpRegistry::instance();
    int nOps = MnaRegistryLoader::load_directory(fsPath(regDir), registry);

    QVERIFY(nOps > 0);
    QVERIFY(registry.has_op("load_fiff_raw"));
}

//=============================================================================================================

void TestMnaRegistryLoader::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMnaRegistryLoader)

#include "test_mna_registry_loader.moc"
