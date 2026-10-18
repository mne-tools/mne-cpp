//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     networkobject.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Instanced connectivity-graph renderable: node spheres and edge cylinders coloured by weight through a named colormap.
 *
 * NetworkObject mirrors the @ref DISP3DLIB::DipoleObject instancing pattern
 * but splits the scene into two meshes &mdash; nodes (spheres) and
 * edges (cylinders) &mdash; each with its own vertex / index / instance
 * buffer triple. The node mesh re-uses the dipole instance layout
 * for shader compatibility (model + colour + isSelected), so a
 * single pipeline serves both renderables.
 *
 * @ref DISP3DLIB::NetworkObject::setColormap "setColormap" selects the palette (Viridis, Hot, Jet, ...) used
 * to map edge weight to RGBA; @ref DISP3DLIB::NetworkObject::setThreshold "setThreshold" prunes edges and
 * nodes below a normalised cut-off and regenerates the instance
 * stream without touching the underlying @ref CONNECTIVITYLIB::Network.
 */

#ifndef NETWORKOBJECT_H
#define NETWORKOBJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include "../core/rendertypes.h"

#include <QMatrix4x4>
#include <QVector3D>
#include <QColor>
#include <vector>
#include <memory>

#include <connectivity/network/network.h>


namespace DISP3DLIB
{

//=============================================================================================================
/**
 * NetworkObject renders connectivity network nodes (instanced spheres) and edges (instanced cylinders)
 * using the same instanced rendering pipeline as DipoleObject.
 *
 * @brief Renderable connectivity network visualization.
 *
 * @snippet ex_disp3d_scene/main.cpp network_object_usage
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
    void load(const CONNECTIVITYLIB::Network& network, const QString& sColormap = "Viridis");

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
    void setColormap(const QString& sColormap);

    //=========================================================================================================
    /**
     * @return CPU streams of the node sphere and of one instance per node.
     */
    InstancedStreams nodeStreams() const;

    /**
     * @return CPU streams of the edge cylinder and of one instance per edge.
     */
    InstancedStreams edgeStreams() const;

    /** @return Number of nodes drawn, i.e. those touching an edge above the threshold. */
    int nodeInstanceCount() const
    {
        return m_nodeInstanceCount;
    }

    /** @return Number of edges drawn above the threshold. */
    int edgeInstanceCount() const
    {
        return m_edgeInstanceCount;
    }

    /**
     * @return Identities of the node and edge meshes in the renderer's GPU cache.
     */
    const RenderKey& nodeRenderKey() const
    {
        return m_nodeRenderKey;
    }
    const RenderKey& edgeRenderKey() const
    {
        return m_edgeRenderKey;
    }

    // ── Visibility ──────────────────────────────────────────────────────
    bool isVisible() const
    {
        return m_visible;
    }
    void setVisible(bool visible)
    {
        m_visible = visible;
    }

    bool hasData() const
    {
        return !m_network.isEmpty();
    }

private:
    /**
     * @brief Per-instance transform, color, and weight for GPU-instanced network edge rendering.
     */
    // Instance data: identical layout to DipoleObject for shader compatibility
    // Model Matrix (4x4) + Color (vec4) + isSelected (float)
    struct InstanceData
    {
        float model[16];
        float color[4];
        float isSelected; // Always 0.0 for networks
    };

    /**
     * @brief Interleaved vertex attributes for network node and edge meshes.
     */
    struct VertexData
    {
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
    RenderKey m_nodeRenderKey;
    RenderKey m_edgeRenderKey;

    QByteArray m_nodeVertexData;
    QByteArray m_nodeIndexData;
    QByteArray m_nodeInstanceData;
    int m_nodeIndexCount = 0;
    int m_nodeInstanceCount = 0;
    quint64 m_nodeGeometryGeneration = 0;
    quint64 m_nodeInstanceGeneration = 0;

    // ── Edge GPU resources ──────────────────────────────────────────────
    QByteArray m_edgeVertexData;
    QByteArray m_edgeIndexData;
    QByteArray m_edgeInstanceData;
    int m_edgeIndexCount = 0;
    int m_edgeInstanceCount = 0;
    quint64 m_edgeGeometryGeneration = 0;
    quint64 m_edgeInstanceGeneration = 0;

    bool m_visible = true;
};

} // namespace DISP3DLIB

#endif // NETWORKOBJECT_H
