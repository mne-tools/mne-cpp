//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_api_registry.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     May, 2026
 * @brief    Validates invariants of doc/api_registry.json:
 *             1. JSON is well-formed and has a `classes` array.
 *             2. Every `header` path exists under `src/libs/mne/`.
 *             3. Every `test` directory (when not null) exists under `src/testframes/`.
 *             4. No entry carries the retired Skigen port fields `skigen_candidate` and `skigen_target`.
 *
 *           The repository root is the source tree the test was configured from (MNE_REPO_ROOT),
 *           so the test also runs when BINARY_OUTPUT_DIRECTORY is outside the repository.
 */
//=============================================================================================================

#include <QtTest/QtTest>
#include <mne/base/log.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QDir>

//=============================================================================================================

class TestApiRegistry : public QObject
{
    Q_OBJECT

private:
    QString m_repoRoot;
    QJsonDocument m_registry;

    bool discoverRepoRoot()
    {
        // The source tree this test was configured from; the binary may be written anywhere.
        m_repoRoot = QStringLiteral(MNE_REPO_ROOT);
        return QFile::exists(m_repoRoot + "/doc/api_registry.json");
    }

    bool loadRegistry()
    {
        const QString regPath = m_repoRoot + "/doc/api_registry.json";
        QFile file(regPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            mne::Log::warning("Failed to open registry: {}", regPath.toStdString());
            return false;
        }
        const QByteArray data = file.readAll();
        file.close();

        QJsonParseError err{};
        m_registry = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            mne::Log::warning("JSON parse error: {} at offset {}", err.errorString().toStdString(), err.offset);
            return false;
        }
        return true;
    }

private slots:
    void initTestCase()
    {
        QVERIFY2(discoverRepoRoot(), "Could not locate mne-cpp repo root");
        mne::Log::debug("Repo root: {}", m_repoRoot.toStdString());
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
        for (const QJsonValue& val : classes) {
            if (!val.isObject())
                continue;
            const QJsonObject cls = val.toObject();
            if (!cls.contains("header"))
                continue;
            const QString headerRel = cls.value("header").toString();
            const QString headerFull = m_repoRoot + "/src/libs/mne/" + headerRel;
            if (!QFile::exists(headerFull)) {
                mne::Log::warning("Missing header: {} (class: {} )", headerRel.toStdString(), cls.value("name").toString().toStdString());
                ++missing;
            }
        }
        QCOMPARE(missing, 0);
    }

    void testTestDirectoriesExist()
    {
        const QJsonArray classes = m_registry.object().value("classes").toArray();
        int missing = 0;
        for (const QJsonValue& val : classes) {
            if (!val.isObject())
                continue;
            const QJsonObject cls = val.toObject();
            const QJsonValue tv = cls.value("test");
            if (tv.isNull() || tv.toString().isEmpty())
                continue;
            const QString testName = tv.toString();
            const QString testDir = m_repoRoot + "/src/testframes/" + testName;
            if (!QDir(testDir).exists()) {
                mne::Log::warning("Missing test directory: {} (class: {} )", testName.toStdString(), cls.value("name").toString().toStdString());
                ++missing;
            }
        }
        QCOMPARE(missing, 0);
    }

    void testNoRetiredSkigenFields()
    {
        const QJsonArray classes = m_registry.object().value("classes").toArray();
        int retired = 0;
        for (const QJsonValue& val : classes) {
            const QJsonObject cls = val.toObject();
            for (const char* field : {"skigen_candidate", "skigen_target"}) {
                if (cls.contains(QLatin1String(field))) {
                    mne::Log::warning("{} carries the retired field {}", cls.value("name").toString().toStdString(), field);
                    ++retired;
                }
            }
        }
        QCOMPARE(retired, 0);
    }
};

QTEST_MAIN(TestApiRegistry)
#include "test_api_registry.moc"
