//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     dipoleobject.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Instanced-arrow renderable for fitted equivalent current dipoles, driven by QRhi instancing.
 *
 * DipoleObject converts an @ref INVLIB::InvEcdSet into a single
 * arrow mesh (cone + shaft) plus a per-dipole @c DipoleObject::InstanceData
 * stream that carries the 4x4 model matrix (translation + dipole
 * orientation), the RGBA colour (mapped from goodness-of-fit by the
 * active dipole colormap) and a selected flag.
 *
 * All dipoles draw with one @c drawIndexed call regardless of how
 * many were fitted; selection toggles only flip the @c isSelected
 * float in the instance buffer, so the renderer never rebuilds
 * geometry on interaction. Ray-picking is supported directly on the
 * instance data for tooltip / click selection.
 */

#ifndef DIPOLEOBJECT_H
#define DIPOLEOBJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QMatrix4x4>
#include <QVector>
#include <QVector3D>
#include <vector>
#include <memory>
#include <inv/dipole_fit/inv_ecd_set.h>

// Forward-declare QRhi types so that this header stays QRhi-free
class QRhi;
class QRhiBuffer;
class QRhiResourceUpdateBatch;

namespace DISP3DLIB
{

//=============================================================================================================
/**
 * Renderable dipole object that builds instanced arrow geometry from an InvEcdSet
 * and manages GPU buffers for QRhi-based rendering.
 *
 * @brief Renderable dipole arrow set with instanced GPU rendering for QRhi.
 *
 * @snippet ex_disp3d_scene/main.cpp dipole_object_usage
 */
class DISP3DSHARED_EXPORT DipoleObject
{
public:
    DipoleObject();
    ~DipoleObject();

    void load(const INVLIB::InvEcdSet& ecdSet);

    // Place all loaded dipoles with this transform (replaces any previous one)
    void applyTransform(const QMatrix4x4& trans);

    void updateBuffers(QRhi* rhi, QRhiResourceUpdateBatch* u);

    QRhiBuffer* vertexBuffer() const;
    QRhiBuffer* indexBuffer() const;
    QRhiBuffer* instanceBuffer() const;

    int indexCount() const
    {
        return m_indexCount;
    }
    int instanceCount() const
    {
        return m_instanceCount;
    }

    bool isVisible() const
    {
        return m_visible;
    }
    void setVisible(bool visible)
    {
        m_visible = visible;
    }

    //=========================================================================================================
    /**
     * Axis-aligned bounds of the (transformed) dipole positions.
     *
     * @param[out] min   Minimum corner.
     * @param[out] max   Maximum corner.
     * @return False if no dipoles are loaded.
     */
    bool boundingBox(QVector3D& min, QVector3D& max) const;

    void setSelected(int index, bool selected);

    //=========================================================================================================
    /**
     * Test ray intersection with dipoles.
     * 
     * @param[in] rayOrigin  Ray origin in world space.
     * @param[in] rayDir     Ray direction (normalized).
     * @param[out] dist      Distance to intersection.
     * @return Index of intersected dipole, or -1 if none.
     */
    int intersect(const QVector3D& rayOrigin, const QVector3D& rayDir, float& dist) const;

private:
    void createGeometry();

    /** @brief QRhi vertex, index, and instance buffers for dipole arrow GPU rendering. */
    struct GpuBuffers;
    std::unique_ptr<GpuBuffers> m_gpu;

    int m_indexCount = 0;
    int m_instanceCount = 0;
    bool m_visible = true;

    /**
     * @brief Interleaved vertex attributes for a single dipole arrow mesh.
     */
    struct VertexData
    {
        float x, y, z;
        float nx, ny, nz;
    };

    /**
     * @brief Per-instance transform and color for GPU-instanced dipole rendering.
     */
    // Instance data: Model Matrix (4x4) + Color (vec4)
    struct InstanceData
    {
        float model[16];
        float color[4];
        float isSelected; // 1.0 = selected, 0.0 = not
    };

    QByteArray m_vertexData;
    QByteArray m_indexData;
    QByteArray m_instanceData;
    QVector<QMatrix4x4> m_loadedModels; /**< Instance model matrices as loaded, before applyTransform(). */

    bool m_geometryDirty = false;
    bool m_instancesDirty = false;

    std::vector<QVector4D> m_originalColors;
};

} // namespace DISP3DLIB

#endif // DIPOLEOBJECT_H
