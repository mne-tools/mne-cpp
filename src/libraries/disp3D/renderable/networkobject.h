//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file networkobject.h
 * @since 2026
 * @date  March 2026
 * @brief Instanced connectivity-graph renderable: node spheres and edge cylinders coloured by weight through a named colormap.
 *
 * NetworkObject mirrors the @ref DipoleObject instancing pattern
 * but splits the scene into two meshes &mdash; nodes (spheres) and
 * edges (cylinders) &mdash; each with its own vertex / index / instance
 * buffer triple. The node mesh re-uses the dipole instance layout
 * for shader compatibility (model + colour + isSelected), so a
 * single pipeline serves both renderables.
 *
 * @ref setColormap selects the palette (Viridis, Hot, Jet, ...) used
 * to map edge weight to RGBA; @ref setThreshold prunes edges and
 * nodes below a normalised cut-off and regenerates the instance
 * stream without touching the underlying @ref CONNLIB::Network.
 */

#ifndef NETWORKOBJECT_H
#define NETWORKOBJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QMatrix4x4>
#include <QVector3D>
#include <QColor>
#include <vector>
#include <memory>

#include <conn/network/network.h>

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
class DISP3DSHARED_EXPORT NetworkObject
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
    void load(const CONNLIB::Network &network, const QString &sColormap = "Viridis");

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
    CONNLIB::Network m_network;
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
