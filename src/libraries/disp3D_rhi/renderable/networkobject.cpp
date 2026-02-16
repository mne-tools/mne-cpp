//=============================================================================================================
/**
 * @file     networkobject.cpp
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
 * @brief    NetworkObject class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "networkobject.h"

#include <rhi/qrhi.h>

#include <connectivity/network/networknode.h>
#include <connectivity/network/networkedge.h>
#include <disp/plots/helpers/colormap.h>

#include <QQuaternion>
#include <QDebug>
#include <cmath>

using namespace CONNECTIVITYLIB;
using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// PIMPL
//=============================================================================================================

struct NetworkObject::GpuBuffers
{
    // Node buffers
    std::unique_ptr<QRhiBuffer> nodeVertexBuffer;
    std::unique_ptr<QRhiBuffer> nodeIndexBuffer;
    std::unique_ptr<QRhiBuffer> nodeInstanceBuffer;
    // Edge buffers
    std::unique_ptr<QRhiBuffer> edgeVertexBuffer;
    std::unique_ptr<QRhiBuffer> edgeIndexBuffer;
    std::unique_ptr<QRhiBuffer> edgeInstanceBuffer;
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NetworkObject::NetworkObject()
    : m_gpu(std::make_unique<GpuBuffers>())
{
}

//=============================================================================================================

NetworkObject::~NetworkObject() = default;

//=============================================================================================================

QRhiBuffer* NetworkObject::nodeVertexBuffer()   const { return m_gpu->nodeVertexBuffer.get(); }
QRhiBuffer* NetworkObject::nodeIndexBuffer()    const { return m_gpu->nodeIndexBuffer.get(); }
QRhiBuffer* NetworkObject::nodeInstanceBuffer() const { return m_gpu->nodeInstanceBuffer.get(); }
QRhiBuffer* NetworkObject::edgeVertexBuffer()   const { return m_gpu->edgeVertexBuffer.get(); }
QRhiBuffer* NetworkObject::edgeIndexBuffer()    const { return m_gpu->edgeIndexBuffer.get(); }
QRhiBuffer* NetworkObject::edgeInstanceBuffer() const { return m_gpu->edgeInstanceBuffer.get(); }

//=============================================================================================================

void NetworkObject::load(const Network &network, const QString &sColormap)
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
    m_network.setThreshold(dThreshold);
    buildNodeInstances();
    buildEdgeInstances();
}

//=============================================================================================================

void NetworkObject::setColormap(const QString &sColormap)
{
    m_colormap = sColormap;
    buildNodeInstances();
    buildEdgeInstances();
}

//=============================================================================================================

void NetworkObject::createNodeGeometry()
{
    if (!m_nodeVertexData.isEmpty()) return;

    // Create an icosphere (subdivision level 1) for nodes
    const int subdivisions = 1;
    const float radius = 1.0f; // Unit sphere, scaled per-instance

    // Start with icosahedron
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    std::vector<QVector3D> vertices = {
        QVector3D(-1,  t,  0).normalized() * radius,
        QVector3D( 1,  t,  0).normalized() * radius,
        QVector3D(-1, -t,  0).normalized() * radius,
        QVector3D( 1, -t,  0).normalized() * radius,
        QVector3D( 0, -1,  t).normalized() * radius,
        QVector3D( 0,  1,  t).normalized() * radius,
        QVector3D( 0, -1, -t).normalized() * radius,
        QVector3D( 0,  1, -t).normalized() * radius,
        QVector3D( t,  0, -1).normalized() * radius,
        QVector3D( t,  0,  1).normalized() * radius,
        QVector3D(-t,  0, -1).normalized() * radius,
        QVector3D(-t,  0,  1).normalized() * radius,
    };

    std::vector<uint32_t> indices = {
        0,11,5,  0,5,1,   0,1,7,   0,7,10,  0,10,11,
        1,5,9,   5,11,4,  11,10,2, 10,7,6,  7,1,8,
        3,9,4,   3,4,2,   3,2,6,   3,6,8,   3,8,9,
        4,9,5,   2,4,11,  6,2,10,  8,6,7,   9,8,1,
    };

    // Subdivide
    for (int s = 0; s < subdivisions; ++s) {
        std::vector<uint32_t> newIndices;
        std::map<uint64_t, uint32_t> midpointCache;

        auto getMidpoint = [&](uint32_t i0, uint32_t i1) -> uint32_t {
            uint64_t key = (uint64_t)std::min(i0, i1) << 32 | std::max(i0, i1);
            auto it = midpointCache.find(key);
            if (it != midpointCache.end()) return it->second;

            QVector3D mid = ((vertices[i0] + vertices[i1]) / 2.0f).normalized() * radius;
            uint32_t idx = (uint32_t)vertices.size();
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
    for (const auto &v : vertices) {
        QVector3D n = v.normalized();
        vd.push_back({v.x(), v.y(), v.z(), n.x(), n.y(), n.z()});
    }

    m_nodeIndexCount = (int)indices.size();

    m_nodeVertexData.resize(vd.size() * sizeof(VertexData));
    memcpy(m_nodeVertexData.data(), vd.data(), m_nodeVertexData.size());

    m_nodeIndexData.resize(indices.size() * sizeof(uint32_t));
    memcpy(m_nodeIndexData.data(), indices.data(), m_nodeIndexData.size());

    m_nodeGeometryDirty = true;
}

//=============================================================================================================

void NetworkObject::createEdgeGeometry()
{
    if (!m_edgeVertexData.isEmpty()) return;

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
        float angle = 2.0f * (float)M_PI * i / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);

        QVector3D normal(x, 0, z);
        normal.normalize();

        // Top side vertex
        vertices.push_back({x, halfHeight, z, normal.x(), normal.y(), normal.z()});
        // Bottom side vertex
        vertices.push_back({x, -halfHeight, z, normal.x(), normal.y(), normal.z()});
    }

    // Top cap vertices (for proper normals)
    int topCapStart = (int)vertices.size();
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * (float)M_PI * i / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        vertices.push_back({x, halfHeight, z, 0, 1, 0});
    }

    // Bottom cap vertices
    int botCapStart = (int)vertices.size();
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * (float)M_PI * i / segments;
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

        indices.insert(indices.end(), {(uint32_t)topCur, (uint32_t)topNext, (uint32_t)botCur});
        indices.insert(indices.end(), {(uint32_t)botCur, (uint32_t)topNext, (uint32_t)botNext});
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

    m_edgeIndexCount = (int)indices.size();

    m_edgeVertexData.resize(vertices.size() * sizeof(VertexData));
    memcpy(m_edgeVertexData.data(), vertices.data(), m_edgeVertexData.size());

    m_edgeIndexData.resize(indices.size() * sizeof(uint32_t));
    memcpy(m_edgeIndexData.data(), indices.data(), m_edgeIndexData.size());

    m_edgeGeometryDirty = true;
}

//=============================================================================================================

void NetworkObject::buildNodeInstances()
{
    if (m_network.isEmpty()) {
        m_nodeInstanceCount = 0;
        m_nodeInstancesDirty = true;
        return;
    }

    const auto &nodes = m_network.getNodes();
    qint16 iMaxDegree = m_network.getMinMaxThresholdedDegrees().second;
    if (iMaxDegree == 0) iMaxDegree = 1;

    VisualizationInfo vizInfo = m_network.getVisualizationInfo();

    std::vector<InstanceData> instances;
    instances.reserve(nodes.size());

    for (int i = 0; i < nodes.size(); ++i) {
        qint16 degree = nodes[i]->getThresholdedDegree();
        if (degree == 0) continue;

        const RowVectorXf &vert = nodes[i]->getVert();
        QVector3D pos(vert(0), vert(1), vert(2));

        // Scale: nodes with higher degree are larger
        // Range: 0.0006 to 0.005 (same as disp3D)
        float scaleFactor = ((float)degree / (float)iMaxDegree) * (0.005f - 0.0006f) + 0.0006f;

        QMatrix4x4 m;
        m.translate(pos);
        m.scale(scaleFactor);

        InstanceData inst;
        const float *mPtr = m.constData();
        for (int j = 0; j < 16; ++j) inst.model[j] = mPtr[j];

        // Color: colormap-based or fixed
        if (vizInfo.sMethod == "Map") {
            float normalized = (float)degree / (float)iMaxDegree;
            QRgb rgb = ColorMap::valueToColor(normalized, vizInfo.sColormap.isEmpty() ? m_colormap : vizInfo.sColormap);
            QColor color(rgb);
            float alpha = std::pow(normalized, 4.0f); // Same as disp3D
            inst.color[0] = color.redF();
            inst.color[1] = color.greenF();
            inst.color[2] = color.blueF();
            inst.color[3] = alpha;
        } else {
            inst.color[0] = vizInfo.colNodes[0] / 255.0f;
            inst.color[1] = vizInfo.colNodes[1] / 255.0f;
            inst.color[2] = vizInfo.colNodes[2] / 255.0f;
            inst.color[3] = vizInfo.colNodes[3] / 255.0f;
        }
        inst.isSelected = 0.0f;

        instances.push_back(inst);
    }

    m_nodeInstanceCount = (int)instances.size();
    m_nodeInstanceData.resize(m_nodeInstanceCount * sizeof(InstanceData));
    if (m_nodeInstanceCount > 0) {
        memcpy(m_nodeInstanceData.data(), instances.data(), m_nodeInstanceData.size());
    }
    m_nodeInstancesDirty = true;

    qDebug() << "NetworkObject: Built" << m_nodeInstanceCount << "node instances";
}

//=============================================================================================================

void NetworkObject::buildEdgeInstances()
{
    if (m_network.isEmpty()) {
        m_edgeInstanceCount = 0;
        m_edgeInstancesDirty = true;
        return;
    }

    const auto &edges = m_network.getThresholdedEdges();
    const auto &nodes = m_network.getNodes();

    double dMaxWeight = m_network.getMinMaxThresholdedWeights().second;
    double dMinWeight = m_network.getMinMaxThresholdedWeights().first;
    double dWeightRange = dMaxWeight - dMinWeight;
    if (dWeightRange == 0.0) dWeightRange = 1.0;

    VisualizationInfo vizInfo = m_network.getVisualizationInfo();

    std::vector<InstanceData> instances;
    instances.reserve(edges.size());

    for (int i = 0; i < edges.size(); ++i) {
        auto &edge = edges[i];
        if (!edge->isActive()) continue;

        int iStart = edge->getStartNodeID();
        int iEnd = edge->getEndNodeID();

        if (iStart < 0 || iStart >= nodes.size() || iEnd < 0 || iEnd >= nodes.size()) continue;

        const RowVectorXf &vStart = nodes[iStart]->getVert();
        const RowVectorXf &vEnd = nodes[iEnd]->getVert();

        QVector3D startPos(vStart(0), vStart(1), vStart(2));
        QVector3D endPos(vEnd(0), vEnd(1), vEnd(2));

        if (startPos == endPos) continue;

        double dWeight = std::fabs(edge->getWeight());
        if (dWeight == 0.0) continue;

        QVector3D diff = endPos - startPos;
        QVector3D midPoint = startPos + diff / 2.0f;
        float length = diff.length();

        // Build transform: translate to midpoint, rotate Y-axis to diff direction, scale
        float normalizedWeight = (float)std::fabs((dWeight - dMinWeight) / dWeightRange);

        // Cylinder radius: proportional to weight, range 0.0001 to 0.001
        float edgeRadius = 0.0001f + normalizedWeight * 0.0009f;

        QMatrix4x4 m;
        m.translate(midPoint);
        m.rotate(QQuaternion::rotationTo(QVector3D(0, 1, 0), diff.normalized()));
        m.scale(edgeRadius, length, edgeRadius);

        InstanceData inst;
        const float *mPtr = m.constData();
        for (int j = 0; j < 16; ++j) inst.model[j] = mPtr[j];

        // Color
        if (vizInfo.sMethod == "Map") {
            float normalized = (dMaxWeight != 0.0) ? (float)std::fabs(dWeight / dMaxWeight) : 0.0f;
            QRgb rgb = ColorMap::valueToColor(normalized, vizInfo.sColormap.isEmpty() ? m_colormap : vizInfo.sColormap);
            QColor color(rgb);
            float alpha = std::pow(normalized, 1.5f); // Same as disp3D
            inst.color[0] = color.redF();
            inst.color[1] = color.greenF();
            inst.color[2] = color.blueF();
            inst.color[3] = alpha;
        } else {
            inst.color[0] = vizInfo.colEdges[0] / 255.0f;
            inst.color[1] = vizInfo.colEdges[1] / 255.0f;
            inst.color[2] = vizInfo.colEdges[2] / 255.0f;
            inst.color[3] = vizInfo.colEdges[3] / 255.0f;
        }
        inst.isSelected = 0.0f;

        instances.push_back(inst);
    }

    m_edgeInstanceCount = (int)instances.size();
    m_edgeInstanceData.resize(m_edgeInstanceCount * sizeof(InstanceData));
    if (m_edgeInstanceCount > 0) {
        memcpy(m_edgeInstanceData.data(), instances.data(), m_edgeInstanceData.size());
    }
    m_edgeInstancesDirty = true;

    qDebug() << "NetworkObject: Built" << m_edgeInstanceCount << "edge instances";
}

//=============================================================================================================

void NetworkObject::updateNodeBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    if (m_nodeGeometryDirty) {
        if (!m_gpu->nodeVertexBuffer) {
            m_gpu->nodeVertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, m_nodeVertexData.size()));
            m_gpu->nodeVertexBuffer->create();
        }
        if (!m_gpu->nodeIndexBuffer) {
            m_gpu->nodeIndexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, m_nodeIndexData.size()));
            m_gpu->nodeIndexBuffer->create();
        }
        u->uploadStaticBuffer(m_gpu->nodeVertexBuffer.get(), m_nodeVertexData.constData());
        u->uploadStaticBuffer(m_gpu->nodeIndexBuffer.get(), m_nodeIndexData.constData());
        m_nodeGeometryDirty = false;
    }

    if (m_nodeInstancesDirty && m_nodeInstanceCount > 0) {
        int requiredSize = m_nodeInstanceData.size();
        if (!m_gpu->nodeInstanceBuffer || m_gpu->nodeInstanceBuffer->size() < requiredSize) {
            m_gpu->nodeInstanceBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, requiredSize));
            m_gpu->nodeInstanceBuffer->create();
        }
        u->updateDynamicBuffer(m_gpu->nodeInstanceBuffer.get(), 0, requiredSize, m_nodeInstanceData.constData());
        m_nodeInstancesDirty = false;
    }
}

//=============================================================================================================

void NetworkObject::updateEdgeBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    if (m_edgeGeometryDirty) {
        if (!m_gpu->edgeVertexBuffer) {
            m_gpu->edgeVertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, m_edgeVertexData.size()));
            m_gpu->edgeVertexBuffer->create();
        }
        if (!m_gpu->edgeIndexBuffer) {
            m_gpu->edgeIndexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, m_edgeIndexData.size()));
            m_gpu->edgeIndexBuffer->create();
        }
        u->uploadStaticBuffer(m_gpu->edgeVertexBuffer.get(), m_edgeVertexData.constData());
        u->uploadStaticBuffer(m_gpu->edgeIndexBuffer.get(), m_edgeIndexData.constData());
        m_edgeGeometryDirty = false;
    }

    if (m_edgeInstancesDirty && m_edgeInstanceCount > 0) {
        int requiredSize = m_edgeInstanceData.size();
        if (!m_gpu->edgeInstanceBuffer || m_gpu->edgeInstanceBuffer->size() < requiredSize) {
            m_gpu->edgeInstanceBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, requiredSize));
            m_gpu->edgeInstanceBuffer->create();
        }
        u->updateDynamicBuffer(m_gpu->edgeInstanceBuffer.get(), 0, requiredSize, m_edgeInstanceData.constData());
        m_edgeInstancesDirty = false;
    }
}
