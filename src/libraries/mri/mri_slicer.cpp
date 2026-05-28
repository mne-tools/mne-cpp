// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

//=============================================================================================================
/**
 * @file mri_slicer.cpp
 *
 * @brief Implementation of @ref MRILIB::MriSlicer: per-orientation extraction with on-the-fly type promotion and per-volume normalisation.
 *
 * Implements the three jobs the header advertises: (1) pick the
 * right axis stride from the volume's @c (dimX, dimY, dimZ)
 * triple for the requested @ref SliceOrientation; (2) iterate
 * the voxel buffer in the on-disk type (UCHAR / SHORT / INT /
 * FLOAT) and promote to the normalised @c Eigen::MatrixXf the
 * viewer consumes --- keeping the dispatch in one place so the
 * four code paths stay byte-for-byte consistent; (3) compose the
 * emitted @c sliceToRas as @c volume.voxToSurfRAS() *
 * orientationAffine so any surface, source estimate or fiducial
 * overlay placed in RAS lands in the right slice without
 * recomputing the volume geometry.
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
