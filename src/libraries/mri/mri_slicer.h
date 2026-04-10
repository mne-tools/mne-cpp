//=============================================================================================================
/**
 * @file     mri_slicer.h
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
 * @brief    MriSlicer class declaration.
 *
 */

#ifndef MRI_SLICER_H
#define MRI_SLICER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MRILIB
//=============================================================================================================

namespace MRILIB {

//=============================================================================================================
/**
 * Slice orientation for orthogonal MRI volume slicing.
 */
enum class SliceOrientation { Axial, Coronal, Sagittal };

//=============================================================================================================
/**
 * Holds extracted 2D slice image data from a 3D MRI volume.
 *
 * @brief Extracted MRI slice image.
 */
struct MRISHARED_EXPORT MriSliceImage
{
    Eigen::MatrixXf pixels;          /**< 2D pixel data, normalised 0-1. */
    int width;                       /**< Width of the slice image in pixels. */
    int height;                      /**< Height of the slice image in pixels. */
    SliceOrientation orientation;    /**< Orientation of the slice. */
    int sliceIndex;                  /**< Index along the slicing axis. */
    Eigen::Matrix4f sliceToRas;      /**< 4x4 transform placing the slice in RAS space. */
};

//=============================================================================================================
/**
 * Provides static methods for extracting 2D slices from 3D MRI volumes.
 *
 * Supports axial, coronal, and sagittal slice extraction with automatic
 * normalisation and RAS coordinate mapping.
 *
 * @brief MRI volume slicer.
 */
class MRISHARED_EXPORT MriSlicer
{
public:
    //=========================================================================================================
    /**
     * Extract a 2D slice from a 3D volume stored as a flat array.
     *
     * @param[in] volData       Flat array of voxel intensities (dimX x dimY x dimZ, x-fastest).
     * @param[in] dims          Volume dimensions {dimX, dimY, dimZ}.
     * @param[in] vox2ras       4x4 voxel-to-RAS transform.
     * @param[in] orientation   Slice orientation (Axial, Coronal, or Sagittal).
     * @param[in] sliceIndex    Index along the slicing axis.
     *
     * @return The extracted slice image.
     */
    static MriSliceImage extractSlice(
        const QVector<float>& volData,
        const QVector<int>& dims,
        const Eigen::Matrix4f& vox2ras,
        SliceOrientation orientation,
        int sliceIndex);

    //=========================================================================================================
    /**
     * Extract all three orthogonal slices at a given RAS point.
     *
     * @param[in] volData   Flat array of voxel intensities.
     * @param[in] dims      Volume dimensions {dimX, dimY, dimZ}.
     * @param[in] vox2ras   4x4 voxel-to-RAS transform.
     * @param[in] rasPoint  RAS coordinate to slice through.
     *
     * @return Vector of three MriSliceImage (axial, coronal, sagittal).
     */
    static QVector<MriSliceImage> extractOrthogonal(
        const QVector<float>& volData,
        const QVector<int>& dims,
        const Eigen::Matrix4f& vox2ras,
        const Eigen::Vector3f& rasPoint);

    //=========================================================================================================
    /**
     * Convert RAS coordinate to voxel index.
     *
     * @param[in] vox2ras   4x4 voxel-to-RAS transform.
     * @param[in] rasPoint  RAS coordinate.
     *
     * @return Voxel index (rounded to nearest integer).
     */
    static Eigen::Vector3i rasToVoxel(const Eigen::Matrix4f& vox2ras,
                                       const Eigen::Vector3f& rasPoint);

    //=========================================================================================================
    /**
     * Convert voxel index to RAS coordinate.
     *
     * @param[in] vox2ras   4x4 voxel-to-RAS transform.
     * @param[in] voxel     Voxel index.
     *
     * @return RAS coordinate.
     */
    static Eigen::Vector3f voxelToRas(const Eigen::Matrix4f& vox2ras,
                                       const Eigen::Vector3i& voxel);
};

} // namespace MRILIB

#endif // MRI_SLICER_H
