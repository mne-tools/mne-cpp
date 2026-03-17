//=============================================================================================================
/**
 * @file     mri_vol_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    MriVolData class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_vol_data.h"

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
