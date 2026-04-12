//=============================================================================================================
/**
 * @file     mri_slicer.cpp
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
 * @brief    MriSlicer class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_slicer.h"
#include "mri_vol_data.h"

#include <Eigen/LU>

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MRILIB;
using namespace Eigen;

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

MriSliceImage MriSlicer::extractSlice(
    const QVector<float>& volData,
    const QVector<int>& dims,
    const Matrix4f& vox2ras,
    SliceOrientation orientation,
    int sliceIndex)
{
    const int dimX = dims[0];
    const int dimY = dims[1];
    const int dimZ = dims[2];

    MriSliceImage result;
    result.orientation = orientation;

    switch (orientation) {
    case SliceOrientation::Axial: {
        sliceIndex = std::clamp(sliceIndex, 0, dimZ - 1);
        result.width = dimX;
        result.height = dimY;
        result.sliceIndex = sliceIndex;
        result.pixels.resize(dimX, dimY);

        for (int iy = 0; iy < dimY; ++iy) {
            for (int ix = 0; ix < dimX; ++ix) {
                result.pixels(ix, iy) = volData[ix + dimX * (iy + dimY * sliceIndex)];
            }
        }

        // sliceToRas: maps (col, row) in the 2D image to RAS
        // col -> x voxel direction, row -> y voxel direction, fixed z = sliceIndex
        result.sliceToRas = Matrix4f::Zero();
        result.sliceToRas.col(0) = vox2ras.col(0); // x direction
        result.sliceToRas.col(1) = vox2ras.col(1); // y direction
        result.sliceToRas.col(2) = vox2ras.col(2); // z direction (unused for 2D but kept)
        Vector4f origin;
        origin << 0.0f, 0.0f, static_cast<float>(sliceIndex), 1.0f;
        result.sliceToRas.col(3) = vox2ras * origin;
        break;
    }
    case SliceOrientation::Coronal: {
        sliceIndex = std::clamp(sliceIndex, 0, dimY - 1);
        result.width = dimX;
        result.height = dimZ;
        result.sliceIndex = sliceIndex;
        result.pixels.resize(dimX, dimZ);

        for (int iz = 0; iz < dimZ; ++iz) {
            for (int ix = 0; ix < dimX; ++ix) {
                result.pixels(ix, iz) = volData[ix + dimX * (sliceIndex + dimY * iz)];
            }
        }

        // col -> x direction, row -> z direction, fixed y = sliceIndex
        result.sliceToRas = Matrix4f::Zero();
        result.sliceToRas.col(0) = vox2ras.col(0); // x direction
        result.sliceToRas.col(1) = vox2ras.col(2); // z direction
        result.sliceToRas.col(2) = vox2ras.col(1); // y direction (unused for 2D but kept)
        Vector4f originC;
        originC << 0.0f, static_cast<float>(sliceIndex), 0.0f, 1.0f;
        result.sliceToRas.col(3) = vox2ras * originC;
        break;
    }
    case SliceOrientation::Sagittal: {
        sliceIndex = std::clamp(sliceIndex, 0, dimX - 1);
        result.width = dimY;
        result.height = dimZ;
        result.sliceIndex = sliceIndex;
        result.pixels.resize(dimY, dimZ);

        for (int iz = 0; iz < dimZ; ++iz) {
            for (int iy = 0; iy < dimY; ++iy) {
                result.pixels(iy, iz) = volData[sliceIndex + dimX * (iy + dimY * iz)];
            }
        }

        // col -> y direction, row -> z direction, fixed x = sliceIndex
        result.sliceToRas = Matrix4f::Zero();
        result.sliceToRas.col(0) = vox2ras.col(1); // y direction
        result.sliceToRas.col(1) = vox2ras.col(2); // z direction
        result.sliceToRas.col(2) = vox2ras.col(0); // x direction (unused for 2D but kept)
        Vector4f originS;
        originS << static_cast<float>(sliceIndex), 0.0f, 0.0f, 1.0f;
        result.sliceToRas.col(3) = vox2ras * originS;
        break;
    }
    }

    // Normalize pixels to [0, 1]
    float minVal = result.pixels.minCoeff();
    float maxVal = result.pixels.maxCoeff();
    if (maxVal > minVal) {
        result.pixels = (result.pixels.array() - minVal) / (maxVal - minVal);
    } else {
        result.pixels.setZero();
    }

    return result;
}

//=============================================================================================================

QVector<MriSliceImage> MriSlicer::extractOrthogonal(
    const QVector<float>& volData,
    const QVector<int>& dims,
    const Matrix4f& vox2ras,
    const Vector3f& rasPoint)
{
    Vector3i voxel = rasToVoxel(vox2ras, rasPoint);

    QVector<MriSliceImage> slices;
    slices.reserve(3);
    slices.append(extractSlice(volData, dims, vox2ras, SliceOrientation::Axial, voxel.z()));
    slices.append(extractSlice(volData, dims, vox2ras, SliceOrientation::Coronal, voxel.y()));
    slices.append(extractSlice(volData, dims, vox2ras, SliceOrientation::Sagittal, voxel.x()));

    return slices;
}

//=============================================================================================================

Vector3i MriSlicer::rasToVoxel(const Matrix4f& vox2ras,
                                const Vector3f& rasPoint)
{
    Matrix4f ras2vox = vox2ras.inverse();
    Vector4f rasH;
    rasH << rasPoint, 1.0f;
    Vector4f voxH = ras2vox * rasH;

    return Vector3i(
        static_cast<int>(std::round(voxH.x())),
        static_cast<int>(std::round(voxH.y())),
        static_cast<int>(std::round(voxH.z()))
    );
}

//=============================================================================================================

Vector3f MriSlicer::voxelToRas(const Matrix4f& vox2ras,
                                const Vector3i& voxel)
{
    Vector4f voxH;
    voxH << static_cast<float>(voxel.x()),
             static_cast<float>(voxel.y()),
             static_cast<float>(voxel.z()),
             1.0f;
    Vector4f rasH = vox2ras * voxH;

    return rasH.head<3>();
}

//=============================================================================================================
// MriVolData convenience overloads
//=============================================================================================================

MriSliceImage MriSlicer::extractSlice(const MriVolData& vol,
                                       SliceOrientation orientation,
                                       int sliceIndex)
{
    return extractSlice(vol.voxelDataAsFloat(), vol.dims(),
                        vol.computeVox2Ras(), orientation, sliceIndex);
}

//=============================================================================================================

QVector<MriSliceImage> MriSlicer::extractOrthogonal(const MriVolData& vol,
                                                     const Vector3f& rasPoint)
{
    return extractOrthogonal(vol.voxelDataAsFloat(), vol.dims(),
                             vol.computeVox2Ras(), rasPoint);
}

//=============================================================================================================

Vector3i MriSlicer::rasToVoxel(const MriVolData& vol,
                                const Vector3f& rasPoint)
{
    return rasToVoxel(vol.computeVox2Ras(), rasPoint);
}

//=============================================================================================================

Vector3f MriSlicer::voxelToRas(const MriVolData& vol,
                                const Vector3i& voxel)
{
    return voxelToRas(vol.computeVox2Ras(), voxel);
}
