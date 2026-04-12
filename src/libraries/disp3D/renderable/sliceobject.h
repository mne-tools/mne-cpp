//=============================================================================================================
/**
 * @file     sliceobject.h
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
 * @brief    SliceObject class declaration — MRI volume slice renderable.
 *
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
