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
#include <fiff/fiff_file.h>

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

    // MriVolData convenience-API tests
    void testVolDataDimAccessors();
    void testVolDataVoxelDataAsFloat();
    void testVolDataSliceOverload();
    void testVolDataOrthogonalOverload();
    void testVolDataRasRoundTrip();

    void cleanupTestCase();

private:
    // Flat-array path
    QVector<float> m_volData;
    QVector<int>   m_dims;
    Matrix4f       m_vox2ras;

    // MriVolData path — populated with same synthetic data
    MriVolData     m_vol;
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

    // Populate m_vol with equivalent data via MriVolData fields
    m_vol.width  = m_dims[0];
    m_vol.height = m_dims[1];
    m_vol.depth  = m_dims[2];
    m_vol.nframes = 1;
    m_vol.rasGood = true;
    m_vol.xsize = 1.0f;
    m_vol.ysize = 1.0f;
    m_vol.zsize = 1.0f;
    // Identity direction cosines → identity vox2ras
    m_vol.x_ras = Vector3f(1.0f, 0.0f, 0.0f);
    m_vol.y_ras = Vector3f(0.0f, 1.0f, 0.0f);
    m_vol.z_ras = Vector3f(0.0f, 0.0f, 1.0f);
    // c_ras at centre of volume so that P0 = 0 (origin at corner)
    m_vol.c_ras = Vector3f(m_dims[0] / 2.0f, m_dims[1] / 2.0f, m_dims[2] / 2.0f);

    // Build per-slice data (z slices, each width x height, float pixel format)
    m_vol.slices.resize(m_dims[2]);
    for (int z = 0; z < m_dims[2]; ++z) {
        MriSlice& s = m_vol.slices[z];
        s.pixelFormat = FIFFV_MRI_PIXEL_FLOAT;
        s.width  = m_dims[0];
        s.height = m_dims[1];
        int sliceSize = m_dims[0] * m_dims[1];
        s.pixelsFloat.resize(sliceSize);
        for (int y = 0; y < m_dims[1]; ++y)
            for (int x = 0; x < m_dims[0]; ++x)
                s.pixelsFloat[x + y * m_dims[0]] = static_cast<float>(x + y + z);
    }
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
// MriVolData convenience-API tests
//=============================================================================================================

void TestMriSlicer::testVolDataDimAccessors()
{
    QCOMPARE(m_vol.dimX(), m_dims[0]);
    QCOMPARE(m_vol.dimY(), m_dims[1]);
    QCOMPARE(m_vol.dimZ(), m_dims[2]);

    QVector<int> dims = m_vol.dims();
    QCOMPARE(dims.size(), 3);
    QCOMPARE(dims[0], m_dims[0]);
    QCOMPARE(dims[1], m_dims[1]);
    QCOMPARE(dims[2], m_dims[2]);
}

//=============================================================================================================

void TestMriSlicer::testVolDataVoxelDataAsFloat()
{
    QVector<float> flat = m_vol.voxelDataAsFloat();
    QCOMPARE(flat.size(), m_volData.size());

    // Verify content matches the flat-array data
    for (int i = 0; i < flat.size(); ++i) {
        QCOMPARE(flat[i], m_volData[i]);
    }
}

//=============================================================================================================

void TestMriSlicer::testVolDataSliceOverload()
{
    int midAxial = m_dims[2] / 2;

    // Extract via flat-array API using the MriVolData's own vox2ras
    Matrix4f volVox2ras = m_vol.computeVox2Ras();
    QVector<float> flat = m_vol.voxelDataAsFloat();
    QVector<int>   dims = m_vol.dims();

    MriSliceImage refSlice = MriSlicer::extractSlice(flat, dims, volVox2ras,
                                                      SliceOrientation::Axial, midAxial);
    // Extract via MriVolData overload
    MriSliceImage volSlice = MriSlicer::extractSlice(m_vol, SliceOrientation::Axial, midAxial);

    QCOMPARE(volSlice.orientation, refSlice.orientation);
    QCOMPARE(volSlice.sliceIndex, refSlice.sliceIndex);
    QCOMPARE(volSlice.width, refSlice.width);
    QCOMPARE(volSlice.height, refSlice.height);

    // Pixel data must match
    QCOMPARE(volSlice.pixels.rows(), refSlice.pixels.rows());
    QCOMPARE(volSlice.pixels.cols(), refSlice.pixels.cols());
    QVERIFY(volSlice.pixels.isApprox(refSlice.pixels, 1e-6f));
}

//=============================================================================================================

void TestMriSlicer::testVolDataOrthogonalOverload()
{
    // Use a RAS point in meters (computeVox2Ras returns meters)
    Matrix4f volVox2ras = m_vol.computeVox2Ras();
    QVector<float> flat = m_vol.voxelDataAsFloat();
    QVector<int>   dims = m_vol.dims();

    // Centre voxel in RAS (meters)
    Vector3i centerVox(m_dims[0] / 2, m_dims[1] / 2, m_dims[2] / 2);
    Vector3f centerRas = MriSlicer::voxelToRas(volVox2ras, centerVox);

    QVector<MriSliceImage> refSlices = MriSlicer::extractOrthogonal(flat, dims,
                                                                     volVox2ras, centerRas);
    QVector<MriSliceImage> volSlices = MriSlicer::extractOrthogonal(m_vol, centerRas);

    QCOMPARE(volSlices.size(), 3);
    QCOMPARE(volSlices.size(), refSlices.size());

    for (int i = 0; i < 3; ++i) {
        QCOMPARE(volSlices[i].orientation, refSlices[i].orientation);
        QCOMPARE(volSlices[i].sliceIndex, refSlices[i].sliceIndex);
        QVERIFY(volSlices[i].pixels.isApprox(refSlices[i].pixels, 1e-6f));
    }
}

//=============================================================================================================

void TestMriSlicer::testVolDataRasRoundTrip()
{
    // Convert a few voxel indices to RAS and back using MriVolData overloads
    QVector<Vector3i> testVoxels;
    testVoxels << Vector3i(5, 5, 5)
               << Vector3i(10, 3, 8)
               << Vector3i(1, 12, 7);

    Matrix4f volVox2ras = m_vol.computeVox2Ras();

    for (const auto& vox : testVoxels) {
        // Via flat-array API
        Vector3f refRas = MriSlicer::voxelToRas(volVox2ras, vox);
        Vector3i refVox = MriSlicer::rasToVoxel(volVox2ras, refRas);

        // Via MriVolData overloads
        Vector3f volRas = MriSlicer::voxelToRas(m_vol, vox);
        Vector3i volVox = MriSlicer::rasToVoxel(m_vol, volRas);

        QCOMPARE(volVox, refVox);
        float maxErr = (volRas - refRas).cwiseAbs().maxCoeff();
        QVERIFY2(maxErr < 1e-6f,
                 qPrintable(QString("MriVolData RAS mismatch=%1").arg(maxErr)));
    }
}

//=============================================================================================================

void TestMriSlicer::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMriSlicer)
#include "test_mri_slicer.moc"
