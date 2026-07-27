//=============================================================================================================
/**
 * @file     polylineobject.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.9
 * @date     July, 2026
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
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    PolylineObject class declaration.
 *
 * A polyline is an ordered chain of points drawn as connected segments, for
 * things like a head movement path, a dipole trajectory or a fibre tract.
 *
 * This exists as its own renderable rather than reusing NetworkObject, which
 * can also express a chain but is built for connectivity graphs: it draws a
 * sphere at every point, it prunes segments by weight in @ref
 * NetworkObject::setThreshold, and its node ids are qint16, which silently
 * wraps past 32767 points. None of that suits an ordered path, so a polyline
 * of N points here costs N-1 segment instances and nothing else.
 */

#ifndef DISP3DLIB_POLYLINEOBJECT_H
#define DISP3DLIB_POLYLINEOBJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QColor>
#include <QString>
#include <QVector>

#include <memory>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QRhi;
class QRhiBuffer;
class QRhiResourceUpdateBatch;

//=============================================================================================================
// DEFINE CLASS
//=============================================================================================================

// Deliberately at global scope, matching DipoleObject and NetworkObject. The
// other renderables in this directory are not namespaced either.

//=============================================================================================================
/**
 * Renders an ordered sequence of points as connected line segments.
 *
 * Each segment is one instance of a unit cylinder, using the same instance
 * layout as DipoleObject and NetworkObject so the existing Dipole pipeline can
 * draw it without a dedicated shader.
 *
 * @brief Connected line segments through an ordered list of points.
 */
class DISP3DSHARED_EXPORT PolylineObject
{
public:
    //=========================================================================================================
    /**
     * Constructs a PolylineObject without any points.
     */
    PolylineObject();

    //=========================================================================================================
    /**
     * Destroys the PolylineObject.
     */
    ~PolylineObject();

    //=========================================================================================================
    /**
     * Sets the points of the polyline.
     *
     * Fewer than two points clears the object, since a single point has no
     * segment to draw.
     *
     * @param[in] vecPoints     Points in scene coordinates, in order.
     */
    void setPoints(const QVector<Eigen::Vector3f>& vecPoints);

    //=========================================================================================================
    /**
     * Clears all points.
     */
    void clear();

    //=========================================================================================================
    /**
     * Sets the radius of the drawn segments.
     *
     * @param[in] fRadius   Segment radius in scene units.
     */
    void setRadius(float fRadius);

    //=========================================================================================================
    /**
     * Sets the colours the segments are graded between from start to end.
     *
     * Grading the line makes the direction of travel readable without any
     * extra decoration.
     *
     * @param[in] startColor    Colour of the first segment.
     * @param[in] endColor      Colour of the last segment.
     */
    void setGradient(const QColor& startColor, const QColor& endColor);

    //=========================================================================================================
    /**
     * Uploads or refreshes the GPU buffers.
     *
     * @param[in] rhi   The RHI to allocate through.
     * @param[in] u     Batch collecting the resource updates.
     */
    void updateBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u);

    //=========================================================================================================
    /**
     * @return Vertex buffer of the unit segment mesh.
     */
    QRhiBuffer* vertexBuffer() const;

    //=========================================================================================================
    /**
     * @return Index buffer of the unit segment mesh.
     */
    QRhiBuffer* indexBuffer() const;

    //=========================================================================================================
    /**
     * @return Per-segment instance buffer.
     */
    QRhiBuffer* instanceBuffer() const;

    //=========================================================================================================
    /**
     * @return Number of indices in the unit segment mesh.
     */
    int indexCount() const { return m_iIndexCount; }

    //=========================================================================================================
    /**
     * @return Number of segment instances, one less than the number of points.
     */
    int instanceCount() const { return m_iInstanceCount; }

    //=========================================================================================================
    /**
     * @return Whether the polyline is drawn.
     */
    bool isVisible() const { return m_bVisible; }

    //=========================================================================================================
    /**
     * Sets whether the polyline is drawn.
     *
     * @param[in] bVisible  True to draw the polyline.
     */
    void setVisible(bool bVisible) { m_bVisible = bVisible; }

    //=========================================================================================================
    /**
     * @return Whether there is at least one segment to draw.
     */
    bool hasData() const { return m_iInstanceCount > 0; }

private:
    //=========================================================================================================
    /**
     * Builds the unit cylinder used for every segment.
     */
    void createSegmentGeometry();

    //=========================================================================================================
    /**
     * Builds the per-segment instance transforms and colours.
     */
    void buildInstances();

    // Instance layout shared with DipoleObject and NetworkObject so that the
    // Dipole pipeline can render this object unchanged.
    struct InstanceData {
        float model[16];
        float color[4];
        float isSelected;   /**< Always 0, polylines are not pickable. */
    };

    struct VertexData {
        float x, y, z;
        float nx, ny, nz;
    };

    QVector<Eigen::Vector3f>    m_vecPoints;            /**< Points of the polyline, in order. */

    QByteArray                  m_vertexData;           /**< Unit segment vertices. */
    QByteArray                  m_indexData;            /**< Unit segment indices. */
    QByteArray                  m_instanceData;         /**< Per-segment instance data. */

    int                         m_iIndexCount;          /**< Index count of the unit segment. */
    int                         m_iInstanceCount;       /**< Number of segments. */

    float                       m_fRadius;              /**< Segment radius in scene units. */
    QColor                      m_startColor;           /**< Colour of the first segment. */
    QColor                      m_endColor;             /**< Colour of the last segment. */

    bool                        m_bVisible;             /**< Whether the polyline is drawn. */
    bool                        m_bGeometryDirty;       /**< Unit segment needs uploading. */
    bool                        m_bInstancesDirty;      /**< Instance buffer needs refreshing. */

    struct GpuResources;
    std::unique_ptr<GpuResources> m_gpu;                /**< RHI buffers, hidden to keep QRhi out of this header. */
};

#endif // DISP3DLIB_POLYLINEOBJECT_H
