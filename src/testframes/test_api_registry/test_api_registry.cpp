//=============================================================================================================
/**
 * @file     test_api_registry.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Validates invariants of doc/api_registry.json:
 *             1. JSON is well-formed and has a `classes` array.
 *             2. Every `header` path exists under `src/libraries/`.
 *             3. Every `test` directory (when not null) exists under `src/testframes/`.
 *             4. No entry with `skigen_candidate: true` AND `status: "done"`
 *                lacks a non-empty `skigen_target`.
 *
 *           The repository root is discovered by walking up from the test binary
 *           location and looking for `CMakeLists.txt` containing `project(mne-cpp`.
 */
//=============================================================================================================

#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

//=============================================================================================================

class TestApiRegistry : public QObject
{
    Q_OBJECT

private:
    QString m_repoRoot;
    QJsonDocument m_registry;

    bool discoverRepoRoot()
    {
        QDir dir(QCoreApplication::applicationDirPath());
        for (int i = 0; i < 12; ++i) {
            const QString cmake = dir.absoluteFilePath("CMakeLists.txt");
            QFile f(cmake);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                const QByteArray data = f.read(4096);
                f.close();
                if (data.contains("mne_cpp_root")) {
                    m_repoRoot = dir.absolutePath();
                    return true;
                }
            }
            if (!dir.cdUp()) {
                break;
            }
        }
        return false;
    }

    bool loadRegistry()
    {
        const QString regPath = m_repoRoot + "/doc/api_registry.json";
        QFile file(regPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Failed to open registry:" << regPath;
            return false;
        }
        const QByteArray data = file.readAll();
        file.close();

        QJsonParseError err{};
        m_registry = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << err.errorString() << "at offset" << err.offset;
            return false;
        }
        return true;
    }

private slots:
    void initTestCase()
    {
        QVERIFY2(discoverRepoRoot(), "Could not locate mne-cpp repo root");
        qDebug() << "Repo root:" << m_repoRoot;
        QVERIFY2(loadRegistry(), "Failed to load doc/api_registry.json");
    }

    void testJsonWellFormed()
    {
        QVERIFY(m_registry.isObject());
        const QJsonObject obj = m_registry.object();
        QVERIFY(obj.contains("classes"));
        QVERIFY(obj.value("classes").isArray());
    }

    void testHeaderPathsExist()
    {
        const QJsonArray classes = m_registry.object().value("classes").toArray();
        int missing = 0;
        for (const QJsonValue &val : classes) {
            if (!val.isObject()) continue;
            const QJsonObject cls = val.toObject();
            if (!cls.contains("header")) continue;
            const QString headerRel = cls.value("header").toString();
            const QString headerFull = m_repoRoot + "/src/libraries/" + headerRel;
            if (!QFile::exists(headerFull)) {
                qWarning() << "Missing header:" << headerRel
                           << "(class:" << cls.value("name").toString() << ")";
                ++missing;
            }
        }
        QCOMPARE(missing, 0);
    }

    void testTestDirectoriesExist()
    {
        const QJsonArray classes = m_registry.object().value("classes").toArray();
        int missing = 0;
        for (const QJsonValue &val : classes) {
            if (!val.isObject()) continue;
            const QJsonObject cls = val.toObject();
            const QJsonValue tv = cls.value("test");
            if (tv.isNull() || tv.toString().isEmpty()) continue;
            const QString testName = tv.toString();
            const QString testDir = m_repoRoot + "/src/testframes/" + testName;
            if (!QDir(testDir).exists()) {
                qWarning() << "Missing test directory:" << testName
                           << "(class:" << cls.value("name").toString() << ")";
                ++missing;
            }
        }
        QCOMPARE(missing, 0);
    }

    void testSkigenTargetPresence()
    {
        const QJsonArray classes = m_registry.object().value("classes").toArray();
        int violations = 0;
        for (const QJsonValue &val : classes) {
            if (!val.isObject()) continue;
            const QJsonObject cls = val.toObject();
            const bool skigenCand = cls.value("skigen_candidate").toBool(false);
            const QString status = cls.value("status").toString();
            const QString target = cls.value("skigen_target").toString();
            if (skigenCand && status == "done" && target.isEmpty()) {
                qWarning() << "Violation:" << cls.value("name").toString()
                           << "has skigen_candidate=true, status=done, but no skigen_target";
                ++violations;
            }
        }
        QCOMPARE(violations, 0);
    }
};

QTEST_MAIN(TestApiRegistry)
#include "test_api_registry.moc"
