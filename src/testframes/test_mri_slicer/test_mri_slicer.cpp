//=============================================================================================================
/**
 * @file     test_mri_slicer.cpp
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
 * @brief    Tests for MriSlicer — MRI volume slice extraction.
 *           Data-driven tests use T1 volume from mne-cpp-test-data (skipped if absent).
 *           Synthetic tests verify coordinate transformations.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mri/mri_slicer.h>
#include <mri/mri_vol_data.h>

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

using namespace MRILIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestMriSlicer
 *
 * @brief Tests for MriSlicer orthogonal slice extraction.
 */
class TestMriSlicer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testSliceOrientations();
    void testRasVoxelRoundTrip();
    void testOrthogonalExtraction();
    void testSliceNormalization();

    void cleanupTestCase();

private:
    MriVolData m_vol;
    bool m_hasTestData = false;
};

//=============================================================================================================

void TestMriSlicer::initTestCase()
{
    // Try to find a T1 volume from test data
    QStringList candidates;
    candidates << QDir::currentPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/mri/T1.mgz"
               << QDir::currentPath() + "/resources/data/mne-cpp-test-data/subjects/sample/mri/T1.mgz";

    QString mneData = qEnvironmentVariable("MNE_DATA");
    if (!mneData.isEmpty()) {
        candidates << mneData + "/MNE-sample-data/subjects/sample/mri/T1.mgz";
    }

    for (const auto& path : candidates) {
        if (QFile::exists(path)) {
            m_hasTestData = m_vol.read(path);
            if (m_hasTestData) {
                qDebug() << "Loaded T1 volume from:" << path;
                break;
            }
        }
    }

    if (!m_hasTestData) {
        qWarning("No T1.mgz found — some MriSlicer tests will be limited. "
                 "Set MNE_DATA or provide mne-cpp-test-data.");
    }
}

//=============================================================================================================

void TestMriSlicer::testSliceOrientations()
{
    if (!m_hasTestData)
        return;

    // Extract one slice of each orientation at the middle index
    int midAxial = m_vol.dimZ() / 2;
    MriSliceImage axial = MriSlicer::extractSlice(m_vol, SliceOrientation::Axial, midAxial);
    QCOMPARE(axial.orientation, SliceOrientation::Axial);
    QVERIFY(axial.width > 0);
    QVERIFY(axial.height > 0);
    QCOMPARE(axial.sliceIndex, midAxial);

    int midCoronal = m_vol.dimY() / 2;
    MriSliceImage coronal = MriSlicer::extractSlice(m_vol, SliceOrientation::Coronal, midCoronal);
    QCOMPARE(coronal.orientation, SliceOrientation::Coronal);
    QVERIFY(coronal.width > 0);
    QVERIFY(coronal.height > 0);

    int midSagittal = m_vol.dimX() / 2;
    MriSliceImage sagittal = MriSlicer::extractSlice(m_vol, SliceOrientation::Sagittal, midSagittal);
    QCOMPARE(sagittal.orientation, SliceOrientation::Sagittal);
    QVERIFY(sagittal.width > 0);
    QVERIFY(sagittal.height > 0);
}

//=============================================================================================================

void TestMriSlicer::testRasVoxelRoundTrip()
{
    if (!m_hasTestData)
        return;

    // For random RAS points within the volume, round-trip should be consistent
    // voxelToRas(rasToVoxel(ras)) ≈ ras (within half voxel)
    QVector<Vector3f> testPoints;
    testPoints << Vector3f(0.0f, 0.0f, 0.0f)
               << Vector3f(10.0f, -20.0f, 30.0f)
               << Vector3f(-5.0f, 15.0f, -10.0f);

    for (const auto& ras : testPoints) {
        Vector3i vox = MriSlicer::rasToVoxel(m_vol, ras);
        Vector3f roundTrip = MriSlicer::voxelToRas(m_vol, vox);

        // The round-trip error should be at most 1 voxel width in each dimension
        // (due to integer rounding of voxel indices)
        float maxErr = (roundTrip - ras).cwiseAbs().maxCoeff();
        QVERIFY2(maxErr < 2.0f,  // Allow up to 2mm (typical voxel size is 1mm)
                 qPrintable(QString("RAS round-trip error=%1mm for point (%2,%3,%4)")
                           .arg(maxErr).arg(ras.x()).arg(ras.y()).arg(ras.z())));
    }
}

//=============================================================================================================

void TestMriSlicer::testOrthogonalExtraction()
{
    if (!m_hasTestData)
        return;

    // Extract three orthogonal slices at a point
    Vector3f centerRas(0.0f, 0.0f, 0.0f);
    QVector<MriSliceImage> slices = MriSlicer::extractOrthogonal(m_vol, centerRas);

    QCOMPARE(slices.size(), 3);

    // Each orientation should appear exactly once
    bool hasAxial = false, hasCoronal = false, hasSagittal = false;
    for (const auto& s : slices) {
        if (s.orientation == SliceOrientation::Axial) hasAxial = true;
        if (s.orientation == SliceOrientation::Coronal) hasCoronal = true;
        if (s.orientation == SliceOrientation::Sagittal) hasSagittal = true;
    }
    QVERIFY(hasAxial);
    QVERIFY(hasCoronal);
    QVERIFY(hasSagittal);
}

//=============================================================================================================

void TestMriSlicer::testSliceNormalization()
{
    if (!m_hasTestData)
        return;

    // Slice pixels should be normalized to [0, 1]
    MriSliceImage slice = MriSlicer::extractSlice(m_vol, SliceOrientation::Axial, m_vol.dimZ() / 2);

    float minVal = slice.pixels.minCoeff();
    float maxVal = slice.pixels.maxCoeff();

    QVERIFY2(minVal >= 0.0f,
             qPrintable(QString("Pixel min=%1, expected >= 0").arg(minVal)));
    QVERIFY2(maxVal <= 1.0f,
             qPrintable(QString("Pixel max=%1, expected <= 1").arg(maxVal)));

    // Slice should not be all zero (contains brain anatomy)
    QVERIFY2(maxVal > 0.0f, "Slice should contain non-zero pixels (brain anatomy)");
}

//=============================================================================================================

void TestMriSlicer::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMriSlicer)
#include "test_mri_slicer.moc"
