//=============================================================================================================
/**
 * @file     brainsurface.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January, 2026
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
 * @brief    BrainSurface class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainsurface.h"
#include <QDebug>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================
#include <QDebug>

//=============================================================================================================

BrainSurface::BrainSurface()
{
}

//=============================================================================================================

BrainSurface::~BrainSurface()
{
}

//=============================================================================================================

void BrainSurface::fromSurface(const FSLIB::Surface &surf)
{
    m_vertexData.clear();
    m_indexData.clear();

    const Eigen::MatrixXf &rr = surf.rr();
    const Eigen::MatrixXf &nn = surf.nn();
    const Eigen::MatrixXi &tris = surf.tris();
    const Eigen::VectorXf &curv = surf.curv();
    m_curvature.resize(curv.size());
    for(int i=0; i<curv.size(); ++i) m_curvature[i] = curv[i];

    // Populate vertex data
    m_vertexData.reserve(rr.rows());
    
    for (int i = 0; i < rr.rows(); ++i) {
        VertexData v;
        v.pos = QVector3D(rr(i, 0), rr(i, 1), rr(i, 2));
        v.norm = QVector3D(nn(i, 0), nn(i, 1), nn(i, 2));
        v.color = 0xFFFFFFFF; // Default white
        m_vertexData.append(v);
    }
    
    // Default to white (Surface mode)
    // If we have curvature, we could use it, but start with standard.

    m_indexData.reserve(tris.rows() * 3);
    for (int i = 0; i < tris.rows(); ++i) {
        m_indexData.append(tris(i, 0));
        m_indexData.append(tris(i, 1));
        m_indexData.append(tris(i, 2));
    }
    m_indexCount = m_indexData.size();
    
    m_bBuffersDirty = true;
    
    // Initial coloring based on current visualization mode
    updateVertexColors();
}

//=============================================================================================================

bool BrainSurface::loadAnnotation(const QString &path)
{
    if (!FSLIB::Annotation::read(path, m_annotation)) {
        qWarning() << "BrainSurface: Failed to load annotation from" << path;
        return false;
    }
    m_hasAnnotation = true;
    updateVertexColors();
    m_bBuffersDirty = true;
    return true;
}


//=============================================================================================================

void BrainSurface::setVisible(bool visible)
{
    m_visible = visible;
}

//=============================================================================================================

void BrainSurface::setVisualizationMode(VisualizationMode mode)
{
    if (m_visMode != mode) {
        m_visMode = mode;
        updateVertexColors();
        m_bBuffersDirty = true;
    }
}

//=============================================================================================================

void BrainSurface::applySourceEstimateColors(const QVector<uint32_t> &colors)
{
    // Set mode to source estimate
    m_visMode = ModeSourceEstimate;
    
    // Apply colors to vertices
    for (int i = 0; i < qMin(colors.size(), m_vertexData.size()); ++i) {
        m_vertexData[i].color = colors[i];
    }
    
    m_bBuffersDirty = true;
}

//=============================================================================================================

void BrainSurface::updateVertexColors()
{
    // Reset to white first
    for (auto &v : m_vertexData) {
        v.color = 0xFFFFFFFF; 
    }

    if (m_visMode == ModeAnnotation) {
        if (!m_hasAnnotation || m_vertexData.isEmpty()) return;

        const Eigen::VectorXi &vertices = m_annotation.getVertices();
        const Eigen::VectorXi &labelIds = m_annotation.getLabelIds();
        const FSLIB::Colortable &ct = m_annotation.getColortable();

        for (int i = 0; i < labelIds.rows(); ++i) {
            int vertexIdx = vertices(i);
            
            if (vertexIdx >= 0 && vertexIdx < m_vertexData.size()) {
                int colorIdx = -1;
                // The labelId in .annot is often (R + G*2^8 + B*2^16)
                for(int c=0; c < ct.numEntries; ++c) {
                    if (ct.table(c, 4) == labelIds(i)) {
                        colorIdx = c;
                        break;
                    }
                }
                
                if (colorIdx >= 0) {
                    QColor c(ct.table(colorIdx, 0), ct.table(colorIdx, 1), ct.table(colorIdx, 2), 255);
                    // QColor to uint32_t ABGR/RGBA (little endian)
                    // For RHI (Metal/Vulkan), typically ABGR (0xAABBGGRR)
                    // Qt's toRgb().rgb() gives 0xAARRGGBB, we might need to swizzle in shader or here.
                    // Assuming typical little endian reading:
                    uint32_t r = c.red();
                    uint32_t g = c.green();
                    uint32_t b = c.blue();
                    // Let's pack as AABBGGRR (0xFF, B, G, R)
                    m_vertexData[vertexIdx].color = (255 << 24) | (b << 16) | (g << 8) | r;
                }
            }
        }
    }
    else if (m_visMode == ModeScientific) {
        // Curvature > 0: Light Gray (Gyri)
        // Curvature <= 0: Dark Gray (Sulci)
        for (int i = 0; i < m_vertexData.size() && i < m_curvature.size(); ++i) {
             // 0x66 = 102 (Dark), 0x99 = 153 (Light) or go higher contrast
             // Dark Gray: 0xFF202020
             // Standard Gray: 0xFF808080 or White?
             // FreeSurfer style: Sulci (concave, < 0) are Dark, Gyri (convex, > 0) are Light.
             
             uint32_t val;
             if (m_curvature[i] > 0) {
                 // Sulcus (Concave) -> Dark
                 val = 0x40; // Dark Gray
             } else {
                 // Gyrus (Convex) -> Light
                 val = 0xAA; // Light Gray
             }
             // Store as grayscale
             m_vertexData[i].color = (255 << 24) | (val << 16) | (val << 8) | val;
        }
    }
}

float BrainSurface::minX() const
{
    float minVal = std::numeric_limits<float>::max();
    for (const auto &v : m_vertexData) {
        if (v.pos.x() < minVal) minVal = v.pos.x();
    }
    return minVal;
}

float BrainSurface::maxX() const
{
    float maxVal = std::numeric_limits<float>::lowest();
    for (const auto &v : m_vertexData) {
        if (v.pos.x() > maxVal) maxVal = v.pos.x();
    }
    return maxVal;
}

void BrainSurface::translateX(float offset)
{
    for (auto &v : m_vertexData) {
        v.pos.setX(v.pos.x() + offset);
    }
    m_bBuffersDirty = true;
}

//=============================================================================================================

void BrainSurface::updateBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    if (!m_bBuffersDirty && m_vertexBuffer && m_indexBuffer) return;

    if (!m_vertexBuffer) {
        m_vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, m_vertexData.size() * sizeof(VertexData)));
        m_vertexBuffer->create();
    }
    if (!m_indexBuffer) {
        m_indexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, m_indexData.size() * sizeof(uint32_t)));
        m_indexBuffer->create();
    }

    u->uploadStaticBuffer(m_vertexBuffer.get(), m_vertexData.constData());
    u->uploadStaticBuffer(m_indexBuffer.get(), m_indexData.constData());
    m_bBuffersDirty = false;
}

//=============================================================================================================

QVector<QVector<int>> BrainSurface::computeNeighbors() const
{
    QVector<QVector<int>> neighbors(m_vertexData.size());
    
    // Triangles are stored in m_indexData as triplets
    for (int i = 0; i + 2 < m_indexData.size(); i += 3) {
        int v0 = m_indexData[i];
        int v1 = m_indexData[i + 1];
        int v2 = m_indexData[i + 2];
        
        // Add bidirectional edges
        if (!neighbors[v0].contains(v1)) neighbors[v0].append(v1);
        if (!neighbors[v0].contains(v2)) neighbors[v0].append(v2);
        if (!neighbors[v1].contains(v0)) neighbors[v1].append(v0);
        if (!neighbors[v1].contains(v2)) neighbors[v1].append(v2);
        if (!neighbors[v2].contains(v0)) neighbors[v2].append(v0);
        if (!neighbors[v2].contains(v1)) neighbors[v2].append(v1);
    }
    
    return neighbors;
}

//=============================================================================================================

Eigen::MatrixX3f BrainSurface::verticesAsMatrix() const
{
    Eigen::MatrixX3f mat(m_vertexData.size(), 3);
    for (int i = 0; i < m_vertexData.size(); ++i) {
        mat(i, 0) = m_vertexData[i].pos.x();
        mat(i, 1) = m_vertexData[i].pos.y();
        mat(i, 2) = m_vertexData[i].pos.z();
    }
    return mat;
}

