//=============================================================================================================
/**
 * @file     test_fs_atlas_lookup.cpp
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
 * @brief    Tests for FsAtlasLookup — FreeSurfer volume parcellation atlas lookup.
 *           Tests the API surface and edge cases. Data-driven tests require
 *           aparc+aseg.mgz from FreeSurfer sample subject (skipped if absent).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/fs_atlas_lookup.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestFsAtlasLookup
 *
 * @brief Tests for FsAtlasLookup atlas region lookup.
 */
class TestFsAtlasLookup : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testDefaultNotLoaded();
    void testLoadNonexistent();
    void testBatchLookupEmpty();
    void testLoadAndLookup();
    void testOutsideBrainReturnsUnknown();
    void testBatchLookup();

    void cleanupTestCase();

private:
    QString m_parcPath;
    bool m_hasTestData = false;
};

//=============================================================================================================

void TestFsAtlasLookup::initTestCase()
{
    // Try to locate aparc+aseg.mgz from FreeSurfer test data
    // Check common locations relative to the build directory
    QStringList candidates;
    candidates << QDir::currentPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/mri/aparc+aseg.mgz"
               << QDir::currentPath() + "/resources/data/mne-cpp-test-data/subjects/sample/mri/aparc+aseg.mgz";

    // Also check MNE_DATA environment variable
    QString mneData = qEnvironmentVariable("MNE_DATA");
    if (!mneData.isEmpty()) {
        candidates << mneData + "/MNE-sample-data/subjects/sample/mri/aparc+aseg.mgz";
    }

    for (const auto& path : candidates) {
        if (QFile::exists(path)) {
            m_parcPath = path;
            m_hasTestData = true;
            break;
        }
    }
}

//=============================================================================================================

void TestFsAtlasLookup::testDefaultNotLoaded()
{
    FsAtlasLookup lookup;
    QVERIFY(!lookup.isLoaded());
}

//=============================================================================================================

void TestFsAtlasLookup::testLoadNonexistent()
{
    FsAtlasLookup lookup;
    bool ok = lookup.load("/nonexistent/aparc+aseg.mgz");
    QVERIFY(!ok);
    QVERIFY(!lookup.isLoaded());
}

//=============================================================================================================

void TestFsAtlasLookup::testBatchLookupEmpty()
{
    FsAtlasLookup lookup;
    QVector<Vector3f> positions;
    QStringList labels = lookup.labelsForPositions(positions);
    QVERIFY(labels.isEmpty());
}

//=============================================================================================================

void TestFsAtlasLookup::testLoadAndLookup()
{
    if (!m_hasTestData) {
        qWarning("No aparc+aseg.mgz found — skipping data-driven atlas lookup test. "
                 "Set MNE_DATA or provide mne-cpp-test-data.");
        return;
    }

    FsAtlasLookup lookup;
    QVERIFY(lookup.load(m_parcPath));
    QVERIFY(lookup.isLoaded());

    // RAS coordinate near the brain center — should return a recognized label
    Vector3f center(0.0f, 0.0f, 0.0f);
    QString label = lookup.labelAtRas(center);
    QVERIFY2(!label.isEmpty(),
             "Label at brain center should not be empty");
}

//=============================================================================================================

void TestFsAtlasLookup::testOutsideBrainReturnsUnknown()
{
    if (!m_hasTestData) {
        qWarning("No aparc+aseg.mgz found — skipping data-driven test.");
        return;
    }

    FsAtlasLookup lookup;
    QVERIFY(lookup.load(m_parcPath));

    // RAS coordinate far outside the brain
    Vector3f farAway(500.0f, 500.0f, 500.0f);
    QString label = lookup.labelAtRas(farAway);
    // Should return "Unknown" or empty — not crash
    QVERIFY2(label.isEmpty() || label.toLower().contains("unknown") || label == "0",
             qPrintable(QString("Far-away label should be Unknown, got '%1'").arg(label)));
}

//=============================================================================================================

void TestFsAtlasLookup::testBatchLookup()
{
    if (!m_hasTestData) {
        qWarning("No aparc+aseg.mgz found — skipping batch lookup test.");
        return;
    }

    FsAtlasLookup lookup;
    QVERIFY(lookup.load(m_parcPath));

    QVector<Vector3f> positions;
    positions << Vector3f(0.0f, 0.0f, 0.0f)     // center
              << Vector3f(0.0f, -25.0f, -15.0f)  // near hippocampus
              << Vector3f(500.0f, 500.0f, 500.0f); // outside

    QStringList labels = lookup.labelsForPositions(positions);
    QCOMPARE(labels.size(), 3);
    // All entries should be non-empty (even "Unknown" for outside-brain)
    for (int i = 0; i < labels.size(); ++i) {
        QVERIFY2(!labels[i].isEmpty(),
                 qPrintable(QString("Label[%1] should not be empty").arg(i)));
    }
}

//=============================================================================================================

void TestFsAtlasLookup::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestFsAtlasLookup)
#include "test_fs_atlas_lookup.moc"
