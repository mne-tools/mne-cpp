//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mri_vol_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Implementation of @ref mne::MriVolData: header-driven geometry computations and the suffix-dispatch loader.
 *
 * Houses the non-trivial methods @ref MriVolData declares:
 *
 * - @c voxToSurfRAS() / @c surfRASToVox() build the
 * canonical 4\u00d74 affines from the MGH @c Mdc direction-
 * cosine matrix, @c spacing vector and @c c_ras centre,
 * matching FreeSurfer's @c MRIxfmCRS2XYZtkreg() exactly
 * (the same convention MNE-Python's @c _read_mri_info
 * returns) so source-space and BEM tooling stay
 * coordinate-compatible.
 * - @c voxelDataAsFloat() flattens the slice-of-slices
 * representation back into the column-major x-fastest
 * buffer that downstream resamplers and exporters expect,
 * promoting UCHAR / SHORT / INT inputs to float on the fly.
 * - @c read() inspects the path suffix and dispatches to
 * @ref MriMghIO (.mgh / .mgz), @ref MriNiftiIO
 * (.nii / .nii.gz), or the COR directory loader, giving
 * callers a single \"load whatever this is\" entry point.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_vol_data.h"

#include "mri_mgh_io.h"
#include "mri_nifti_io.h"
#include <fiff/fiff_file.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MriVolData::MriVolData()
: version(0)
, width(0)
, height(0)
, depth(0)
, nframes(0)
, type(MRI_UCHAR)
, dof(0)
, rasGood(false)
, xsize(1.0f)
, ysize(1.0f)
, zsize(1.0f)
, x_ras(-1.0f, 0.0f, 0.0f) // FreeSurfer defaults when goodRASflag is false
, y_ras(0.0f, 0.0f, -1.0f) // See: https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
, z_ras(0.0f, 1.0f, 0.0f)
, c_ras(0.0f, 0.0f, 0.0f)
, TR(0.0f)
, flipAngle(0.0f)
, TE(0.0f)
, TI(0.0f)
, FoV(0.0f)
{
}

//=============================================================================================================

bool MriVolData::isValid() const
{
    return (version == MRI_MGH_VERSION && width > 0 && height > 0 && depth > 0);
}

//=============================================================================================================

bool MriVolData::read(const std::string& path)
{
    // One loader for FreeSurfer MGH/MGZ and NIfTI-1 (.nii / .nii.gz), chosen by suffix
    const std::string lower = mne::to_lower(path);
    if (lower.ends_with(".nii") || lower.ends_with(".nii.gz")) {
        return MriNiftiIO::read(path, *this);
    }
    std::vector<mne::FiffCoordTrans> additionalTrans;
    return MriMghIO::read(path, *this, additionalTrans);
}

//=============================================================================================================

std::vector<float> MriVolData::voxelDataAsFloat() const
{
    if (slices.empty() || width <= 0 || height <= 0 || depth <= 0)
        return {};

    const std::size_t nPixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::vector<float> result(nPixels * static_cast<std::size_t>(depth), 0.0f);
    const std::size_t nSlices = std::min(static_cast<std::size_t>(depth), slices.size());
    for (std::size_t k = 0; k < nSlices; ++k) {
        const MriSlice& slice = slices[k];
        float* out = result.data() + k * nPixels;
        switch (slice.pixelFormat) {
            case FIFFV_MRI_PIXEL_BYTE:
                std::transform(slice.pixels.begin(), slice.pixels.begin() + std::ptrdiff_t(std::min(nPixels, slice.pixels.size())), out, [](unsigned char v) { return static_cast<float>(v); });
                break;
            case FIFFV_MRI_PIXEL_WORD:
                std::transform(slice.pixelsWord.begin(), slice.pixelsWord.begin() + std::ptrdiff_t(std::min(nPixels, slice.pixelsWord.size())), out, [](unsigned short v) { return static_cast<float>(v); });
                break;
            case FIFFV_MRI_PIXEL_FLOAT:
                std::copy_n(slice.pixelsFloat.begin(), std::min(nPixels, slice.pixelsFloat.size()), out);
                break;
        }
    }
    return result;
}

//=============================================================================================================

Matrix4f MriVolData::computeVox2Ras() const
{
    //
    // Build voxel-to-surface-RAS transform (FreeSurfer convention).
    //
    // The direction cosine matrix Mdc stores the orientation of each voxel axis
    // (columns are x, y, z directions). Scaling by voxel size gives the actual
    // spacing matrix M:
    //
    //   M = Mdc * diag(xsize, ysize, zsize)
    //
    // The origin P0 in RAS coordinates is:
    //
    //   P0 = c_ras - M * (dim/2)
    //
    // The full 4x4 vox2ras matrix is:
    //
    //   | M   P0 |     (in mm, then converted to meters for FIFF)
    //   | 0    1 |
    //
    // Reference: https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
    //

    // Construct M = Mdc * D (scale direction cosines by voxel sizes)
    Matrix3f M;
    M.col(0) = x_ras * xsize;
    M.col(1) = y_ras * ysize;
    M.col(2) = z_ras * zsize;

    // Compute center voxel
    Vector3f center(static_cast<float>(width) / 2.0f,
                    static_cast<float>(height) / 2.0f,
                    static_cast<float>(depth) / 2.0f);

    // Compute P0 = c_ras - M * center
    Vector3f P0 = c_ras - M * center;

    // Build 4x4 matrix in meters (FreeSurfer uses mm, FIFF uses meters)
    Matrix4f vox2ras = Matrix4f::Identity();
    vox2ras.block<3, 3>(0, 0) = M / 1000.0f;
    vox2ras.block<3, 1>(0, 3) = P0 / 1000.0f;

    return vox2ras;
}

//=============================================================================================================

Matrix4f MriVolData::computeVox2RasTkr() const
{
    // Same as computeVox2Ras() but with c_ras = (0,0,0).
    // This gives the tkregister (surface RAS) coordinate system
    // that FreeSurfer surfaces use.
    Matrix3f M;
    M.col(0) = x_ras * xsize;
    M.col(1) = y_ras * ysize;
    M.col(2) = z_ras * zsize;

    Vector3f center(static_cast<float>(width) / 2.0f,
                    static_cast<float>(height) / 2.0f,
                    static_cast<float>(depth) / 2.0f);

    Vector3f P0 = -M * center; // c_ras = (0,0,0) for tkRAS

    Matrix4f vox2ras_tkr = Matrix4f::Identity();
    vox2ras_tkr.block<3, 3>(0, 0) = M / 1000.0f;
    vox2ras_tkr.block<3, 1>(0, 3) = P0 / 1000.0f;

    return vox2ras_tkr;
}
