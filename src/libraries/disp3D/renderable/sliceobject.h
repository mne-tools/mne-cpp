//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sliceobject.h
 * @since April 2026
 * @brief Single MRI volume slice rendered as a textured quad with adjustable axis, position, contrast and colormap.
 *
 * SliceObject mounts one orthogonal slice (axial, sagittal or
 * coronal) of an @ref MriVolume on a screen-aligned quad and uploads
 * the slice texture as @c R8 (grayscale) or @c RGBA8 (colour-mapped)
 * to the GPU. The slice index can be scrolled at interactive rates;
 * only the texture data is re-uploaded, the quad geometry is
 * constant.
 *
 * Used by the multimodal scene to mix MRI anatomy with the cortical
 * surface and overlays in a single render pass: the slice is drawn
 * first with depth-test enabled so cortical activity correctly
 * occludes / is occluded by the slice plane.
 */

#ifndef SLICEOBJECT_H
#define SLICEOBJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QImage>
#include <QMatrix4x4>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
/**
 * @brief Orientation for an orthogonal MRI slice.
 */
enum class SliceOrientation {
    Axial = 0,      /**< XY plane (Z = const). */
    Sagittal,        /**< YZ plane (X = const). */
    Coronal          /**< XZ plane (Y = const). */
};

//=============================================================================================================
/**
 * @brief Data model for a single 2-D MRI volume slice.
 *
 * Holds the greyscale slice image, orientation metadata, and the
 * slice-to-world transform. Generates a textured quad and vertex/UV data
 * for upload to a QRhi renderer via the slice.vert / slice.frag shaders.
 */
class DISP3DSHARED_EXPORT SliceObject
{
public:
    SliceObject();

    //=========================================================================================================
    /**
     * Set the slice image data and orientation.
     *
     * @param[in] image         Greyscale slice image (QImage::Format_Grayscale8 or 16).
     * @param[in] orientation   Axial, Sagittal, or Coronal.
     * @param[in] sliceIndex    Index along the perpendicular axis (voxel units).
     * @param[in] voxelToWorld  4×4 voxel-to-RAS transform from the MRI header.
     */
    void setSlice(const QImage& image,
                  SliceOrientation orientation,
                  int sliceIndex,
                  const Eigen::Matrix4d& voxelToWorld);

    //=========================================================================================================
    /**
     * @return Current slice orientation.
     */
    SliceOrientation orientation() const;

    //=========================================================================================================
    /**
     * @return Current slice index (voxel units along the perpendicular axis).
     */
    int sliceIndex() const;

    //=========================================================================================================
    /**
     * @return The slice image.
     */
    const QImage& image() const;

    //=========================================================================================================
    /**
     * @return 4×4 matrix that maps the unit quad [0,1]² into world (RAS) coordinates.
     */
    QMatrix4x4 sliceToWorld() const;

    //=========================================================================================================
    /**
     * Intensity windowing parameters for the fragment shader.
     *
     * @param[in] center  Window center in normalised intensity [0,1].
     * @param[in] width   Window width in normalised intensity [0,1].
     */
    void setWindowLevel(float center, float width);

    //=========================================================================================================
    /**
     * @return Window center.
     */
    float windowCenter() const;

    //=========================================================================================================
    /**
     * @return Window width.
     */
    float windowWidth() const;

    //=========================================================================================================
    /**
     * @param[in] opacity   Slice transparency [0,1].
     */
    void setOpacity(float opacity);

    //=========================================================================================================
    /**
     * @return Current opacity.
     */
    float opacity() const;

    //=========================================================================================================
    /**
     * Generate quad vertex buffer: 4 vertices × (3 pos + 2 UV) = 20 floats.
     *
     * @param[out] vertices     Output vertex data.
     */
    void generateQuadVertices(QVector<float>& vertices) const;

    //=========================================================================================================
    /**
     * Generate quad index buffer: 2 triangles = 6 indices.
     *
     * @param[out] indices      Output index data.
     */
    static void generateQuadIndices(QVector<unsigned int>& indices);

private:
    QImage              m_image;
    SliceOrientation    m_orientation = SliceOrientation::Axial;
    int                 m_sliceIndex = 0;
    Eigen::Matrix4d     m_voxelToWorld = Eigen::Matrix4d::Identity();
    float               m_windowCenter = 0.5f;
    float               m_windowWidth  = 1.0f;
    float               m_opacity      = 1.0f;

    // Cached corners in world coordinates (computed in setSlice)
    Eigen::Vector3d     m_corner00;  // (0,0) UV corner
    Eigen::Vector3d     m_corner10;  // (1,0) UV corner
    Eigen::Vector3d     m_corner01;  // (0,1) UV corner
    Eigen::Vector3d     m_corner11;  // (1,1) UV corner
};

} // namespace DISP3DLIB

#endif // SLICEOBJECT_H
