//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fs_atlas_lookup.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for FsAtlasLookup — FreeSurfer volume parcellation atlas lookup.
 *           Tests the API surface and edge cases. Data-driven tests require
 *           aparc+aseg.mgz from FreeSurfer sample subject (skipped if absent).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/fs_atlas_lookup.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QDir>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
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
    void testSyntheticMghAndMgz();

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
    QVERIFY(!lookup.is_loaded());
}

//=============================================================================================================

void TestFsAtlasLookup::testLoadNonexistent()
{
    FsAtlasLookup lookup;
    bool ok = lookup.load("/nonexistent/aparc+aseg.mgz");
    QVERIFY(!ok);
    QVERIFY(!lookup.is_loaded());
}

//=============================================================================================================

void TestFsAtlasLookup::testBatchLookupEmpty()
{
    FsAtlasLookup lookup;
    QVERIFY(lookup.labels_for_positions({}).empty());
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
    QVERIFY(lookup.load(m_parcPath.toStdU16String()));
    QVERIFY(lookup.is_loaded());

    // RAS coordinate near the brain center — should return a recognized label
    Vector3f center(0.0f, 0.0f, 0.0f);
    const std::string& label = lookup.label_at_ras(center);
    QVERIFY2(!label.empty(),
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
    QVERIFY(lookup.load(m_parcPath.toStdU16String()));

    // RAS coordinate far outside the brain
    Vector3f farAway(500.0f, 500.0f, 500.0f);
    QCOMPARE(lookup.label_at_ras(farAway), std::string("Unknown"));
}

//=============================================================================================================

void TestFsAtlasLookup::testBatchLookup()
{
    if (!m_hasTestData) {
        qWarning("No aparc+aseg.mgz found — skipping batch lookup test.");
        return;
    }

    FsAtlasLookup lookup;
    QVERIFY(lookup.load(m_parcPath.toStdU16String()));

    const std::vector<Vector3f> positions{Vector3f(0.0f, 0.0f, 0.0f),        // center
                                          Vector3f(0.0f, -25.0f, -15.0f),    // near hippocampus
                                          Vector3f(500.0f, 500.0f, 500.0f)}; // outside
    const std::vector<std::string> labels = lookup.labels_for_positions(positions);
    QCOMPARE(labels.size(), std::size_t{3});
    // All entries are non-empty, "Unknown" outside the brain
    for (const std::string& label : labels) {
        QVERIFY(!label.empty());
    }
}

//=============================================================================================================

void TestFsAtlasLookup::testSyntheticMghAndMgz()
{
    // A 4x4x4 uchar volume with 1 mm voxels centred on RAS 0: voxel (1, 2, 3) is Left-Hippocampus (17),
    // voxel (3, 0, 0) a label the lookup table does not know
    std::vector<unsigned char> mgh(284 + 64, 0);
    const auto put32 = [&mgh](std::size_t at, std::uint32_t v) {
        for (int k = 0; k < 4; ++k)
            mgh[at + static_cast<std::size_t>(k)] = static_cast<unsigned char>(v >> (24 - 8 * k));
    };
    const auto putFloat = [&put32](std::size_t at, float v) {
        put32(at, std::bit_cast<std::uint32_t>(v));
    };
    put32(0, 1);
    put32(4, 4);
    put32(8, 4);
    put32(12, 4);
    put32(16, 1);
    mgh[29] = 1; // ras_good_flag
    for (std::size_t axis = 0; axis < 3; ++axis) {
        putFloat(30 + 4 * axis, 1.0f);
        putFloat(42 + 12 * axis + 4 * axis, 1.0f);
    }
    mgh[284 + 1 + 4 * (2 + 4 * 3)] = 17;
    mgh[284 + 3] = 250;

    // The same bytes as gzip with one stored deflate block
    std::vector<unsigned char> mgz{0x1f, 0x8b, 0x08, 0, 0, 0, 0, 0, 0, 0x03, 0x01};
    const auto size = static_cast<std::uint16_t>(mgh.size());
    mgz.insert(mgz.end(), {static_cast<unsigned char>(size), static_cast<unsigned char>(size >> 8), static_cast<unsigned char>(~size), static_cast<unsigned char>(~size >> 8)});
    mgz.insert(mgz.end(), mgh.begin(), mgh.end());
    std::uint32_t crc = 0xFFFFFFFFu;
    for (const unsigned char byte : mgh) {
        crc ^= byte;
        for (int k = 0; k < 8; ++k)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    crc = ~crc;
    for (const std::uint32_t v : {crc, static_cast<std::uint32_t>(mgh.size())})
        for (int k = 0; k < 4; ++k)
            mgz.push_back(static_cast<unsigned char>(v >> (8 * k)));

    QTemporaryDir dir;
    for (const auto& [name, bytes] : {std::pair{"atlas.mgh", &mgh}, std::pair{"atlas.MGZ", &mgz}}) {
        const std::filesystem::path path = dir.filePath(QString::fromLatin1(name)).toStdU16String();
        std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(bytes->data()), static_cast<std::streamsize>(bytes->size()));

        FsAtlasLookup lookup;
        QVERIFY2(lookup.load(path), name);
        QCOMPARE(lookup.label_at_ras(Vector3f(-1.0f, 0.0f, 1.0f)), std::string("Left-Hippocampus"));
        QCOMPARE(lookup.label_at_ras(Vector3f(1.0f, -2.0f, -2.0f)), std::string("Unknown"));
        QCOMPARE(lookup.label_at_ras(Vector3f(0.0f, 0.0f, 0.0f)), std::string("Unknown"));
        QCOMPARE(lookup.label_at_ras(Vector3f(9.0f, 0.0f, 0.0f)), std::string("Unknown"));
    }

    // Cut voxel data is rejected
    mgh.resize(300);
    const std::filesystem::path cut = dir.filePath(QStringLiteral("cut.mgh")).toStdU16String();
    std::ofstream(cut, std::ios::binary).write(reinterpret_cast<const char*>(mgh.data()), static_cast<std::streamsize>(mgh.size()));
    FsAtlasLookup lookup;
    QVERIFY(!lookup.load(cut));
    QVERIFY(!lookup.is_loaded());
}

//=============================================================================================================

void TestFsAtlasLookup::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestFsAtlasLookup)
#include "test_fs_atlas_lookup.moc"
