//=============================================================================================================
/**
 * @file     test_mna_registry_loader.cpp
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
 * @brief    Tests for MnaRegistryLoader — declarative op schema loading from JSON manifests.
 *
 *           Uses the master registry (resources/mna/mna-registry.json) and a
 *           synthetic drop-in manifest to verify loading, schema population,
 *           round-trip save/load, and drop-in override behavior.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mna/mna_registry_loader.h>
#include <mna/mna_op_registry.h>
#include <mna/mna_op_schema.h>
#include <mna/mna_types.h>

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
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

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
            QStringLiteral("resources/mna/mna-registry.json"));
        if (QFile::exists(candidate))
            return candidate;
        dir.cdUp();
    }
    // Also try current working directory
    if (QFile::exists(QStringLiteral("resources/mna/mna-registry.json")))
        return QDir::currentPath() + QStringLiteral("/resources/mna/mna-registry.json");
    return {};
}

//=============================================================================================================

void TestMnaRegistryLoader::testLoadMasterRegistry()
{
    if (m_registryPath.isEmpty()) {
        QSKIP("mna-registry.json not found — test-data directory absent");
    }

    MnaOpRegistry& registry = MnaOpRegistry::instance();
    int nOps = MnaRegistryLoader::loadFile(m_registryPath, registry);

    QVERIFY2(nOps > 0, "Master registry should contain at least one op");
    QVERIFY(registry.hasOp(QStringLiteral("load_fiff_raw")));
    QVERIFY(registry.hasOp(QStringLiteral("apply_inverse")));
    QVERIFY(registry.hasOp(QStringLiteral("make_inverse_operator")));

    QStringList ops = registry.registeredOps();
    QVERIFY(ops.size() >= 6);
}

//=============================================================================================================

void TestMnaRegistryLoader::testSchemaFields()
{
    if (m_registryPath.isEmpty()) {
        QSKIP("mna-registry.json not found — test-data directory absent");
    }

    MnaOpRegistry& registry = MnaOpRegistry::instance();
    MnaRegistryLoader::loadFile(m_registryPath, registry);

    // Check load_fiff_raw schema fields
    MnaOpSchema schema = registry.schema(QStringLiteral("load_fiff_raw"));
    QCOMPARE(schema.opType, QStringLiteral("load_fiff_raw"));
    QCOMPARE(schema.binding, QStringLiteral("internal"));
    QCOMPARE(schema.category, QStringLiteral("io"));
    QCOMPARE(schema.library, QStringLiteral("mne_fiff"));
    QVERIFY(!schema.description.isEmpty());
    QVERIFY(!schema.version.isEmpty());
    QVERIFY(schema.outputPorts.size() >= 1);

    // Check make_inverse_operator has expected inputs
    MnaOpSchema invSchema = registry.schema(QStringLiteral("make_inverse_operator"));
    QCOMPARE(invSchema.category, QStringLiteral("source_estimation"));
    QVERIFY(invSchema.inputPorts.size() >= 3);
    QVERIFY(invSchema.outputPorts.size() >= 1);
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
    int nBefore = originalRegistry.registeredOps().size();
    int nOriginal = MnaRegistryLoader::loadFile(m_registryPath, originalRegistry);
    QVERIFY(nOriginal > 0);

    // Save to temp file
    QString savedPath = m_tempDir.filePath(QStringLiteral("saved-registry.json"));
    QVERIFY(MnaRegistryLoader::saveFile(savedPath, QStringLiteral("mne-cpp-test"), originalRegistry));
    QVERIFY(QFile::exists(savedPath));

    // Load from saved file into the same singleton (ops should re-register without error)
    int nReloaded = MnaRegistryLoader::loadFile(savedPath, originalRegistry);
    QVERIFY(nReloaded > 0);

    // Verify round-tripped schemas are still present and correct
    for (const QString& opType : originalRegistry.registeredOps()) {
        QVERIFY2(originalRegistry.hasOp(opType),
                 qPrintable(QStringLiteral("Missing op after round-trip: ") + opType));
        MnaOpSchema schema = originalRegistry.schema(opType);
        QVERIFY(!schema.opType.isEmpty());
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
        root[QStringLiteral("mna_registry_version")] = QStringLiteral("1.0");
        root[QStringLiteral("provider")] = QStringLiteral("test");
        root[QStringLiteral("description")] = QStringLiteral("Test registry");

        QJsonArray ops;
        QJsonObject op;
        op[QStringLiteral("type")] = QStringLiteral("test_op");
        op[QStringLiteral("version")] = QStringLiteral("1.0.0");
        op[QStringLiteral("binding")] = QStringLiteral("internal");
        op[QStringLiteral("category")] = QStringLiteral("test");
        op[QStringLiteral("description")] = QStringLiteral("Original description");
        op[QStringLiteral("library")] = QStringLiteral("mne_test");
        op[QStringLiteral("inputs")] = QJsonArray();
        op[QStringLiteral("outputs")] = QJsonArray();
        op[QStringLiteral("parameters")] = QJsonArray();
        ops.append(op);
        root[QStringLiteral("ops")] = ops;

        QFile f(regDir.filePath(QStringLiteral("mna-registry.json")));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(root).toJson());
        f.close();
    }

    // Write a drop-in that overrides test_op description and adds a new op
    {
        QDir dropInDir(regDir.filePath(QStringLiteral("mna-registry.d")));
        QVERIFY(dropInDir.mkpath(QStringLiteral(".")));

        QJsonObject root;
        root[QStringLiteral("mna_registry_version")] = QStringLiteral("1.0");
        root[QStringLiteral("provider")] = QStringLiteral("test-dropin");
        root[QStringLiteral("description")] = QStringLiteral("Drop-in test");

        QJsonArray ops;
        // Override test_op
        QJsonObject overrideOp;
        overrideOp[QStringLiteral("type")] = QStringLiteral("test_op");
        overrideOp[QStringLiteral("version")] = QStringLiteral("2.0.0");
        overrideOp[QStringLiteral("binding")] = QStringLiteral("internal");
        overrideOp[QStringLiteral("category")] = QStringLiteral("test");
        overrideOp[QStringLiteral("description")] = QStringLiteral("Overridden description");
        overrideOp[QStringLiteral("library")] = QStringLiteral("mne_test");
        overrideOp[QStringLiteral("inputs")] = QJsonArray();
        overrideOp[QStringLiteral("outputs")] = QJsonArray();
        overrideOp[QStringLiteral("parameters")] = QJsonArray();
        ops.append(overrideOp);

        // New op
        QJsonObject newOp;
        newOp[QStringLiteral("type")] = QStringLiteral("extra_op");
        newOp[QStringLiteral("version")] = QStringLiteral("1.0.0");
        newOp[QStringLiteral("binding")] = QStringLiteral("cli");
        newOp[QStringLiteral("category")] = QStringLiteral("external");
        newOp[QStringLiteral("description")] = QStringLiteral("Extra from drop-in");
        newOp[QStringLiteral("library")] = QString();
        newOp[QStringLiteral("executable")] = QStringLiteral("extra_tool");
        newOp[QStringLiteral("inputs")] = QJsonArray();
        newOp[QStringLiteral("outputs")] = QJsonArray();
        newOp[QStringLiteral("parameters")] = QJsonArray();
        ops.append(newOp);
        root[QStringLiteral("ops")] = ops;

        QFile f(dropInDir.absoluteFilePath(QStringLiteral("01-dropin.json")));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(root).toJson());
        f.close();
    }

    // Load the directory into the singleton
    MnaOpRegistry& registry = MnaOpRegistry::instance();
    int nOps = MnaRegistryLoader::loadDirectory(regDir.path(), registry);

    QVERIFY(nOps >= 2);
    QVERIFY(registry.hasOp(QStringLiteral("test_op")));
    QVERIFY(registry.hasOp(QStringLiteral("extra_op")));

    // Verify override took effect
    MnaOpSchema overridden = registry.schema(QStringLiteral("test_op"));
    QCOMPARE(overridden.description, QStringLiteral("Overridden description"));
    QCOMPARE(overridden.version, QStringLiteral("2.0.0"));

    // Verify new op from drop-in
    MnaOpSchema extra = registry.schema(QStringLiteral("extra_op"));
    QCOMPARE(extra.binding, QStringLiteral("cli"));
    QCOMPARE(extra.executable, QStringLiteral("extra_tool"));
}

//=============================================================================================================

void TestMnaRegistryLoader::testLoadNonexistentFile()
{
    MnaOpRegistry& registry = MnaOpRegistry::instance();
    int nBefore = registry.registeredOps().size();
    int nOps = MnaRegistryLoader::loadFile(
        QStringLiteral("/nonexistent/path/registry.json"), registry);
    QVERIFY(nOps <= 0);
    // Existing ops should be unaffected
    QCOMPARE(registry.registeredOps().size(), nBefore);
}

//=============================================================================================================

void TestMnaRegistryLoader::testEmptyOpsArray()
{
    // Write a manifest with no ops
    QJsonObject root;
    root[QStringLiteral("mna_registry_version")] = QStringLiteral("1.0");
    root[QStringLiteral("provider")] = QStringLiteral("empty");
    root[QStringLiteral("description")] = QStringLiteral("Empty registry");
    root[QStringLiteral("ops")] = QJsonArray();

    QString path = m_tempDir.filePath(QStringLiteral("empty-registry.json"));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QJsonDocument(root).toJson());
    f.close();

    MnaOpRegistry& registry = MnaOpRegistry::instance();
    int nBefore = registry.registeredOps().size();
    int nOps = MnaRegistryLoader::loadFile(path, registry);
    QCOMPARE(nOps, 0);
    // Existing ops should be unaffected
    QCOMPARE(registry.registeredOps().size(), nBefore);
}

//=============================================================================================================

void TestMnaRegistryLoader::testMissingOps()
{
    MnaOpRegistry& registry = MnaOpRegistry::instance();

    // Register a unique op for this test
    MnaOpSchema schema;
    schema.opType = QStringLiteral("known_test_op_unique");
    registry.registerOp(schema);

    // Check missing ops
    QStringList pipeline;
    pipeline << QStringLiteral("known_test_op_unique") << QStringLiteral("definitely_unknown_op_xyz");
    QStringList missing = registry.missingOps(pipeline);

    QCOMPARE(missing.size(), 1);
    QCOMPARE(missing.first(), QStringLiteral("definitely_unknown_op_xyz"));
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
    int nOps = MnaRegistryLoader::loadDirectory(regDir, registry);

    QVERIFY(nOps > 0);
    QVERIFY(registry.hasOp(QStringLiteral("load_fiff_raw")));
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
