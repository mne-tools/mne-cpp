//=============================================================================================================
/**
 * @file     networkobject.h
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
 * @brief    NetworkObject class declaration.
 *
 */

#ifndef NETWORKOBJECT_H
#define NETWORKOBJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include <QMatrix4x4>
#include <QVector3D>
#include <QColor>
#include <vector>
#include <memory>

#include <connectivity/network/network.h>

// Forward-declare QRhi types so that this header stays QRhi-free
class QRhi;
class QRhiBuffer;
class QRhiResourceUpdateBatch;

//=============================================================================================================
/**
 * NetworkObject renders connectivity network nodes (instanced spheres) and edges (instanced cylinders)
 * using the same instanced rendering pipeline as DipoleObject for the QRhi backend.
 *
 * @brief Renderable network visualization for QRhi.
 */
class DISP3DRHISHARED_EXPORT NetworkObject
{
public:
    //=========================================================================================================
    /**
     * Default constructor.
     */
    NetworkObject();

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~NetworkObject();

    //=========================================================================================================
    /**
     * Load network data and generate node/edge instances.
     *
     * @param[in] network       The connectivity network to visualize.
     * @param[in] sColormap     Colormap name for weight-based coloring (e.g. "Viridis", "Hot").
     */
    void load(const CONNECTIVITYLIB::Network &network, const QString &sColormap = "Viridis");

    //=========================================================================================================
    /**
     * Update the threshold and regenerate visible edges/nodes.
     *
     * @param[in] dThreshold    New threshold value (0.0 – 1.0).
     */
    void setThreshold(double dThreshold);

    //=========================================================================================================
    /**
     * Set the colormap used for edge/node coloring.
     *
     * @param[in] sColormap     Colormap name (e.g. "Hot", "Viridis", "Jet").
     */
    void setColormap(const QString &sColormap);

    //=========================================================================================================
    /**
     * Upload or update GPU buffers for node geometry.
     *
     * @param[in] rhi         QRhi instance.
     * @param[in] u           Resource update batch.
     */
    void updateNodeBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u);

    //=========================================================================================================
    /**
     * Upload or update GPU buffers for edge geometry.
     *
     * @param[in] rhi         QRhi instance.
     * @param[in] u           Resource update batch.
     */
    void updateEdgeBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u);

    // ── Node accessors ──────────────────────────────────────────────────
    QRhiBuffer* nodeVertexBuffer() const;
    QRhiBuffer* nodeIndexBuffer() const;
    QRhiBuffer* nodeInstanceBuffer() const;
    int nodeIndexCount() const { return m_nodeIndexCount; }
    int nodeInstanceCount() const { return m_nodeInstanceCount; }

    // ── Edge accessors ──────────────────────────────────────────────────
    QRhiBuffer* edgeVertexBuffer() const;
    QRhiBuffer* edgeIndexBuffer() const;
    QRhiBuffer* edgeInstanceBuffer() const;
    int edgeIndexCount() const { return m_edgeIndexCount; }
    int edgeInstanceCount() const { return m_edgeInstanceCount; }

    // ── Visibility ──────────────────────────────────────────────────────
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    bool hasData() const { return !m_network.isEmpty(); }

private:
    /**
     * @brief Per-instance transform, color, and weight for GPU-instanced network edge rendering.
     */
    // Instance data: identical layout to DipoleObject for shader compatibility
    // Model Matrix (4x4) + Color (vec4) + isSelected (float)
    struct InstanceData {
        float model[16];
        float color[4];
        float isSelected;   // Always 0.0 for networks
    };

    /**
     * @brief Interleaved vertex attributes for network node and edge meshes.
     */
    struct VertexData {
        float x, y, z;
        float nx, ny, nz;
    };

    void createNodeGeometry();
    void createEdgeGeometry();
    void buildNodeInstances();
    void buildEdgeInstances();

    // ── Network data ────────────────────────────────────────────────────
    CONNECTIVITYLIB::Network m_network;
    QString m_colormap = "Viridis";

    // ── Node GPU resources ──────────────────────────────────────────────
    /** @brief QRhi vertex, index, and instance buffers for network node and edge GPU rendering. */
    struct GpuBuffers;
    std::unique_ptr<GpuBuffers> m_gpu;

    QByteArray m_nodeVertexData;
    QByteArray m_nodeIndexData;
    QByteArray m_nodeInstanceData;
    int m_nodeIndexCount = 0;
    int m_nodeInstanceCount = 0;
    bool m_nodeGeometryDirty = false;
    bool m_nodeInstancesDirty = false;

    // ── Edge GPU resources ──────────────────────────────────────────────
    QByteArray m_edgeVertexData;
    QByteArray m_edgeIndexData;
    QByteArray m_edgeInstanceData;
    int m_edgeIndexCount = 0;
    int m_edgeInstanceCount = 0;
    bool m_edgeGeometryDirty = false;
    bool m_edgeInstancesDirty = false;

    bool m_visible = true;
};

#endif // NETWORKOBJECT_H
