//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mri_slicer.h
 * @since 2026
 * @date  May 2026
 * @brief Orthogonal-plane resampler that turns a 3D @ref MriVolData into the 2D textures consumed by the slice viewer.
 *
 * The slicer is the bridge between the format-specific MRI
 * readers (@ref MriMghIO, @ref MriNiftiIO, @ref MriCorIO) and
 * the @c MriSlicesPlugin / @c MriSlicesView widgets that
 * render axial / coronal / sagittal cross-sections in
 * @c mne_analyze_studio. Two cooperating types live here:
 *
 * - @ref SliceOrientation --- a strong enum picking the slicing
 * axis (Axial / Coronal / Sagittal); used both as the user-
 * facing radio-button value in the slice viewer and as the
 * index-permutation key inside the slicer itself.
 * - @ref MriSliceImage --- the result type: a normalised
 * (0..1) @c Eigen::MatrixXf pixel buffer ready to upload
 * as an OpenGL texture, paired with its own slice-to-RAS
 * 4\u00d74 transform so the viewer can overlay surface meshes,
 * source estimates or fiducials in the correct world space
 * without re-deriving the geometry from the volume header.
 *
 * The static @ref MriSlicer::extract() method does the actual
 * work: it picks the right axis stride from the volume header,
 * dispatches by on-disk voxel type (UCHAR / SHORT / INT /
 * FLOAT) so quantisation matches the source, normalises by the
 * per-volume max so windowing stays stable across slices, and
 * composes @c sliceToRas from the volume's @c voxToSurfRAS()
 * pre-multiplied by the orientation-specific in-slice affine.
 */
//=============================================================================================================

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
// FORWARD DECLARATIONS
//=============================================================================================================

class MriVolData;

//=============================================================================================================
/**
 * Slice orientation for orthogonal MRI volume slicing.
 */
enum class SliceOrientation { Axial, Coronal, Sagittal };

//=============================================================================================================
/**
 * @brief Single 2D MRI cross-section produced by @ref MriSlicer (pixel buffer + RAS metadata).
 *
 * Carries one orthogonal-plane sample of a 3D @ref MriVolData ready for
 * upload to the slice viewer: a normalised 8-bit grayscale pixel buffer,
 * the in-volume slice index it was sampled at, and the bookkeeping the
 * widget needs to map mouse picks back to RAS millimetres.
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
 * @brief Orthogonal-plane resampler (axial / coronal / sagittal) from @ref MriVolData to @ref MriSliceImage.
 *
 * Pure @c static methods --- no instance state is owned. For a given
 * orientation and slice index, samples the source volume at the matching
 * row/column/depth plane, normalises voxel intensities into 8-bit grayscale
 * via the volume's percentile-clipped window, and emits the result with
 * the slice→RAS transform the viewer needs to draw rulers and crosshairs
 * in millimetres.
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

    //=========================================================================================================
    // MriVolData convenience overloads
    //=========================================================================================================

    /**
     * Extract a 2D slice from an MriVolData volume.
     *
     * @param[in] vol           Loaded MRI volume.
     * @param[in] orientation   Slice orientation.
     * @param[in] sliceIndex    Index along the slicing axis.
     *
     * @return The extracted slice image.
     */
    static MriSliceImage extractSlice(const MriVolData& vol,
                                       SliceOrientation orientation,
                                       int sliceIndex);

    /**
     * Extract all three orthogonal slices at a given RAS point.
     *
     * @param[in] vol       Loaded MRI volume.
     * @param[in] rasPoint  RAS coordinate to slice through.
     *
     * @return Vector of three MriSliceImage (axial, coronal, sagittal).
     */
    static QVector<MriSliceImage> extractOrthogonal(const MriVolData& vol,
                                                     const Eigen::Vector3f& rasPoint);

    /**
     * Convert RAS coordinate to voxel index using a volume's transform.
     *
     * @param[in] vol       Loaded MRI volume.
     * @param[in] rasPoint  RAS coordinate.
     *
     * @return Voxel index (rounded to nearest integer).
     */
    static Eigen::Vector3i rasToVoxel(const MriVolData& vol,
                                       const Eigen::Vector3f& rasPoint);

    /**
     * Convert voxel index to RAS coordinate using a volume's transform.
     *
     * @param[in] vol    Loaded MRI volume.
     * @param[in] voxel  Voxel index.
     *
     * @return RAS coordinate.
     */
    static Eigen::Vector3f voxelToRas(const MriVolData& vol,
                                       const Eigen::Vector3i& voxel);
};

} // namespace MRILIB

#endif // MRI_SLICER_H
