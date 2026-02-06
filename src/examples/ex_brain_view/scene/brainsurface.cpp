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


    // Reset to base color first
    for (auto &v : m_vertexData) {
        v.color = baseVal; 
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

    // Apply selection highlight (brightening)
    if (m_selected || m_selectedRegionId != -1) {
        if (m_hasAnnotation && m_selectedRegionId != -1) {
            // Highlight only the selected region
            const Eigen::VectorXi &vertices = m_annotation.getVertices();
            const Eigen::VectorXi &labelIds = m_annotation.getLabelIds();
            
            for (int i = 0; i < labelIds.rows(); ++i) {
                if (labelIds(i) == m_selectedRegionId) {
                    int vertexIdx = vertices(i);
                    if (vertexIdx >= 0 && vertexIdx < m_vertexData.size()) {
                        uint32_t &c = m_vertexData[vertexIdx].color;
                        uint32_t a = (c >> 24) & 0xFF;
                        uint32_t b = (c >> 16) & 0xFF;
                        uint32_t g = (c >> 8) & 0xFF;
                        uint32_t r = c & 0xFF;

                        // Yellowish highlight (r+60, g+60, b+20)
                        r = std::min(255u, r + 70);
                        g = std::min(255u, g + 70);
                        b = std::min(255u, b + 20);

                        c = (a << 24) | (b << 16) | (g << 8) | r;
                    }
                }
            }
        } else if (m_selected) {
            // Highlight whole hemisphere if no specific region selected
            for (auto &v : m_vertexData) {
                uint32_t a = (v.color >> 24) & 0xFF;
                uint32_t b = (v.color >> 16) & 0xFF;
                uint32_t g = (v.color >> 8) & 0xFF;
                uint32_t r = v.color & 0xFF;

                r = std::min(255u, r + 40);
                g = std::min(255u, g + 40);
                b = std::min(255u, b + 40);

                v.color = (a << 24) | (b << 16) | (g << 8) | r;
            }
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
}

//=============================================================================================================

void BrainSurface::applyTransform(const QMatrix4x4 &m)
{
    m_vertexData = m_originalVertexData;
    if (!m.isIdentity()) {
        transform(m);
    } else {
        m_bBuffersDirty = true;
    }
}

//=============================================================================================================

void BrainSurface::updateBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    if (!m_bBuffersDirty && m_vertexBuffer && m_indexBuffer) return;
    
    // Debug logging


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


//=============================================================================================================

void BrainSurface::boundingBox(QVector3D &min, QVector3D &max) const
{
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
}

bool BrainSurface::intersects(const QVector3D &rayOrigin, const QVector3D &rayDir, float &dist, int &vertexIdx) const
{
    vertexIdx = -1;
    if (m_vertexData.isEmpty()) return false;

    // 1. AABB Check
    QVector3D min, max;
    boundingBox(min, max);
    
    // Ray-AABB slab method with small epsilon padding
    float eps = 1e-4f;
    float tmin = (min.x() - eps - rayOrigin.x()) / rayDir.x();
    float tmax = (max.x() + eps - rayOrigin.x()) / rayDir.x();
    if (tmin > tmax) std::swap(tmin, tmax);
    
    float tymin = (min.y() - eps - rayOrigin.y()) / rayDir.y();
    float tymax = (max.y() + eps - rayOrigin.y()) / rayDir.y();
    if (tmin > tymax || tymin > tmax) return false;
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;
    
    float tzmin = (min.z() - eps - rayOrigin.z()) / rayDir.z();
    float tzmax = (max.z() + eps - rayOrigin.z()) / rayDir.z();
    if (tzmin > tzmax) std::swap(tzmin, tzmax);
    
    if (tmin > tzmax || tzmin > tmax) return false;
    
    if (tzmin > tmin) tmin = tzmin;
    if (tzmax < tmax) tmax = tzmax;
    
    // Debug AABB failures for BrainSurface (large mesh)
    static int debugCounter = 0;
    if (m_vertexData.size() > 5000 && debugCounter++ % 60 == 0) {
        // qDebug() << "BrainSurface(Large) AABB Hit. tmin:" << tmin << "tmax:" << tmax << "Ray:" << rayOrigin << rayDir;
    }
    
    // access to tmin, tmax for debug if needed
    // If we passed AABB, check triangles if vertex count is small (e.g. Sensor)
    // For large brain meshes (100k+), this is too slow without Octree.
    // Heuristic: If < 1000 vertices, do full check. Else, accept AABB hit ?? 
    // No, AABB hit on brain is basically "entire screen".
    // For now: Only precise check for "Sensors" (small).
    
    // if (m_vertexData.size() > 5000) {
    //    // Just AABB for big meshes? No that's useless for picking specific gyri.
    //    // Return false for now for big meshes to avoid selecting the whole head when trying to pick sensors.
    //    return false; 
    // }

    float closestDist = std::numeric_limits<float>::max();
    bool hit = false;
    int closestVert = -1;
    
    // Brute-force triangle intersection
    for (int i = 0; i < m_indexData.size(); i += 3) {
        int i0 = m_indexData[i];
        int i1 = m_indexData[i+1];
        int i2 = m_indexData[i+2];
        const QVector3D &v0 = m_vertexData[i0].pos;
        const QVector3D &v1 = m_vertexData[i1].pos;
        const QVector3D &v2 = m_vertexData[i2].pos;
        
        // Möller–Trumbore intersection algorithm
        // Relaxing epsilon for steep angles/large scales
        const float EPSILON = 1e-6f; 
        QVector3D edge1 = v1 - v0;
        QVector3D edge2 = v2 - v0;
        QVector3D h = QVector3D::crossProduct(rayDir, edge2);
        float a = QVector3D::dotProduct(edge1, h);
        
        // Use backface culling or double sided? 
        // For brain we usually want double sided if we click from inside, 
        // but for now let's just make it more robust.
        if (std::abs(a) < 1e-10f) continue; // Extremely parallel
        
        const float BARY_EPSILON = 1e-4f;
        
        float f = 1.0f / a;
        QVector3D s = rayOrigin - v0;
        float u = f * QVector3D::dotProduct(s, h);
        if (u < -BARY_EPSILON || u > 1.0f + BARY_EPSILON) continue;
        
        QVector3D q = QVector3D::crossProduct(s, edge1);
        float v = f * QVector3D::dotProduct(rayDir, q);
        if (v < -BARY_EPSILON || u + v > 1.0f + BARY_EPSILON) continue;
        
        float t = f * QVector3D::dotProduct(edge2, q);
        if (t > 0.0f && t < closestDist) {
            closestDist = t;
            hit = true;
            
            // Find closest vertex of the hit triangle to the hit point
            QVector3D hitPoint = rayOrigin + t * rayDir;
            float d0 = (v0 - hitPoint).lengthSquared();
            float d1 = (v1 - hitPoint).lengthSquared();
            float d2 = (v2 - hitPoint).lengthSquared();
            
            if (d0 < d1 && d0 < d2) closestVert = i0;
            else if (d1 < d2) closestVert = i1;
            else closestVert = i2;
        }
    }
    
    if (hit) {
        dist = closestDist;
        vertexIdx = closestVert;
    }
    
    return hit;
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
