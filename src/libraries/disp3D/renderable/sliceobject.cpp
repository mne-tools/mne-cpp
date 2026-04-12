//=============================================================================================================
/**
 * @file     sliceobject.cpp
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
 * @brief    SliceObject class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sliceobject.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SliceObject::SliceObject()
    : m_corner00(Eigen::Vector3d::Zero())
    , m_corner10(Eigen::Vector3d::UnitX())
    , m_corner01(Eigen::Vector3d::UnitY())
    , m_corner11(Eigen::Vector3d::UnitX() + Eigen::Vector3d::UnitY())
{
}

//=============================================================================================================

void SliceObject::setSlice(const QImage& image,
                           SliceOrientation orientation,
                           int sliceIndex,
                           const Eigen::Matrix4d& voxelToWorld)
{
    m_image        = image;
    m_orientation  = orientation;
    m_sliceIndex   = sliceIndex;
    m_voxelToWorld = voxelToWorld;

    // Image dimensions define the number of voxels along each in-plane axis.
    const int w = m_image.width();
    const int h = m_image.height();

    // Build the four corner positions in voxel coordinates, then transform to world.
    // u runs along image columns, v along image rows.
    Eigen::Vector4d c00, c10, c01, c11;

    switch (m_orientation) {
    case SliceOrientation::Axial:
        //  u → X,  v → Y,  slice along Z
        c00 << 0, 0, sliceIndex, 1;
        c10 << w, 0, sliceIndex, 1;
        c01 << 0, h, sliceIndex, 1;
        c11 << w, h, sliceIndex, 1;
        break;
    case SliceOrientation::Sagittal:
        //  u → Y,  v → Z,  slice along X
        c00 << sliceIndex, 0, 0, 1;
        c10 << sliceIndex, w, 0, 1;
        c01 << sliceIndex, 0, h, 1;
        c11 << sliceIndex, w, h, 1;
        break;
    case SliceOrientation::Coronal:
        //  u → X,  v → Z,  slice along Y
        c00 << 0, sliceIndex, 0, 1;
        c10 << w, sliceIndex, 0, 1;
        c01 << 0, sliceIndex, h, 1;
        c11 << w, sliceIndex, h, 1;
        break;
    }

    // Transform voxel → world (RAS)
    Eigen::Vector4d w00 = m_voxelToWorld * c00;
    Eigen::Vector4d w10 = m_voxelToWorld * c10;
    Eigen::Vector4d w01 = m_voxelToWorld * c01;
    Eigen::Vector4d w11 = m_voxelToWorld * c11;

    m_corner00 = w00.head<3>();
    m_corner10 = w10.head<3>();
    m_corner01 = w01.head<3>();
    m_corner11 = w11.head<3>();
}

//=============================================================================================================

SliceOrientation SliceObject::orientation() const
{
    return m_orientation;
}

//=============================================================================================================

int SliceObject::sliceIndex() const
{
    return m_sliceIndex;
}

//=============================================================================================================

const QImage& SliceObject::image() const
{
    return m_image;
}

//=============================================================================================================

QMatrix4x4 SliceObject::sliceToWorld() const
{
    // Build a 4×4 matrix that maps the unit quad [0,1]² to the world-space quad.
    // Column 0: edge along u (corner10 – corner00)
    // Column 1: edge along v (corner01 – corner00)
    // Column 3: translation  (corner00)
    Eigen::Vector3d u = m_corner10 - m_corner00;
    Eigen::Vector3d v = m_corner01 - m_corner00;
    Eigen::Vector3d n = u.cross(v).normalized();

    QMatrix4x4 mat;
    mat.setToIdentity();
    mat(0, 0) = static_cast<float>(u.x());
    mat(1, 0) = static_cast<float>(u.y());
    mat(2, 0) = static_cast<float>(u.z());

    mat(0, 1) = static_cast<float>(v.x());
    mat(1, 1) = static_cast<float>(v.y());
    mat(2, 1) = static_cast<float>(v.z());

    mat(0, 2) = static_cast<float>(n.x());
    mat(1, 2) = static_cast<float>(n.y());
    mat(2, 2) = static_cast<float>(n.z());

    mat(0, 3) = static_cast<float>(m_corner00.x());
    mat(1, 3) = static_cast<float>(m_corner00.y());
    mat(2, 3) = static_cast<float>(m_corner00.z());

    return mat;
}

//=============================================================================================================

void SliceObject::setWindowLevel(float center, float width)
{
    m_windowCenter = center;
    m_windowWidth  = width;
}

//=============================================================================================================

float SliceObject::windowCenter() const
{
    return m_windowCenter;
}

//=============================================================================================================

float SliceObject::windowWidth() const
{
    return m_windowWidth;
}

//=============================================================================================================

void SliceObject::setOpacity(float opacity)
{
    m_opacity = opacity;
}

//=============================================================================================================

float SliceObject::opacity() const
{
    return m_opacity;
}

//=============================================================================================================

void SliceObject::generateQuadVertices(QVector<float>& vertices) const
{
    // 4 vertices: pos(3) + uv(2) = 5 floats each  → 20 floats total
    vertices.resize(20);
    float* p = vertices.data();

    // Vertex 0: corner00, uv(0,0)
    *p++ = static_cast<float>(m_corner00.x());
    *p++ = static_cast<float>(m_corner00.y());
    *p++ = static_cast<float>(m_corner00.z());
    *p++ = 0.0f;  *p++ = 0.0f;

    // Vertex 1: corner10, uv(1,0)
    *p++ = static_cast<float>(m_corner10.x());
    *p++ = static_cast<float>(m_corner10.y());
    *p++ = static_cast<float>(m_corner10.z());
    *p++ = 1.0f;  *p++ = 0.0f;

    // Vertex 2: corner01, uv(0,1)
    *p++ = static_cast<float>(m_corner01.x());
    *p++ = static_cast<float>(m_corner01.y());
    *p++ = static_cast<float>(m_corner01.z());
    *p++ = 0.0f;  *p++ = 1.0f;

    // Vertex 3: corner11, uv(1,1)
    *p++ = static_cast<float>(m_corner11.x());
    *p++ = static_cast<float>(m_corner11.y());
    *p++ = static_cast<float>(m_corner11.z());
    *p++ = 1.0f;  *p++ = 1.0f;
}

//=============================================================================================================

void SliceObject::generateQuadIndices(QVector<unsigned int>& indices)
{
    // Two triangles: (0,1,2) and (2,1,3)
    indices = { 0, 1, 2,  2, 1, 3 };
}
