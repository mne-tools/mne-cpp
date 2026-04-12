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
    QVector<float> m_volData;
    QVector<int>   m_dims;
    Matrix4f       m_vox2ras;
};

//=============================================================================================================

void TestMriSlicer::initTestCase()
{
    // Create a small synthetic 16×16×16 volume with identity vox2ras
    m_dims = {16, 16, 16};
    int nVoxels = m_dims[0] * m_dims[1] * m_dims[2];
    m_volData.resize(nVoxels);

    // Fill with a gradient so slices are non-uniform
    for (int z = 0; z < m_dims[2]; ++z)
        for (int y = 0; y < m_dims[1]; ++y)
            for (int x = 0; x < m_dims[0]; ++x)
                m_volData[x + y * m_dims[0] + z * m_dims[0] * m_dims[1]] =
                    static_cast<float>(x + y + z);

    // Identity vox2ras (1mm voxels, origin at corner)
    m_vox2ras = Matrix4f::Identity();
}

//=============================================================================================================

void TestMriSlicer::testSliceOrientations()
{
    // Extract one slice of each orientation at the middle index
    int midAxial = m_dims[2] / 2;
    MriSliceImage axial = MriSlicer::extractSlice(m_volData, m_dims, m_vox2ras,
                                                   SliceOrientation::Axial, midAxial);
    QCOMPARE(axial.orientation, SliceOrientation::Axial);
    QVERIFY(axial.width > 0);
    QVERIFY(axial.height > 0);
    QCOMPARE(axial.sliceIndex, midAxial);

    int midCoronal = m_dims[1] / 2;
    MriSliceImage coronal = MriSlicer::extractSlice(m_volData, m_dims, m_vox2ras,
                                                     SliceOrientation::Coronal, midCoronal);
    QCOMPARE(coronal.orientation, SliceOrientation::Coronal);
    QVERIFY(coronal.width > 0);
    QVERIFY(coronal.height > 0);

    int midSagittal = m_dims[0] / 2;
    MriSliceImage sagittal = MriSlicer::extractSlice(m_volData, m_dims, m_vox2ras,
                                                      SliceOrientation::Sagittal, midSagittal);
    QCOMPARE(sagittal.orientation, SliceOrientation::Sagittal);
    QVERIFY(sagittal.width > 0);
    QVERIFY(sagittal.height > 0);
}

//=============================================================================================================

void TestMriSlicer::testRasVoxelRoundTrip()
{
    // For RAS points within the volume, round-trip should be consistent
    // voxelToRas(rasToVoxel(ras)) ≈ ras (within half voxel)
    QVector<Vector3f> testPoints;
    testPoints << Vector3f(5.0f, 5.0f, 5.0f)
               << Vector3f(10.0f, 3.0f, 8.0f)
               << Vector3f(1.0f, 12.0f, 7.0f);

    for (const auto& ras : testPoints) {
        Vector3i vox = MriSlicer::rasToVoxel(m_vox2ras, ras);
        Vector3f roundTrip = MriSlicer::voxelToRas(m_vox2ras, vox);

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
    // Extract three orthogonal slices at a point
    Vector3f centerRas(8.0f, 8.0f, 8.0f);
    QVector<MriSliceImage> slices = MriSlicer::extractOrthogonal(m_volData, m_dims,
                                                                  m_vox2ras, centerRas);

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
    // Slice pixels should be normalized to [0, 1]
    MriSliceImage slice = MriSlicer::extractSlice(m_volData, m_dims, m_vox2ras,
                                                   SliceOrientation::Axial, m_dims[2] / 2);

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
