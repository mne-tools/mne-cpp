//=============================================================================================================
/**
 * @file     test_mri_vol_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
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
 * @brief    Tests for MriVolData and MriSlice (header geometry, vox2ras,
 *           slice construction). Validates voxel-to-RAS transform against
 *           the FreeSurfer MGH specification.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mri/mri_vol_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MRILIB;
using namespace Eigen;

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestMriVolData : public QObject
{
    Q_OBJECT

private slots:

    //=========================================================================================================
    // MriSlice tests
    //=========================================================================================================

    void testSliceDefaults()
    {
        MriSlice slice;
        QCOMPARE(slice.width, 0);
        QCOMPARE(slice.height, 0);
        QCOMPARE(slice.pixelFormat, 1);  // FIFFV_MRI_PIXEL_BYTE
        QVERIFY(qAbs(slice.scale - 1.0f) < 1e-6f);
    }

    void testSlicePixelData()
    {
        MriSlice slice;
        slice.width = 256;
        slice.height = 256;
        slice.pixels.resize(256 * 256);
        slice.pixels.fill(128);

        QCOMPARE(slice.pixels.size(), 256 * 256);
        QCOMPARE(slice.pixels[0], static_cast<unsigned char>(128));
    }

    //=========================================================================================================
    // MriVolData default construction
    //=========================================================================================================

    void testVolDataDefaults()
    {
        MriVolData vol;
        QVERIFY(!vol.isValid());
        QCOMPARE(vol.version, 0);
        QCOMPARE(vol.width, 0);
        QCOMPARE(vol.height, 0);
        QCOMPARE(vol.depth, 0);
        QCOMPARE(vol.nframes, 0);
    }

    //=========================================================================================================
    // Vox2RAS transform — validate against FreeSurfer convention.
    //
    // Reference: FreeSurfer wiki MGH format specification
    //   M = Mdc * diag(xsize, ysize, zsize)
    //   P0 = c_ras - M * (dim/2)
    //   vox2ras = [M P0; 0 0 0 1]
    //
    // For a 256^3 volume, 1mm isotropic, default direction cosines
    // (-1,0,0), (0,0,-1), (0,1,0), c_ras=(0,0,0):
    //   M = diag(-1,0,0) * 1 = col0=(-1,0,0), etc.
    //   P0 = (0,0,0) - M*(128,128,128) = (128, 128, -128) in mm
    //   Convert to meters: (0.128, 0.128, -0.128)
    //=========================================================================================================

    void testVox2RasIdentity()
    {
        MriVolData vol;
        vol.width = 256;
        vol.height = 256;
        vol.depth = 256;
        vol.nframes = 1;
        vol.rasGood = true;
        vol.xsize = 1.0f;
        vol.ysize = 1.0f;
        vol.zsize = 1.0f;

        // Default FreeSurfer direction cosines
        vol.x_ras = Vector3f(-1.0f, 0.0f, 0.0f);
        vol.y_ras = Vector3f(0.0f, 0.0f, -1.0f);
        vol.z_ras = Vector3f(0.0f, 1.0f, 0.0f);
        vol.c_ras = Vector3f(0.0f, 0.0f, 0.0f);

        Matrix4f V = vol.computeVox2Ras();

        // Check it's 4x4
        QCOMPARE(V.rows(), 4);
        QCOMPARE(V.cols(), 4);

        // Bottom row should be (0,0,0,1)
        QVERIFY(qAbs(V(3,0)) < 1e-6f);
        QVERIFY(qAbs(V(3,1)) < 1e-6f);
        QVERIFY(qAbs(V(3,2)) < 1e-6f);
        QVERIFY(qAbs(V(3,3) - 1.0f) < 1e-6f);

        // Rotation part: M = Mdc * diag(xsize, ysize, zsize) in meters
        // x_ras * xsize/1000 = (-0.001, 0, 0), etc.
        QVERIFY(qAbs(V(0,0) - (-0.001f)) < 1e-7f);

        // Translation P0 should place center voxel at c_ras
        // Center voxel (128,128,128) should map to c_ras (0,0,0)
        Vector4f center;
        center << 128.0f, 128.0f, 128.0f, 1.0f;
        Vector4f ras = V * center;
        QVERIFY(qAbs(ras(0)) < 1e-3f);  // Should be ~0
        QVERIFY(qAbs(ras(1)) < 1e-3f);
        QVERIFY(qAbs(ras(2)) < 1e-3f);
    }

    //=========================================================================================================
    // Vox2RAS with non-zero c_ras — validates translation offset
    //=========================================================================================================

    void testVox2RasWithOffset()
    {
        MriVolData vol;
        vol.width = 256;
        vol.height = 256;
        vol.depth = 256;
        vol.nframes = 1;
        vol.rasGood = true;
        vol.xsize = 1.0f;
        vol.ysize = 1.0f;
        vol.zsize = 1.0f;
        vol.x_ras = Vector3f(-1.0f, 0.0f, 0.0f);
        vol.y_ras = Vector3f(0.0f, 0.0f, -1.0f);
        vol.z_ras = Vector3f(0.0f, 1.0f, 0.0f);

        // Offset from origin
        vol.c_ras = Vector3f(10.0f, 20.0f, 30.0f);

        Matrix4f V = vol.computeVox2Ras();

        // Center voxel should now map to c_ras (10,20,30) mm → (0.01, 0.02, 0.03) m
        Vector4f center;
        center << 128.0f, 128.0f, 128.0f, 1.0f;
        Vector4f ras = V * center;
        QVERIFY(qAbs(ras(0) - 0.01f) < 1e-3f);
        QVERIFY(qAbs(ras(1) - 0.02f) < 1e-3f);
        QVERIFY(qAbs(ras(2) - 0.03f) < 1e-3f);
    }

    //=========================================================================================================
    // Non-isotropic spacing
    //=========================================================================================================

    void testVox2RasAnisotropic()
    {
        MriVolData vol;
        vol.width = 128;
        vol.height = 128;
        vol.depth = 64;
        vol.nframes = 1;
        vol.rasGood = true;
        vol.xsize = 2.0f;   // 2mm
        vol.ysize = 2.0f;   // 2mm
        vol.zsize = 3.0f;   // 3mm
        vol.x_ras = Vector3f(-1.0f, 0.0f, 0.0f);
        vol.y_ras = Vector3f(0.0f, 0.0f, -1.0f);
        vol.z_ras = Vector3f(0.0f, 1.0f, 0.0f);
        vol.c_ras = Vector3f(0.0f, 0.0f, 0.0f);

        Matrix4f V = vol.computeVox2Ras();

        // Voxel spacing should scale the direction cosines
        // x_ras * 2mm/1000 = (-0.002, 0, 0)
        QVERIFY(qAbs(V(0,0) - (-0.002f)) < 1e-7f);
    }
};

QTEST_GUILESS_MAIN(TestMriVolData)
#include "test_mri_vol_data.moc"
