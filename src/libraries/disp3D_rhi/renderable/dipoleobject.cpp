//=============================================================================================================
/**
 * @file     dipoleobject.cpp
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
 * @brief    DipoleObject class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipoleobject.h"

#include <rhi/qrhi.h>
#include <cmath>
#include <QQuaternion>
#include <QRandomGenerator>
#include <QDebug>

//=============================================================================================================
// PIMPL
//=============================================================================================================

struct DipoleObject::GpuBuffers
{
    std::unique_ptr<QRhiBuffer> vertexBuffer;
    std::unique_ptr<QRhiBuffer> indexBuffer;
    std::unique_ptr<QRhiBuffer> instanceBuffer;
};

DipoleObject::DipoleObject()
    : m_gpu(std::make_unique<GpuBuffers>())
{
}

DipoleObject::~DipoleObject() = default;

QRhiBuffer* DipoleObject::vertexBuffer()  const { return m_gpu->vertexBuffer.get(); }
QRhiBuffer* DipoleObject::indexBuffer()   const { return m_gpu->indexBuffer.get(); }
QRhiBuffer* DipoleObject::instanceBuffer() const { return m_gpu->instanceBuffer.get(); }

void DipoleObject::load(const INVERSELIB::ECDSet &ecdSet)
{
    createGeometry();
    
    m_instanceCount = ecdSet.size();
    m_instanceData.resize(m_instanceCount * sizeof(InstanceData));
    InstanceData *data = reinterpret_cast<InstanceData*>(m_instanceData.data());
    
    QVector3D from(0.0f, 1.0f, 0.0f); // Cone points up Y axis
    
    if (ecdSet.size() > 0) {
        qDebug() << "DipoleObject: First dipole raw pos:" << ecdSet[0].rd(0) << ecdSet[0].rd(1) << ecdSet[0].rd(2);
    }

    // Heuristic: Check if coordinates are likely in mm (e.g., > 1.0 or similar)
    // Head model is usually < 0.2m radius. If values are > 0.5, they are likely mm or cm.
    // sample data is ~44.56 mm.
    float unitScale = 1.0f;
    float maxCoord = 0.0f;
    float maxMag = 0.0f;
    
    for(int i=0; i < ecdSet.size(); ++i) {
        maxCoord = std::max(maxCoord, std::abs(ecdSet[i].rd(0)));
        maxCoord = std::max(maxCoord, std::abs(ecdSet[i].rd(1)));
        maxCoord = std::max(maxCoord, std::abs(ecdSet[i].rd(2)));
        
        float mag = std::sqrt(std::pow(ecdSet[i].Q(0), 2) + std::pow(ecdSet[i].Q(1), 2) + std::pow(ecdSet[i].Q(2), 2));
        maxMag = std::max(maxMag, mag);
    }
    
    if (maxCoord > 5.0f) {
        unitScale = 0.001f; // Convert mm to meters
        qDebug() << "DipoleObject: Detected large coordinates (max" << maxCoord << "), applying mm->m scale (0.001).";
    }

    qDebug() << "DipoleObject: Loading" << m_instanceCount << "dipoles with scale" << unitScale;
    
    for (int i = 0; i < ecdSet.size(); ++i) {
        const auto &dip = ecdSet[i];
        
        QVector3D pos(dip.rd(0) * unitScale, dip.rd(1) * unitScale, dip.rd(2) * unitScale);
        QVector3D Q(dip.Q(0), dip.Q(1), dip.Q(2));
        float mag = Q.length();
        
        QVector3D to = Q.normalized();
        // Handle case where 'to' is parallel to 'from'
        QQuaternion rot;
        if (QVector3D::dotProduct(from, to) > 0.99f) {
            rot = QQuaternion();
        } else if (QVector3D::dotProduct(from, to) < -0.99f) {
            rot = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 180.0f);
        } else {
            rot = QQuaternion::rotationTo(from, to);
        }
        
        // Scale based on magnitude relative to max, with a minimum size of 20%
        float scaleFactor = (maxMag > 0.0f) ? (0.2f + 0.8f * (mag / maxMag)) : 1.0f;
        
        QMatrix4x4 m;
        m.translate(pos);
        m.rotate(rot);
        m.scale(scaleFactor);
        
        const float *mPtr = m.constData();
        for (int j = 0; j < 16; ++j) {
            data[i].model[j] = mPtr[j];
        }
        
        // Random Color
        data[i].color[0] = QRandomGenerator::global()->generateDouble();
        data[i].color[1] = QRandomGenerator::global()->generateDouble();
        data[i].color[2] = QRandomGenerator::global()->generateDouble();
        data[i].color[3] = 1.0f;
        
        // Selection state
        data[i].isSelected = 0.0f;
    }
    
    if (m_instanceCount > 0) {
         InstanceData *data = reinterpret_cast<InstanceData*>(m_instanceData.data());
         float x = data[0].model[12];
         float y = data[0].model[13];
         float z = data[0].model[14];
         qDebug() << "DipoleObject: First dipole initial pos (Scaled):" << x << y << z;
    }
    
    m_instancesDirty = true;
}

void DipoleObject::applyTransform(const QMatrix4x4 &trans)
{
    if (m_instanceCount == 0) return;
    
    InstanceData *data = reinterpret_cast<InstanceData*>(m_instanceData.data());
    
    for (int i = 0; i < m_instanceCount; ++i) {
        // Reconstruct current model matrix
        QMatrix4x4 currentModel;
        const float *src = data[i].model;
        float *dst = currentModel.data();
        for(int j=0; j<16; ++j) dst[j] = src[j];
        
        // Apply transformation: NewModel = Trans * OldModel
        // Wait, OldModel places the cone at Pos with Rot.
        // We want to transform Pos and Rot.
        // So yes, Trans * OldModel works.
        
        QMatrix4x4 newModel = trans * currentModel;
        
        // Store back
        const float *newPtr = newModel.constData();
        for (int j = 0; j < 16; ++j) {
            data[i].model[j] = newPtr[j];
        }
    }
    
    if (m_instanceCount > 0) {
        // Log first instance pos
        InstanceData *data = reinterpret_cast<InstanceData*>(m_instanceData.data());
        // Matrix is column major? No, we stored it as we got it from QMatrix4x4.constData(), which is Column-Major.
        // Translation is in the last column (indices 12, 13, 14).
        float x = data[0].model[12];
        float y = data[0].model[13];
        float z = data[0].model[14];
        qDebug() << "DipoleObject: First dipole transformed pos (Meters):" << x << y << z;
    }
    
    m_instancesDirty = true;
}

QVector3D DipoleObject::debugFirstDipolePosition() const
{
    if (m_instanceCount == 0) return QVector3D();
    const InstanceData *data = reinterpret_cast<const InstanceData*>(m_instanceData.constData());
    // Column 3 is translation (12, 13, 14)
    return QVector3D(data[0].model[12], data[0].model[13], data[0].model[14]);
}

void DipoleObject::createGeometry()
{
    if (!m_vertexData.isEmpty()) return;
    
    // Create a simple cone
    // Radius 0.001, Height 0.003
    // 32 segments
    
    float radius = 0.005f; // Slightly larger for visibility
    float height = 0.01f;
    int segments = 16;
    
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    
    // Tip
    VertexData tip = {0, height, 0, 0, 1, 0};
    vertices.push_back(tip);
    
    // Base center
    VertexData baseCenter = {0, 0, 0, 0, -1, 0};
    vertices.push_back(baseCenter); // Index 1
    
    // Rim vertices
    int centerIdx = 1;
    int firstRimIdx = 2;
    
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        
        // Side normal
        // Slope vector: (cos, -h/r, sin) -> normalized
        QVector3D n(x, radius/height, z);
        n.normalize();
        
        VertexData vSide = {x, 0, z, n.x(), n.y(), n.z()};
        vertices.push_back(vSide);
        
        VertexData vBase = {x, 0, z, 0, -1, 0};
        vertices.push_back(vBase);
    }
    
    // Indices
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        
        // Cone sides
        // Tip (0), current(i), next(next)
        // Side vertices start at firstRimIdx.
        // Layout: Tip, BaseCenter, Side0, Base0, Side1, Base1...
        // Wait, easier to keep separate lists or just flat
        
        // Re-do layout for ease: 
        // 0: Tip
        // 1: Base Center
        // 2..2+segments-1: Side rim vertices
        // 2+segments..2+2*segments-1: Base rim vertices
    }
    
    vertices.clear();
    vertices.push_back(tip); // 0
    vertices.push_back(baseCenter); // 1
    
    for (int i=0; i<segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        
        QVector3D n(x, 0, z); // Approximate normal for side (flat shading effectively if we don't slant)
        // Better normal: vector perpendicular to slope
        QVector3D slope(x, -height, z);
        QVector3D tangent(-sin(angle), 0, cos(angle));
        QVector3D sideNormal = QVector3D::crossProduct(tangent, slope).normalized(); // Wait, slope is vector down side.
        // Actually simple: normal y component is radius/height ratio related.
        
        vertices.push_back({x, 0, z, sideNormal.x(), sideNormal.y(), sideNormal.z()}); // Side rim
    }
    
     for (int i=0; i<segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        vertices.push_back({x, 0, z, 0, -1, 0}); // Base rim
    }
    
    int sideStart = 2;
    int baseStart = 2 + segments;
    
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        
        // Cone face
        indices.push_back(0);
        indices.push_back(sideStart + next);
        indices.push_back(sideStart + i);
        
        // Base face
        indices.push_back(1);
        indices.push_back(baseStart + i);
        indices.push_back(baseStart + next);
    }
    
    m_indexCount = indices.size();
    
    m_vertexData.resize(vertices.size() * sizeof(VertexData));
    memcpy(m_vertexData.data(), vertices.data(), m_vertexData.size());
    
    m_indexData.resize(indices.size() * sizeof(uint32_t));
    memcpy(m_indexData.data(), indices.data(), m_indexData.size());
    
    m_geometryDirty = true;
}

void DipoleObject::updateBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    if (m_geometryDirty) {
        if (!m_gpu->vertexBuffer) {
            m_gpu->vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, m_vertexData.size()));
            m_gpu->vertexBuffer->create();
            qDebug() << "DipoleObject: Created vertex buffer size" << m_vertexData.size();
        }
        if (!m_gpu->indexBuffer) {
            m_gpu->indexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, m_indexData.size()));
            m_gpu->indexBuffer->create();
            qDebug() << "DipoleObject: Created index buffer size" << m_indexData.size();
        }
        u->uploadStaticBuffer(m_gpu->vertexBuffer.get(), m_vertexData.constData());
        u->uploadStaticBuffer(m_gpu->indexBuffer.get(), m_indexData.constData());
        m_geometryDirty = false;
    }
    
    if (m_instancesDirty && m_instanceCount > 0) {
        if (!m_gpu->instanceBuffer || m_gpu->instanceBuffer->size() < m_instanceData.size()) {
            m_gpu->instanceBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, m_instanceData.size()));
            m_gpu->instanceBuffer->create();
            qDebug() << "DipoleObject: Created instance buffer size" << m_instanceData.size();
        }
        u->updateDynamicBuffer(m_gpu->instanceBuffer.get(), 0, m_instanceData.size(), m_instanceData.constData());
        m_instancesDirty = false;
    }
}

int DipoleObject::intersect(const QVector3D &rayOrigin, const QVector3D &rayDir, float &dist) const
{
    if (m_instanceCount == 0) return -1;
    
    int closestIdx = -1;
    float closestDist = std::numeric_limits<float>::max();
    
    const InstanceData *data = reinterpret_cast<const InstanceData*>(m_instanceData.constData());
    
    // Geometry radius ~ 0.005 (base) to 0.01 (height). 
    // Let's use a slightly larger hit radius to make selection easier and more accurate.
    const float baseRadius = 0.02f; 

    for (int i = 0; i < m_instanceCount; ++i) {
        // Extract translation (last column)
        QVector3D center(data[i].model[12], data[i].model[13], data[i].model[14]);
        
        // Extract scale (length of first column) - assumes uniform scale roughly
        QVector3D col0(data[i].model[0], data[i].model[1], data[i].model[2]);
        float scale = col0.length();
        
        float radius = baseRadius * scale;
        
        // Ray-Sphere Intersection
        QVector3D L = center - rayOrigin;
        float tca = QVector3D::dotProduct(L, rayDir);
        
        if (tca < 0) continue; // Behind ray
        
        float d2 = QVector3D::dotProduct(L, L) - tca * tca;
        if (d2 > radius * radius) continue; // Miss
        
        float thc = std::sqrt(radius * radius - d2);
        float t0 = tca - thc;
        float t1 = tca + thc;
        
        float t = t0;
        if (t < 0) t = t1;
        if (t < 0) continue;
        
        if (t < closestDist) {
            closestDist = t;
            closestIdx = i;
        }
    }
    
    if (closestIdx != -1) {
        dist = closestDist;
        return closestIdx;
    }
    
    return -1;
}

void DipoleObject::setSelected(int index, bool selected)
{
    if (index < 0 || index >= m_instanceCount) return;
    
    InstanceData *data = reinterpret_cast<InstanceData*>(m_instanceData.data());
    
    data[index].isSelected = selected ? 1.0f : 0.0f;
    
    m_instancesDirty = true;
}
