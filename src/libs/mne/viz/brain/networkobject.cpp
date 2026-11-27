//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     networkobject.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Node-sphere and edge-cylinder instanced geometry construction with colormap-driven weight encoding.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "networkobject.h"

#include "instancing.h"
#include "../figures/colormap.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <numbers>
#include <string>
#include <vector>

using namespace mne;
using namespace mne::viz;
using namespace mne::viz;
using namespace Eigen;

namespace mne::viz
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NetworkObject::NetworkObject() = default;

//=============================================================================================================

NetworkObject::~NetworkObject() = default;

//=============================================================================================================

InstancedStreams NetworkObject::nodeStreams() const
{
    return {{m_nodeVertexData.data(), static_cast<std::uint32_t>(m_nodeVertexData.size()), m_nodeGeometryGeneration},
            {m_nodeIndexData.data(), static_cast<std::uint32_t>(m_nodeIndexData.size()), m_nodeGeometryGeneration},
            {m_nodeInstanceData.data(), static_cast<std::uint32_t>(m_nodeInstanceCount * sizeof(InstanceData)), m_nodeInstanceGeneration},
            m_nodeIndexCount,
            m_nodeInstanceCount};
}

InstancedStreams NetworkObject::edgeStreams() const
{
    return {{m_edgeVertexData.data(), static_cast<std::uint32_t>(m_edgeVertexData.size()), m_edgeGeometryGeneration},
            {m_edgeIndexData.data(), static_cast<std::uint32_t>(m_edgeIndexData.size()), m_edgeGeometryGeneration},
            {m_edgeInstanceData.data(), static_cast<std::uint32_t>(m_edgeInstanceCount * sizeof(InstanceData)), m_edgeInstanceGeneration},
            m_edgeIndexCount,
            m_edgeInstanceCount};
}

//=============================================================================================================

void NetworkObject::load(const Network& network, const std::string& sColormap)
{
    m_network = network;
    m_colormap = sColormap;

    createNodeGeometry();
    createEdgeGeometry();
    buildNodeInstances();
    buildEdgeInstances();
}

//=============================================================================================================

void NetworkObject::setThreshold(double dThreshold)
{
    m_network.set_threshold(dThreshold);
    buildNodeInstances();
    buildEdgeInstances();
}

//=============================================================================================================

void NetworkObject::setColormap(const std::string& sColormap)
{
    m_colormap = sColormap;
    buildNodeInstances();
    buildEdgeInstances();
}

//=============================================================================================================

void NetworkObject::createNodeGeometry()
{
    if (!m_nodeVertexData.empty())
        return;

    // Create an icosphere (subdivision level 1) for nodes
    const int subdivisions = 1;
    const float radius = 1.0f; // Unit sphere, scaled per-instance

    // Start with icosahedron
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    std::vector<Eigen::Vector3f> vertices = {
        Eigen::Vector3f(-1, t, 0).normalized() * radius,
        Eigen::Vector3f(1, t, 0).normalized() * radius,
        Eigen::Vector3f(-1, -t, 0).normalized() * radius,
        Eigen::Vector3f(1, -t, 0).normalized() * radius,
        Eigen::Vector3f(0, -1, t).normalized() * radius,
        Eigen::Vector3f(0, 1, t).normalized() * radius,
        Eigen::Vector3f(0, -1, -t).normalized() * radius,
        Eigen::Vector3f(0, 1, -t).normalized() * radius,
        Eigen::Vector3f(t, 0, -1).normalized() * radius,
        Eigen::Vector3f(t, 0, 1).normalized() * radius,
        Eigen::Vector3f(-t, 0, -1).normalized() * radius,
        Eigen::Vector3f(-t, 0, 1).normalized() * radius,
    };

    std::vector<uint32_t> indices = {
        0,
        11,
        5,
        0,
        5,
        1,
        0,
        1,
        7,
        0,
        7,
        10,
        0,
        10,
        11,
        1,
        5,
        9,
        5,
        11,
        4,
        11,
        10,
        2,
        10,
        7,
        6,
        7,
        1,
        8,
        3,
        9,
        4,
        3,
        4,
        2,
        3,
        2,
        6,
        3,
        6,
        8,
        3,
        8,
        9,
        4,
        9,
        5,
        2,
        4,
        11,
        6,
        2,
        10,
        8,
        6,
        7,
        9,
        8,
        1,
    };

    // Subdivide
    for (int s = 0; s < subdivisions; ++s) {
        std::vector<uint32_t> newIndices;
        std::map<uint64_t, uint32_t> midpointCache;

        auto getMidpoint = [&](uint32_t i0, uint32_t i1) -> uint32_t {
            uint64_t key = static_cast<uint64_t>(std::min(i0, i1)) << 32 | std::max(i0, i1);
            auto it = midpointCache.find(key);
            if (it != midpointCache.end())
                return it->second;

            const Eigen::Vector3f mid = ((vertices[i0] + vertices[i1]) / 2.0f).normalized() * radius;
            uint32_t idx = static_cast<uint32_t>(vertices.size());
            vertices.push_back(mid);
            midpointCache[key] = idx;
            return idx;
        };

        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t a = indices[i], b = indices[i + 1], c = indices[i + 2];
            uint32_t ab = getMidpoint(a, b);
            uint32_t bc = getMidpoint(b, c);
            uint32_t ca = getMidpoint(c, a);

            newIndices.insert(newIndices.end(), {a, ab, ca});
            newIndices.insert(newIndices.end(), {b, bc, ab});
            newIndices.insert(newIndices.end(), {c, ca, bc});
            newIndices.insert(newIndices.end(), {ab, bc, ca});
        }

        indices = std::move(newIndices);
    }

    // Build vertex data with normals (normal = normalized position for sphere)
    std::vector<VertexData> vd;
    vd.reserve(vertices.size());
    for (const auto& v : vertices) {
        const Eigen::Vector3f n = v.normalized();
        vd.push_back({v.x(), v.y(), v.z(), n.x(), n.y(), n.z()});
    }

    m_nodeIndexCount = static_cast<int>(indices.size());

    m_nodeVertexData.resize(vd.size() * sizeof(VertexData));
    memcpy(m_nodeVertexData.data(), vd.data(), m_nodeVertexData.size());

    m_nodeIndexData.resize(indices.size() * sizeof(uint32_t));
    memcpy(m_nodeIndexData.data(), indices.data(), m_nodeIndexData.size());

    ++m_nodeGeometryGeneration;
}

//=============================================================================================================

void NetworkObject::createEdgeGeometry()
{
    if (!m_edgeVertexData.empty())
        return;

    // Create a unit cylinder along Y axis (height=1, radius=1, scaled per-instance)
    const int segments = 8;
    const float radius = 1.0f;
    const float halfHeight = 0.5f;

    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;

    // Top center (0)
    vertices.push_back({0, halfHeight, 0, 0, 1, 0});
    // Bottom center (1)
    vertices.push_back({0, -halfHeight, 0, 0, -1, 0});

    // Side vertices: top ring (2..2+segments-1), bottom ring (2+segments..2+2*segments-1)
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * static_cast<float>(std::numbers::pi) * i / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);

        const Eigen::Vector3f normal = Eigen::Vector3f(x, 0.0f, z).normalized();

        // Top side vertex
        vertices.push_back({x, halfHeight, z, normal.x(), normal.y(), normal.z()});
        // Bottom side vertex
        vertices.push_back({x, -halfHeight, z, normal.x(), normal.y(), normal.z()});
    }

    // Top cap vertices (for proper normals)
    int topCapStart = static_cast<int>(vertices.size());
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * static_cast<float>(std::numbers::pi) * i / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        vertices.push_back({x, halfHeight, z, 0, 1, 0});
    }

    // Bottom cap vertices
    int botCapStart = static_cast<int>(vertices.size());
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * static_cast<float>(std::numbers::pi) * i / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        vertices.push_back({x, -halfHeight, z, 0, -1, 0});
    }

    // Side faces
    int sideStart = 2;
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        int topCur = sideStart + i * 2;
        int botCur = sideStart + i * 2 + 1;
        int topNext = sideStart + next * 2;
        int botNext = sideStart + next * 2 + 1;

        indices.insert(indices.end(), {static_cast<uint32_t>(topCur), static_cast<uint32_t>(topNext), static_cast<uint32_t>(botCur)});
        indices.insert(indices.end(), {static_cast<uint32_t>(botCur), static_cast<uint32_t>(topNext), static_cast<uint32_t>(botNext)});
    }

    // Top cap
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        indices.push_back(0); // center
        indices.push_back(topCapStart + i);
        indices.push_back(topCapStart + next);
    }

    // Bottom cap
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        indices.push_back(1); // center
        indices.push_back(botCapStart + next);
        indices.push_back(botCapStart + i);
    }

    m_edgeIndexCount = static_cast<int>(indices.size());

    m_edgeVertexData.resize(vertices.size() * sizeof(VertexData));
    memcpy(m_edgeVertexData.data(), vertices.data(), m_edgeVertexData.size());

    m_edgeIndexData.resize(indices.size() * sizeof(uint32_t));
    memcpy(m_edgeIndexData.data(), indices.data(), m_edgeIndexData.size());

    ++m_edgeGeometryGeneration;
}

//=============================================================================================================

void NetworkObject::buildNodeInstances()
{
    if (m_network.empty()) {
        m_nodeInstanceCount = 0;
        ++m_nodeInstanceGeneration;
        return;
    }

    const auto& nodes = m_network.nodes();
    int iMaxDegree = m_network.degree_range(EdgeSet::thresholded).second;
    if (iMaxDegree == 0)
        iMaxDegree = 1;

    const VisualizationInfo& vizInfo = m_network.visualization_info();
    const std::string& colormap = vizInfo.colormap.empty() ? m_colormap : vizInfo.colormap;

    std::vector<InstanceData> instances;
    instances.reserve(nodes.size());

    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        const int degree = m_network.degree(i, EdgeSet::thresholded);
        if (degree == 0)
            continue;

        const Vector3f& vert = nodes[i].position();

        // Scale: nodes with higher degree are larger
        // Range: 0.0006 to 0.005 (same as mne_viz_qt)
        float scaleFactor = (static_cast<float>(degree) / static_cast<float>(iMaxDegree)) * (0.005f - 0.0006f) + 0.0006f;

        InstanceData inst;
        storeMatrix(modelMatrix(vert, Eigen::Quaternionf::Identity(), Eigen::Vector3f::Constant(scaleFactor)), inst.model);

        // Color: colormap-based or fixed
        if (vizInfo.coloring == VisualizationInfo::Coloring::colormap) {
            float normalized = static_cast<float>(degree) / static_cast<float>(iMaxDegree);
            storeColor(map_color(colormap, normalized), inst.color);
            inst.color[3] = std::pow(normalized, 4.0f);
        } else {
            inst.color[0] = vizInfo.node_color[0] / 255.0f;
            inst.color[1] = vizInfo.node_color[1] / 255.0f;
            inst.color[2] = vizInfo.node_color[2] / 255.0f;
            inst.color[3] = vizInfo.node_color[3] / 255.0f;
        }
        inst.isSelected = 0.0f;

        instances.push_back(inst);
    }

    m_nodeInstanceCount = static_cast<int>(instances.size());
    m_nodeInstanceData.resize(m_nodeInstanceCount * sizeof(InstanceData));
    if (m_nodeInstanceCount > 0) {
        memcpy(m_nodeInstanceData.data(), instances.data(), m_nodeInstanceData.size());
    }
    ++m_nodeInstanceGeneration;
}

//=============================================================================================================

void NetworkObject::buildEdgeInstances()
{
    if (m_network.empty()) {
        m_edgeInstanceCount = 0;
        ++m_edgeInstanceGeneration;
        return;
    }

    const auto& nodes = m_network.nodes();

    const auto [dMinWeight, dMaxWeight] = m_network.weight_range(EdgeSet::thresholded);
    double dWeightRange = dMaxWeight - dMinWeight;
    if (dWeightRange == 0.0)
        dWeightRange = 1.0;

    const VisualizationInfo& vizInfo = m_network.visualization_info();
    const std::string& colormap = vizInfo.colormap.empty() ? m_colormap : vizInfo.colormap;

    std::vector<InstanceData> instances;
    instances.reserve(m_network.edge_count(EdgeSet::thresholded));

    for (const NetworkEdge& edge : m_network.edges()) {
        if (!edge.active())
            continue;

        const Vector3f& vStart = nodes[edge.source()].position();
        const Vector3f& vEnd = nodes[edge.target()].position();

        if (vStart == vEnd)
            continue;

        double dWeight = std::fabs(edge.weight());
        if (dWeight == 0.0)
            continue;

        float normalizedWeight = static_cast<float>(std::fabs((dWeight - dMinWeight) / dWeightRange));

        // Cylinder radius: proportional to weight, range 0.0001 to 0.001
        float edgeRadius = 0.0001f + normalizedWeight * 0.0009f;

        InstanceData inst;
        storeMatrix(segmentMatrix(vStart, vEnd, edgeRadius), inst.model);

        // Color
        if (vizInfo.coloring == VisualizationInfo::Coloring::colormap) {
            float normalized = (dMaxWeight != 0.0) ? static_cast<float>(std::fabs(dWeight / dMaxWeight)) : 0.0f;
            storeColor(map_color(colormap, normalized), inst.color);
            inst.color[3] = std::pow(normalized, 1.5f);
        } else {
            inst.color[0] = vizInfo.edge_color[0] / 255.0f;
            inst.color[1] = vizInfo.edge_color[1] / 255.0f;
            inst.color[2] = vizInfo.edge_color[2] / 255.0f;
            inst.color[3] = vizInfo.edge_color[3] / 255.0f;
        }
        inst.isSelected = 0.0f;

        instances.push_back(inst);
    }

    m_edgeInstanceCount = static_cast<int>(instances.size());
    m_edgeInstanceData.resize(m_edgeInstanceCount * sizeof(InstanceData));
    if (m_edgeInstanceCount > 0) {
        memcpy(m_edgeInstanceData.data(), instances.data(), m_edgeInstanceData.size());
    }
    ++m_edgeInstanceGeneration;
}

//=============================================================================================================

} // namespace mne::viz
