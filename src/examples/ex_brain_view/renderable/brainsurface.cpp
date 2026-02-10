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


//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================


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
    m_bAABBDirty = true;
    
    // Initial coloring based on current visualization mode
    updateVertexColors();

    m_originalVertexData = m_vertexData;
}

//=============================================================================================================

//=============================================================================================================

void BrainSurface::fromBemSurface(const MNELIB::MNEBemSurface &surf, const QColor &color)
{
    m_vertexData.clear();
    m_indexData.clear();
    m_curvature.clear(); // BEM has no curvature info usually

    int nVerts = surf.rr.rows();
    m_vertexData.reserve(nVerts);
    
    // Compute normals if missing
    Eigen::MatrixX3f nn = surf.nn;
    if (nn.rows() != nVerts) {
        nn = FSLIB::Surface::compute_normals(surf.rr, surf.tris);
    }
    
    m_defaultColor = color;
    m_baseColor = color;
    uint32_t colorVal = (color.alpha() << 24) | (color.blue() << 16) | (color.green() << 8) | color.red();

    for (int i = 0; i < nVerts; ++i) {
        VertexData v;
        v.pos = QVector3D(surf.rr(i, 0), surf.rr(i, 1), surf.rr(i, 2));
        v.norm = QVector3D(nn(i, 0), nn(i, 1), nn(i, 2));
        v.color = colorVal;
        m_vertexData.append(v);
    }

    int nTris = surf.tris.rows();
    m_indexData.reserve(nTris * 3);
    for (int i = 0; i < nTris; ++i) {
        m_indexData.append(surf.tris(i, 0));
        m_indexData.append(surf.tris(i, 1));
        m_indexData.append(surf.tris(i, 2));
    }
    m_indexCount = m_indexData.size();
    
    m_originalVertexData = m_vertexData;
    m_bBuffersDirty = true;
}

void BrainSurface::createFromData(const Eigen::MatrixX3f &vertices, const Eigen::MatrixX3i &triangles, const QColor &color)
{
    // Compute spherical normals (legacy behavior / fallback)
    // Note: detailed normals should be passed via the overload for specific shapes
    Eigen::MatrixX3f normals(vertices.rows(), 3);
    for(int i=0; i<vertices.rows(); ++i) {
        QVector3D p(vertices(i, 0), vertices(i, 1), vertices(i, 2));
        QVector3D n = p.normalized();
        normals(i, 0) = n.x();
        normals(i, 1) = n.y();
        normals(i, 2) = n.z();
    }
    createFromData(vertices, normals, triangles, color);
}

void BrainSurface::createFromData(const Eigen::MatrixX3f &vertices, const Eigen::MatrixX3f &normals, const Eigen::MatrixX3i &triangles, const QColor &color)
{
    m_vertexData.clear();
    m_indexData.clear();
    m_curvature.clear(); 

    int nVerts = vertices.rows();
    m_vertexData.reserve(nVerts);
    
    m_defaultColor = color;
    m_baseColor = color;
    uint32_t colorVal = (color.alpha() << 24) | (color.blue() << 16) | (color.green() << 8) | color.red();

    for (int i = 0; i < nVerts; ++i) {
        VertexData v;
        v.pos = QVector3D(vertices(i, 0), vertices(i, 1), vertices(i, 2));
        v.norm = QVector3D(normals(i, 0), normals(i, 1), normals(i, 2));
        v.color = colorVal;
        m_vertexData.append(v);
    }

    int nTris = triangles.rows();
    m_indexData.reserve(nTris * 3);
    for (int i = 0; i < nTris; ++i) {
        m_indexData.append(triangles(i, 0));
        m_indexData.append(triangles(i, 1));
        m_indexData.append(triangles(i, 2));
    }
    m_indexCount = m_indexData.size();
    
    m_originalVertexData = m_vertexData;
    m_bBuffersDirty = true;
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

void BrainSurface::addAnnotation(const FSLIB::Annotation &annotation)
{
    m_annotation = annotation;
    m_hasAnnotation = true;
    updateVertexColors();
    m_bBuffersDirty = true;
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
    m_stcColors = colors;
    
    // Apply colors to vertices
    for (int i = 0; i < qMin(colors.size(), m_vertexData.size()); ++i) {
        m_vertexData[i].color = colors[i];
    }
    
    m_bBuffersDirty = true;
}

//=============================================================================================================

void BrainSurface::setUseDefaultColor(bool useDefault)
{
    m_baseColor = useDefault ? m_defaultColor : Qt::white;
    updateVertexColors();
    m_bBuffersDirty = true;
}

void BrainSurface::updateVertexColors()
{
    uint32_t baseVal = (m_baseColor.alpha() << 24) | (m_baseColor.blue() << 16) | (m_baseColor.green() << 8) | m_baseColor.red();


    // Reset to base color first, but apply curvature shading if available
    // This provides sulci/gyri visibility even in Surface mode
    if (!m_curvature.isEmpty() && m_curvature.size() == m_vertexData.size()) {
        // Use curvature-based grayscale for depth perception
        for (int i = 0; i < m_vertexData.size(); ++i) {
            uint32_t val;
            if (m_curvature[i] > 0) {
                // Sulcus (Concave) -> Medium-Dark gray
                val = 0x90; // Medium gray (was 0x40 in Scientific mode)
            } else {
                // Gyrus (Convex) -> Light gray/white
                val = 0xE8; // Light gray (was 0xAA in Scientific mode)
            }
            m_vertexData[i].color = (255 << 24) | (val << 16) | (val << 8) | val;
        }
    } else {
        // Fallback: no curvature data, use base color
        for (auto &v : m_vertexData) {
            v.color = baseVal; 
        }
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
    else if (m_visMode == ModeSourceEstimate) {
        for (int i = 0; i < m_vertexData.size() && i < m_stcColors.size(); ++i) {
            m_vertexData[i].color = m_stcColors[i];
        }
    }

    // Apply selection highlight (golden tint blend for visibility)
    if (m_selected || m_selectedRegionId != -1) {
        // Gold highlight color for visibility on any base
        const uint32_t goldR = 255, goldG = 200, goldB = 80;
        const float blendFactor = 0.4f; // 40% gold blend
        
        auto blendToGold = [&](uint32_t &c) {
            uint32_t a = (c >> 24) & 0xFF;
            uint32_t b = (c >> 16) & 0xFF;
            uint32_t g = (c >> 8) & 0xFF;
            uint32_t r = c & 0xFF;
            
            // Blend towards gold
            r = static_cast<uint32_t>(r * (1.0f - blendFactor) + goldR * blendFactor);
            g = static_cast<uint32_t>(g * (1.0f - blendFactor) + goldG * blendFactor);
            b = static_cast<uint32_t>(b * (1.0f - blendFactor) + goldB * blendFactor);
            
            c = (a << 24) | (b << 16) | (g << 8) | r;
        };
        
        if (m_hasAnnotation && m_selectedRegionId != -1) {
            // Highlight only the selected region
            const Eigen::VectorXi &vertices = m_annotation.getVertices();
            const Eigen::VectorXi &labelIds = m_annotation.getLabelIds();
            
            for (int i = 0; i < labelIds.rows(); ++i) {
                if (labelIds(i) == m_selectedRegionId) {
                    int vertexIdx = vertices(i);
                    if (vertexIdx >= 0 && vertexIdx < m_vertexData.size()) {
                        blendToGold(m_vertexData[vertexIdx].color);
                    }
                }
            }
        } else if (m_selected) {
            // Highlight whole surface if no specific region or vertex range selected
            for (auto &v : m_vertexData) {
                blendToGold(v.color);
            }
        }
    }

    // Apply vertex-range highlight (for individual points in batched meshes)
    // Use bright white-yellow with high blend factor for strong visibility on small spheres
    if (m_selectedVertexStart >= 0 && m_selectedVertexCount > 0) {
        const uint32_t hlR = 255, hlG = 255, hlB = 180;
        const float blendFactor = 0.85f;
        int end = qMin(m_selectedVertexStart + m_selectedVertexCount, (int)m_vertexData.size());
        for (int i = m_selectedVertexStart; i < end; ++i) {
            uint32_t &c = m_vertexData[i].color;
            uint32_t a = (c >> 24) & 0xFF;
            uint32_t b = (c >> 16) & 0xFF;
            uint32_t g = (c >> 8) & 0xFF;
            uint32_t r = c & 0xFF;
            r = static_cast<uint32_t>(r * (1.0f - blendFactor) + hlR * blendFactor);
            g = static_cast<uint32_t>(g * (1.0f - blendFactor) + hlG * blendFactor);
            b = static_cast<uint32_t>(b * (1.0f - blendFactor) + hlB * blendFactor);
            c = (a << 24) | (b << 16) | (g << 8) | r;
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
    m_bAABBDirty = true;
}

//=============================================================================================================

void BrainSurface::transform(const QMatrix4x4 &m)
{
    // Extract 3x3 normal matrix (inverse transpose of upper-left 3x3)
    // QMatrix4x4::normalMatrix() returns QMatrix3x3.
    QMatrix3x3 normalMat = m.normalMatrix();

    for (auto &v : m_vertexData) {
        // Transform position
        v.pos = m.map(v.pos);
        
        // Transform normal
        // Note: QMatrix3x3 * QVector3D isn't directly supported by some Qt versions conveniently,
        // but let's assume standard multiplication works or do manually.
        // Actually QVector3D operator*(QMatrix4x4) exists but is row-vector mul.
        // QMatrix4x4 operator*(QVector3D) is standard column-vector mul.
        
        // Use generic map method or just manual multiply if needed.
        // Easier: mapVector for vectors (ignores translation) but needs to be normal matrix for non-uniform scales.
        // If scale is uniform, mapVector is fine.
        
        // Let's do it manually to be safe with QMatrix3x3
        const float *d = normalMat.constData();
        float nx = d[0]*v.norm.x() + d[3]*v.norm.y() + d[6]*v.norm.z();
        float ny = d[1]*v.norm.x() + d[4]*v.norm.y() + d[7]*v.norm.z();
        float nz = d[2]*v.norm.x() + d[5]*v.norm.y() + d[8]*v.norm.z();
        v.norm = QVector3D(nx, ny, nz).normalized();
    }
    m_bBuffersDirty = true;
    m_bAABBDirty = true;
}

//=============================================================================================================

void BrainSurface::applyTransform(const QMatrix4x4 &m)
{
    m_vertexData = m_originalVertexData;
    if (!m.isIdentity()) {
        transform(m);
    } else {
        m_bBuffersDirty = true;
        m_bAABBDirty = true;
    }
}

//=============================================================================================================

void BrainSurface::updateBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    if (!m_bBuffersDirty && m_vertexBuffer && m_indexBuffer) return;

    const quint32 vbufSize = m_vertexData.size() * sizeof(VertexData);
    const quint32 ibufSize = m_indexData.size() * sizeof(uint32_t);

    if (!m_vertexBuffer) {
        m_vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbufSize));
        m_vertexBuffer->create();
    }
    if (!m_indexBuffer) {
        m_indexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, ibufSize));
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


//=============================================================================================================

void BrainSurface::boundingBox(QVector3D &min, QVector3D &max) const
{
    if (!m_bAABBDirty) {
        min = m_aabbMin;
        max = m_aabbMax;
        return;
    }

    if (m_vertexData.isEmpty()) {
        min = QVector3D(0,0,0);
        max = QVector3D(0,0,0);
        return;
    }

    min = m_vertexData[0].pos;
    max = m_vertexData[0].pos;

    for (const auto &v : m_vertexData) {
        min.setX(std::min(min.x(), v.pos.x()));
        min.setY(std::min(min.y(), v.pos.y()));
        min.setZ(std::min(min.z(), v.pos.z()));
        
        max.setX(std::max(max.x(), v.pos.x()));
        max.setY(std::max(max.y(), v.pos.y()));
        max.setZ(std::max(max.z(), v.pos.z()));
    }

    m_aabbMin = min;
    m_aabbMax = max;
    m_bAABBDirty = false;
}

bool BrainSurface::intersects(const QVector3D &rayOrigin, const QVector3D &rayDir, float &dist, int &vertexIdx) const
{
    vertexIdx = -1;
    if (m_vertexData.isEmpty()) return false;

    // 1. AABB Check (Cached)
    QVector3D min, max;
    boundingBox(min, max);
    
    // Ray-AABB slab method (Double Precision for stability)
    double eps = 1e-4;
    double origin[3] = {rayOrigin.x(), rayOrigin.y(), rayOrigin.z()};
    double dir[3] = {rayDir.x(), rayDir.y(), rayDir.z()};
    double minB[3] = {min.x() - eps, min.y() - eps, min.z() - eps};
    double maxB[3] = {max.x() + eps, max.y() + eps, max.z() + eps};

    // Extract individual components for triangle intersection (Möller–Trumbore)
    double originX = origin[0], originY = origin[1], originZ = origin[2];
    double dirX = dir[0], dirY = dir[1], dirZ = dir[2];

    double tmin = -std::numeric_limits<double>::max();
    double tmax = std::numeric_limits<double>::max();

    for (int i = 0; i < 3; ++i) {
        if (std::abs(dir[i]) < 1e-15) {
            if (origin[i] < minB[i] || origin[i] > maxB[i]) return false;
        } else {
            double t1 = (minB[i] - origin[i]) / dir[i];
            double t2 = (maxB[i] - origin[i]) / dir[i];
            if (t1 > t2) std::swap(t1, t2);
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
    }
    
    if (tmax < 1e-7) return false;

    // 2. Triangle intersection
    double closestDist = std::numeric_limits<double>::max();
    bool hit = false;
    int closestVert = -1;
    
    // Brute-force triangle intersection using Double Precision Möller–Trumbore
    // Note: For 100k+ vertices this is slow, ideally we'd use an Octree/BVH.
    // However, since we only do this on mouse-over, results are usually acceptable if not too many surfaces are active.
    for (int i = 0; i < m_indexData.size(); i += 3) {
        int i0 = m_indexData[i];
        int i1 = m_indexData[i+1];
        int i2 = m_indexData[i+2];
        const QVector3D &v0q = m_vertexData[i0].pos;
        const QVector3D &v1q = m_vertexData[i1].pos;
        const QVector3D &v2q = m_vertexData[i2].pos;

        double v0x = v0q.x(), v0y = v0q.y(), v0z = v0q.z();
        double v1x = v1q.x(), v1y = v1q.y(), v1z = v1q.z();
        double v2x = v2q.x(), v2y = v2q.y(), v2z = v2q.z();
        
        double edge1x = v1x - v0x, edge1y = v1y - v0y, edge1z = v1z - v0z;
        double edge2x = v2x - v0x, edge2y = v2y - v0y, edge2z = v2z - v0z;
        
        double hx = dirY * edge2z - dirZ * edge2y;
        double hy = dirZ * edge2x - dirX * edge2z;
        double hz = dirX * edge2y - dirY * edge2x;
        
        double a = edge1x * hx + edge1y * hy + edge1z * hz;
        if (std::abs(a) < 1e-18) continue; // Purely parallel
        
        double f = 1.0 / a;
        double sx = originX - v0x, sy = originY - v0y, sz = originZ - v0z;
        double u = f * (sx * hx + sy * hy + sz * hz);
        if (u < -1e-7 || u > 1.0000001) continue;
        
        double qx = sy * edge1z - sz * edge1y;
        double qy = sz * edge1x - sx * edge1z;
        double qz = sx * edge1y - sy * edge1x;
        
        double v = f * (dirX * qx + dirY * qy + dirZ * qz);
        if (v < -1e-7 || u + v > 1.0000001) continue;
        
        double t = f * (edge2x * qx + edge2y * qy + edge2z * qz);
        if (t > 1e-7 && t < closestDist) {
            closestDist = t;
            hit = true;
            
            // Find closest vertex of the hit triangle to the hit point
            double hitX = originX + t * dirX;
            double hitY = originY + t * dirY;
            double hitZ = originZ + t * dirZ;
            
            double d0 = (v0x - hitX)*(v0x - hitX) + (v0y - hitY)*(v0y - hitY) + (v0z - hitZ)*(v0z - hitZ);
            double d1 = (v1x - hitX)*(v1x - hitX) + (v1y - hitY)*(v1y - hitY) + (v1z - hitZ)*(v1z - hitZ);
            double d2 = (v2x - hitX)*(v2x - hitX) + (v2y - hitY)*(v2y - hitY) + (v2z - hitZ)*(v2z - hitZ);
            
            if (d0 < d1 && d0 < d2) closestVert = i0;
            else if (d1 < d2) closestVert = i1;
            else closestVert = i2;
        }
    }
    
    if (hit) {
        dist = static_cast<float>(closestDist);
        vertexIdx = closestVert;
        return true;
    }
    
    // 3. Proximity Fallback: If no exact intersection, find closest vertex to the ray
    // This handles cases where the ray passes between triangles or near edges
    // Inspired by legacy MNE's nearest_triangle_point approach
    
    // Compute a reasonable threshold based on scene scale (~3mm for brain surfaces)
    double proximityThreshold = 0.003; // 3mm in meters
    double closestVertDist = std::numeric_limits<double>::max();
    int fallbackVert = -1;
    double fallbackT = 0.0;
    
    for (int i = 0; i < m_vertexData.size(); ++i) {
        const QVector3D &vq = m_vertexData[i].pos;
        double vx = vq.x(), vy = vq.y(), vz = vq.z();
        
        // Vector from ray origin to vertex
        double px = vx - originX;
        double py = vy - originY;
        double pz = vz - originZ;
        
        // Project vertex onto ray: t = (P dot D) / (D dot D)
        // Since rayDir is normalized, D dot D = 1
        double t = px * dirX + py * dirY + pz * dirZ;
        
        // Only consider vertices in front of the camera
        if (t < 1e-7) continue;
        
        // Closest point on ray to vertex
        double cpx = originX + t * dirX;
        double cpy = originY + t * dirY;
        double cpz = originZ + t * dirZ;
        
        // Distance from vertex to closest point on ray
        double dx = vx - cpx;
        double dy = vy - cpy;
        double dz = vz - cpz;
        double distToRay = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (distToRay < proximityThreshold && distToRay < closestVertDist) {
            closestVertDist = distToRay;
            fallbackVert = i;
            fallbackT = t;
        }
    }
    
    if (fallbackVert >= 0) {
        dist = static_cast<float>(fallbackT);
        vertexIdx = fallbackVert;
        return true;
    }
    
    return false;
}

//=============================================================================================================

QString BrainSurface::getAnnotationLabel(int vertexIdx) const
{
    if (!m_hasAnnotation || vertexIdx < 0 || vertexIdx >= m_vertexData.size()) {
        return "";
    }

    const Eigen::VectorXi &vertices = m_annotation.getVertices();
    const Eigen::VectorXi &labelIds = m_annotation.getLabelIds();
    const FSLIB::Colortable &ct = m_annotation.getColortable();

    // The .annot file might not contain all vertices if it's sparse, 
    // but usually it contains a mapping for all.
    // Let's find the labelId for this vertex.
    int labelId = -1;
    for (int i = 0; i < vertices.rows(); ++i) {
        if (vertices(i) == vertexIdx) {
            labelId = labelIds(i);
            break;
        }
    }

    if (labelId == -1) return "Unknown";

    // Find the name in colortable
    for (int i = 0; i < ct.numEntries; ++i) {
        if (ct.table(i, 4) == labelId) {
            QString name = ct.struct_names[i];
            // Remove null characters and trailing whitespace that might cause "strange signs"
            while (!name.isEmpty() && (name.endsWith('\0') || name.endsWith(' '))) {
                name.chop(1);
            }
            return name;
        }
    }

    return "Unknown";
}

int BrainSurface::getAnnotationLabelId(int vertexIdx) const
{
    if (!m_hasAnnotation || vertexIdx < 0) return -1;

    const Eigen::VectorXi &vertices = m_annotation.getVertices();
    const Eigen::VectorXi &labelIds = m_annotation.getLabelIds();

    for (int i = 0; i < vertices.rows(); ++i) {
        if (vertices(i) == vertexIdx) {
            return labelIds(i);
        }
    }

    return -1;
}

void BrainSurface::setSelectedRegion(int regionId)
{
    if (m_selectedRegionId != regionId) {
        m_selectedRegionId = regionId;
        updateVertexColors();
        m_bBuffersDirty = true;
    }
}

void BrainSurface::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        updateVertexColors();
        m_bBuffersDirty = true;
    }
}

void BrainSurface::setSelectedVertexRange(int start, int count)
{
    if (m_selectedVertexStart != start || m_selectedVertexCount != count) {
        m_selectedVertexStart = start;
        m_selectedVertexCount = count;
        updateVertexColors();
        m_bBuffersDirty = true;
    }
}
