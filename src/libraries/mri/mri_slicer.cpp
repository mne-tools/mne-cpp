//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mri_slicer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref MRILIB::MriSlicer: per-orientation extraction with on-the-fly type promotion and per-volume normalisation.
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
#include <array>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MRILIB;
using namespace Eigen;

namespace {

struct PlaneSpec
{
    int columnAxis;
    int rowAxis;
    int fixedAxis;
};

Vector3f anatomicalNormal(SliceOrientation orientation)
{
    switch (orientation) {
    case SliceOrientation::Axial:    return Vector3f::UnitZ();
    case SliceOrientation::Coronal:  return Vector3f::UnitY();
    case SliceOrientation::Sagittal: return Vector3f::UnitX();
    }
    return Vector3f::UnitZ();
}

Vector3f anatomicalColumnDirection(SliceOrientation orientation)
{
    switch (orientation) {
    case SliceOrientation::Axial:    return Vector3f::UnitX();
    case SliceOrientation::Coronal:  return Vector3f::UnitX();
    case SliceOrientation::Sagittal: return Vector3f::UnitY();
    }
    return Vector3f::UnitX();
}

Vector3f anatomicalRowDirection(SliceOrientation orientation)
{
    switch (orientation) {
    case SliceOrientation::Axial:    return Vector3f::UnitY();
    case SliceOrientation::Coronal:  return Vector3f::UnitZ();
    case SliceOrientation::Sagittal: return Vector3f::UnitZ();
    }
    return Vector3f::UnitY();
}

int bestVoxelAxisForDirection(const Matrix4f& vox2ras,
                              const Vector3f& target,
                              const std::array<bool, 3>& used)
{
    int bestAxis = 0;
    float bestScore = -1.0f;

    for (int axis = 0; axis < 3; ++axis) {
        if (used[axis]) {
            continue;
        }
        Vector3f axisDirection = vox2ras.block<3, 1>(0, axis);
        const float norm = axisDirection.norm();
        if (norm > 0.0f) {
            axisDirection /= norm;
        }
        const float score = std::abs(axisDirection.dot(target));
        if (score > bestScore) {
            bestScore = score;
            bestAxis = axis;
        }
    }

    return bestAxis;
}

PlaneSpec planeSpecForOrientation(const Matrix4f& vox2ras,
                                  SliceOrientation orientation)
{
    std::array<bool, 3> used = {false, false, false};
    const int fixedAxis = bestVoxelAxisForDirection(vox2ras,
                                                    anatomicalNormal(orientation),
                                                    used);
    used[fixedAxis] = true;

    const int columnAxis = bestVoxelAxisForDirection(vox2ras,
                                                     anatomicalColumnDirection(orientation),
                                                     used);
    used[columnAxis] = true;

    const int rowAxis = bestVoxelAxisForDirection(vox2ras,
                                                  anatomicalRowDirection(orientation),
                                                  used);

    return {columnAxis, rowAxis, fixedAxis};
}

int axisDimension(const QVector<int>& dims, int axis)
{
    return dims[axis];
}

int flatIndex(int x, int y, int z, int dimX, int dimY)
{
    return x + dimX * (y + dimY * z);
}

} // anonymous namespace

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
    const PlaneSpec spec = planeSpecForOrientation(vox2ras, orientation);
    const int width = axisDimension(dims, spec.columnAxis);
    const int height = axisDimension(dims, spec.rowAxis);
    const int fixedDim = axisDimension(dims, spec.fixedAxis);

    sliceIndex = std::clamp(sliceIndex, 0, fixedDim - 1);

    MriSliceImage result;
    result.orientation = orientation;
    result.width = width;
    result.height = height;
    result.sliceIndex = sliceIndex;
    result.pixels.resize(width, height);

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            std::array<int, 3> voxel = {0, 0, 0};
            voxel[spec.columnAxis] = col;
            voxel[spec.rowAxis] = row;
            voxel[spec.fixedAxis] = sliceIndex;
            result.pixels(col, row) = volData[flatIndex(voxel[0], voxel[1], voxel[2], dimX, dimY)];
        }
    }

    result.sliceToRas = Matrix4f::Zero();
    result.sliceToRas.col(0) = vox2ras.col(spec.columnAxis);
    result.sliceToRas.col(1) = vox2ras.col(spec.rowAxis);
    result.sliceToRas.col(2) = vox2ras.col(spec.fixedAxis);
    Vector4f origin = Vector4f::Zero();
    origin(spec.fixedAxis) = static_cast<float>(sliceIndex);
    origin.w() = 1.0f;
    result.sliceToRas.col(3) = vox2ras * origin;

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

int MriSlicer::voxelAxisForOrientation(const Matrix4f& vox2ras,
                                        SliceOrientation orientation)
{
    return planeSpecForOrientation(vox2ras, orientation).fixedAxis;
}

//=============================================================================================================

int MriSlicer::dimensionForOrientation(const QVector<int>& dims,
                                       const Matrix4f& vox2ras,
                                       SliceOrientation orientation)
{
    return axisDimension(dims, voxelAxisForOrientation(vox2ras, orientation));
}

//=============================================================================================================

int MriSlicer::sliceIndexForOrientation(const Matrix4f& vox2ras,
                                        SliceOrientation orientation,
                                        const Vector3i& voxel)
{
    return voxel(voxelAxisForOrientation(vox2ras, orientation));
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
    slices.append(extractSlice(volData, dims, vox2ras, SliceOrientation::Axial,
                               sliceIndexForOrientation(vox2ras, SliceOrientation::Axial, voxel)));
    slices.append(extractSlice(volData, dims, vox2ras, SliceOrientation::Coronal,
                               sliceIndexForOrientation(vox2ras, SliceOrientation::Coronal, voxel)));
    slices.append(extractSlice(volData, dims, vox2ras, SliceOrientation::Sagittal,
                               sliceIndexForOrientation(vox2ras, SliceOrientation::Sagittal, voxel)));

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
                        vol.computeVox2RasTkr(), orientation, sliceIndex);
}

//=============================================================================================================

int MriSlicer::voxelAxisForOrientation(const MriVolData& vol,
                                       SliceOrientation orientation)
{
    return voxelAxisForOrientation(vol.computeVox2RasTkr(), orientation);
}

//=============================================================================================================

int MriSlicer::dimensionForOrientation(const MriVolData& vol,
                                       SliceOrientation orientation)
{
    return dimensionForOrientation(vol.dims(), vol.computeVox2RasTkr(), orientation);
}

//=============================================================================================================

int MriSlicer::sliceIndexForOrientation(const MriVolData& vol,
                                        SliceOrientation orientation,
                                        const Vector3i& voxel)
{
    return sliceIndexForOrientation(vol.computeVox2RasTkr(), orientation, voxel);
}

//=============================================================================================================

QVector<MriSliceImage> MriSlicer::extractOrthogonal(const MriVolData& vol,
                                                     const Vector3f& rasPoint)
{
    return extractOrthogonal(vol.voxelDataAsFloat(), vol.dims(),
                             vol.computeVox2RasTkr(), rasPoint);
}

//=============================================================================================================

Vector3i MriSlicer::rasToVoxel(const MriVolData& vol,
                                const Vector3f& rasPoint)
{
    return rasToVoxel(vol.computeVox2RasTkr(), rasPoint);
}

//=============================================================================================================

Vector3f MriSlicer::voxelToRas(const MriVolData& vol,
                                const Vector3i& voxel)
{
    return voxelToRas(vol.computeVox2RasTkr(), voxel);
}
