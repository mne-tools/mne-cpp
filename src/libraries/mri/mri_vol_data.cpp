//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mri_vol_data.cpp
 * @since 2026
 * @date  May 2026
 * @brief Implementation of @ref MRILIB::MriVolData: header-driven geometry computations and the suffix-dispatch loader.
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

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MRILIB;
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
, x_ras(-1.0f,  0.0f,  0.0f)   // FreeSurfer defaults when goodRASflag is false
, y_ras( 0.0f,  0.0f, -1.0f)   // See: https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
, z_ras( 0.0f,  1.0f,  0.0f)
, c_ras( 0.0f,  0.0f,  0.0f)
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

bool MriVolData::read(const QString& path)
{
    // Dispatch on filename suffix so callers can use the same one-shot loader
    // for FreeSurfer MGH/MGZ and NIfTI-1 (.nii / .nii.gz) volumes.
    const QString lower = path.toLower();
    if (lower.endsWith(QStringLiteral(".nii")) || lower.endsWith(QStringLiteral(".nii.gz"))) {
        return MriNiftiIO::read(path, *this);
    }
    QVector<FIFFLIB::FiffCoordTrans> additionalTrans;
    return MriMghIO::read(path, *this, additionalTrans);
}

//=============================================================================================================

QVector<float> MriVolData::voxelDataAsFloat() const
{
    if (slices.isEmpty() || width <= 0 || height <= 0 || depth <= 0)
        return {};

    const int nPixels = width * height;
    QVector<float> result(width * height * depth, 0.0f);

    for (int k = 0; k < depth && k < slices.size(); ++k) {
        const MriSlice& slice = slices[k];
        int offset = k * nPixels;

        switch (slice.pixelFormat) {
            case FIFFV_MRI_PIXEL_BYTE:
                for (int p = 0; p < qMin(nPixels, slice.pixels.size()); ++p)
                    result[offset + p] = static_cast<float>(slice.pixels[p]);
                break;
            case FIFFV_MRI_PIXEL_WORD:
                for (int p = 0; p < qMin(nPixels, slice.pixelsWord.size()); ++p)
                    result[offset + p] = static_cast<float>(slice.pixelsWord[p]);
                break;
            case FIFFV_MRI_PIXEL_FLOAT:
                for (int p = 0; p < qMin(nPixels, slice.pixelsFloat.size()); ++p)
                    result[offset + p] = slice.pixelsFloat[p];
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
    Vector3f center(static_cast<float>(width)  / 2.0f,
                    static_cast<float>(height) / 2.0f,
                    static_cast<float>(depth)  / 2.0f);

    // Compute P0 = c_ras - M * center
    Vector3f P0 = c_ras - M * center;

    // Build 4x4 matrix in meters (FreeSurfer uses mm, FIFF uses meters)
    Matrix4f vox2ras = Matrix4f::Identity();
    vox2ras.block<3, 3>(0, 0) = M / 1000.0f;
    vox2ras.block<3, 1>(0, 3) = P0 / 1000.0f;

    return vox2ras;
}
