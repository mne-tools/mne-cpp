//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainsurface.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    CPU mesh build, per-vertex colour packing and QRhi vertex / index buffer management for cortical and BEM surfaces.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainsurface.h"

#include <QHash>
#include <QMatrix3x3>
#include <QMatrix4x4>

#include <algorithm>
#include <limits>
#include <set>

namespace DISP3DLIB
{

namespace
{
uint32_t withAlpha(uint32_t color, uint32_t alpha)
{
    return (color & 0x00FFFFFFu) | ((alpha & 0xFFu) << 24);
}

uint32_t curvatureGray(const QVector<float>& curvature, int index)
{
    return (index >= 0 && index < curvature.size() && curvature[index] > 0.0f)
        ? 0x40u
        : 0xAAu;
}

// Overlay alpha is blended into RGB here because the shader keeps the curvature grey in the alpha channel.
uint32_t overCortex(uint32_t overlay, uint32_t gray)
{
    const uint32_t a = overlay >> 24;
    uint32_t rgb = 0;
    for (int shift = 0; shift < 24; shift += 8) {
        const uint32_t c = (overlay >> shift) & 0xFFu;
        rgb |= ((c * a + gray * (255u - a) + 127u) / 255u) << shift;
    }
    return withAlpha(rgb, gray);
}
} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainSurface::BrainSurface() = default;

//=============================================================================================================

BrainSurface::~BrainSurface() = default;

//=============================================================================================================

void BrainSurface::markGeometryDirty()
{
    ++m_geometryGeneration;
    m_bAABBDirty = true;
}

//=============================================================================================================

GpuStream BrainSurface::geometryStream() const
{
    return {m_vertexData.constData(), quint32(m_vertexData.size() * sizeof(VertexData)), m_geometryGeneration};
}

GpuStream BrainSurface::colorStream() const
{
    return {m_colors.constData(), quint32(m_colors.size() * sizeof(uint32_t)), m_colorGeneration};
}

GpuStream BrainSurface::annotationStream() const
{
    return {m_annotationColors.constData(), quint32(m_annotationColors.size() * sizeof(uint32_t)), m_annotationGeneration};
}

GpuStream BrainSurface::indexStream() const
{
    return {m_indexData.constData(), quint32(m_indexData.size() * sizeof(uint32_t)), m_indexGeneration};
}

//=============================================================================================================

void BrainSurface::setGeometry(QVector<VertexData> vertices, QVector<uint32_t> indices, const QColor& color)
{
    m_vertexData = std::move(vertices);
    m_originalVertexData = m_vertexData;
    m_indexData = std::move(indices);
    m_indexCount = m_indexData.size();
    ++m_indexGeneration;
    markGeometryDirty();
    m_defaultColor = color;
    m_baseColor = color;
    m_colors.resize(m_vertexData.size());
    m_annotationColors.fill(0u, m_vertexData.size());
    m_vertexLabelIds.fill(-1, m_vertexData.size());
    ++m_annotationGeneration;
    if (m_hasAnnotation)
        addAnnotation(m_annotation);
    updateVertexColors();
}

//=============================================================================================================

void BrainSurface::fromSurface(const FSLIB::FsSurface& surf)
{
    const Eigen::MatrixXf& rr = surf.rr();
    const Eigen::MatrixXf& nn = surf.nn();
    const Eigen::MatrixXi& tris = surf.tris();
    const Eigen::VectorXf& curv = surf.curv();
    m_curvature.resize(curv.size());
    for (int i = 0; i < curv.size(); ++i)
        m_curvature[i] = curv[i];

    QVector<VertexData> vertices(rr.rows());
    for (int i = 0; i < rr.rows(); ++i)
        vertices[i] = {QVector3D(rr(i, 0), rr(i, 1), rr(i, 2)), QVector3D(nn(i, 0), nn(i, 1), nn(i, 2))};
    QVector<uint32_t> indices(tris.rows() * 3);
    for (int i = 0; i < tris.rows(); ++i)
        for (int j = 0; j < 3; ++j)
            indices[3 * i + j] = tris(i, j);
    setGeometry(std::move(vertices), std::move(indices), Qt::white);
}

//=============================================================================================================

void BrainSurface::fromBemSurface(const MNELIB::MNEBemSurface& surf, const QColor& color)
{
    m_curvature.clear();

    const int nVerts = surf.rr.rows();
    Eigen::MatrixX3f nn = surf.nn;
    if (nn.rows() != nVerts) {
        nn = FSLIB::FsSurface::compute_normals(Eigen::MatrixX3f(surf.rr), Eigen::MatrixX3i(surf.itris));
    }
    QVector<VertexData> vertices(nVerts);
    for (int i = 0; i < nVerts; ++i)
        vertices[i] = {QVector3D(surf.rr(i, 0), surf.rr(i, 1), surf.rr(i, 2)), QVector3D(nn(i, 0), nn(i, 1), nn(i, 2))};
    QVector<uint32_t> indices(surf.itris.rows() * 3);
    for (int i = 0; i < surf.itris.rows(); ++i)
        for (int j = 0; j < 3; ++j)
            indices[3 * i + j] = surf.itris(i, j);
    setGeometry(std::move(vertices), std::move(indices), color);
}

void BrainSurface::createFromData(const Eigen::MatrixX3f& vertices, const Eigen::MatrixX3i& triangles, const QColor& color)
{
    createFromData(vertices, FSLIB::FsSurface::compute_normals(vertices, triangles), triangles, color);
}

void BrainSurface::createFromData(const Eigen::MatrixX3f& vertices, const Eigen::MatrixX3f& normals, const Eigen::MatrixX3i& triangles, const QColor& color)
{
    m_curvature.clear();

    QVector<VertexData> verts(vertices.rows());
    for (int i = 0; i < vertices.rows(); ++i)
        verts[i] = {QVector3D(vertices(i, 0), vertices(i, 1), vertices(i, 2)), QVector3D(normals(i, 0), normals(i, 1), normals(i, 2))};
    QVector<uint32_t> indices(triangles.rows() * 3);
    for (int i = 0; i < triangles.rows(); ++i)
        for (int j = 0; j < 3; ++j)
            indices[3 * i + j] = triangles(i, j);
    setGeometry(std::move(verts), std::move(indices), color);
}

//=============================================================================================================

Eigen::MatrixX3f BrainSurface::vertexPositions() const
{
    Eigen::MatrixX3f rr(m_vertexData.size(), 3);
    for (int i = 0; i < m_vertexData.size(); ++i) {
        rr(i, 0) = m_vertexData[i].pos.x();
        rr(i, 1) = m_vertexData[i].pos.y();
        rr(i, 2) = m_vertexData[i].pos.z();
    }
    return rr;
}

//=============================================================================================================

Eigen::MatrixX3f BrainSurface::vertexNormals() const
{
    Eigen::MatrixX3f nn(m_vertexData.size(), 3);
    for (int i = 0; i < m_vertexData.size(); ++i) {
        nn(i, 0) = m_vertexData[i].norm.x();
        nn(i, 1) = m_vertexData[i].norm.y();
        nn(i, 2) = m_vertexData[i].norm.z();
    }
    return nn;
}

//=============================================================================================================

bool BrainSurface::loadAnnotation(const QString& path)
{
    FSLIB::FsAnnotation annotation;
    if (!FSLIB::FsAnnotation::read(path, annotation)) {
        qWarning() << "BrainSurface: Failed to load annotation from" << path;
        return false;
    }
    addAnnotation(annotation);
    return true;
}

void BrainSurface::addAnnotation(const FSLIB::FsAnnotation& annotation)
{
    m_annotation = annotation;
    m_hasAnnotation = true;

    // Per-vertex label ids and colours, so hover lookups and drawing need no search
    const Eigen::VectorXi& vertices = m_annotation.getVertices();
    const Eigen::VectorXi& labelIds = m_annotation.getLabelIds();
    const FSLIB::FsColortable& ct = m_annotation.getColortable();
    QHash<int, uint32_t> labelColors;
    for (int c = 0; c < ct.numEntries; ++c)
        labelColors.insert(ct.table(c, 4), packABGR(ct.table(c, 0), ct.table(c, 1), ct.table(c, 2)));
    m_vertexLabelIds.fill(-1, m_vertexData.size());
    m_annotationColors.fill(0u, m_vertexData.size());
    for (int i = 0; i < labelIds.rows(); ++i) {
        const int v = vertices(i);
        if (v < 0 || v >= m_vertexData.size())
            continue;
        m_vertexLabelIds[v] = labelIds(i);
        m_annotationColors[v] = labelColors.value(labelIds(i), 0u);
    }
    ++m_annotationGeneration;
}

//=============================================================================================================

void BrainSurface::labelElements(int verticesPerElement)
{
    if (verticesPerElement <= 0)
        return;
    // Alpha 0 keeps element keys apart from atlas colours, which are opaque
    for (int v = 0; v < m_annotationColors.size(); ++v) {
        const uint32_t key = uint32_t(v / verticesPerElement + 1);
        m_annotationColors[v] = packABGR(key & 0xFFu, (key >> 8) & 0xFFu, (key >> 16) & 0xFFu, 0u);
    }
    ++m_annotationGeneration;
}

//=============================================================================================================

void BrainSurface::setVisible(bool visible)
{
    m_visible = visible;
}

//=============================================================================================================

void BrainSurface::setVisualizationMode(VisualizationMode mode)
{
    // The colour stream holds the same data in every mode; each view picks the channel by uniform
    m_visMode = mode;
}

//=============================================================================================================

void BrainSurface::applySourceEstimateColors(const QVector<uint32_t>& colors)
{
    m_visMode = ModeSourceEstimate;
    m_stcColors = colors;

    // Preserve the neutral curvature grey in alpha so Surface mode can still render the light/dark
    // cortex while RGB carries the source-estimate colours.
    const int n = qMin(colors.size(), m_colors.size());
    for (int i = 0; i < n; ++i) {
        m_colors[i] = overCortex(colors[i], curvatureGray(m_curvature, i));
    }
    ++m_colorGeneration;
}

//=============================================================================================================

void BrainSurface::clearSourceEstimateColors()
{
    m_stcColors.clear();
    m_visMode = ModeSurface;
    updateVertexColors();
}

//=============================================================================================================

void BrainSurface::setUseDefaultColor(bool useDefault)
{
    m_baseColor = useDefault ? m_defaultColor : Qt::white;
    updateVertexColors();
}

//=============================================================================================================

void BrainSurface::setColor(const QColor& color)
{
    m_defaultColor = color;
    m_baseColor = color;
    updateVertexColors();
}

void BrainSurface::updateVertexColors()
{
    // Cortex: curvature grey; other surfaces (BEM, sensors, ...): the base colour
    if (!m_curvature.isEmpty() && m_curvature.size() == m_colors.size()) {
        for (int i = 0; i < m_colors.size(); ++i) {
            const uint32_t val = curvatureGray(m_curvature, i);
            m_colors[i] = packABGR(val, val, val, val);
        }
    } else {
        m_colors.fill(packABGR(m_baseColor.red(), m_baseColor.green(), m_baseColor.blue(), m_baseColor.alpha()));
    }

    // Source-estimate colours stay in the colour stream: each view selects its overlay by uniform
    const int n = qMin(m_stcColors.size(), m_colors.size());
    for (int i = 0; i < n; ++i) {
        m_colors[i] = overCortex(m_stcColors[i], curvatureGray(m_curvature, i));
    }
    ++m_colorGeneration;
}

float BrainSurface::minX() const
{
    float minVal = std::numeric_limits<float>::max();
    for (const auto& v : m_vertexData) {
        if (v.pos.x() < minVal)
            minVal = v.pos.x();
    }
    return minVal;
}

float BrainSurface::maxX() const
{
    float maxVal = std::numeric_limits<float>::lowest();
    for (const auto& v : m_vertexData) {
        if (v.pos.x() > maxVal)
            maxVal = v.pos.x();
    }
    return maxVal;
}

void BrainSurface::translateX(float offset)
{
    for (auto& v : m_vertexData) {
        v.pos.setX(v.pos.x() + offset);
    }
    markGeometryDirty();
}

//=============================================================================================================

void BrainSurface::transform(const QMatrix4x4& m)
{
    // Extract 3x3 normal matrix (inverse transpose of upper-left 3x3)
    // QMatrix4x4::normalMatrix() returns QMatrix3x3.
    QMatrix3x3 normalMat = m.normalMatrix();

    for (auto& v : m_vertexData) {
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
        const float* d = normalMat.constData();
        float nx = d[0] * v.norm.x() + d[3] * v.norm.y() + d[6] * v.norm.z();
        float ny = d[1] * v.norm.x() + d[4] * v.norm.y() + d[7] * v.norm.z();
        float nz = d[2] * v.norm.x() + d[5] * v.norm.y() + d[8] * v.norm.z();
        v.norm = QVector3D(nx, ny, nz).normalized();
    }
    markGeometryDirty();
}

//=============================================================================================================

void BrainSurface::applyTransform(const QMatrix4x4& m)
{
    m_vertexData = m_originalVertexData;
    if (!m.isIdentity()) {
        transform(m);
    } else {
        markGeometryDirty();
    }
}

//=============================================================================================================

std::vector<Eigen::VectorXi> BrainSurface::computeNeighbors() const
{
    // Use temporary std::vector<std::set<int>> for dedup during construction
    std::vector<std::set<int>> tempNeighbors(m_vertexData.size());

    // Triangles are stored in m_indexData as triplets
    for (int i = 0; i + 2 < m_indexData.size(); i += 3) {
        int v0 = m_indexData[i];
        int v1 = m_indexData[i + 1];
        int v2 = m_indexData[i + 2];

        // Add bidirectional edges (set handles dedup)
        tempNeighbors[v0].insert(v1);
        tempNeighbors[v0].insert(v2);
        tempNeighbors[v1].insert(v0);
        tempNeighbors[v1].insert(v2);
        tempNeighbors[v2].insert(v0);
        tempNeighbors[v2].insert(v1);
    }

    // Convert to std::vector<VectorXi>
    std::vector<Eigen::VectorXi> neighbors(tempNeighbors.size());
    for (size_t k = 0; k < tempNeighbors.size(); ++k) {
        const auto& s = tempNeighbors[k];
        neighbors[k].resize(static_cast<Eigen::Index>(s.size()));
        Eigen::Index idx = 0;
        for (int val : s) {
            neighbors[k][idx++] = val;
        }
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

void BrainSurface::boundingBox(QVector3D& min, QVector3D& max) const
{
    if (!m_bAABBDirty) {
        min = m_aabbMin;
        max = m_aabbMax;
        return;
    }

    if (m_vertexData.isEmpty()) {
        min = QVector3D(0, 0, 0);
        max = QVector3D(0, 0, 0);
        return;
    }

    min = m_vertexData[0].pos;
    max = m_vertexData[0].pos;

    for (const auto& v : m_vertexData) {
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

struct BrainSurface::Bvh
{
    struct Node
    {
        float min[3];
        float max[3];
        int first; // first triangle (leaf) or left child (inner node)
        int count; // triangles in a leaf, 0 for an inner node
        int right; // right child of an inner node
    };
    std::vector<Node> nodes;
    std::vector<int> triangles; // triangle indices, grouped by leaf
    quint64 geometryGeneration = 0;
    quint64 indexGeneration = 0;
};

namespace
{
constexpr int kBvhLeafSize = 4;
// Pads node bounds so a ray grazing a flat, axis-aligned box still enters it
constexpr float kBvhPad = 1e-6f;

void growBox(float* lo, float* hi, const QVector3D& p)
{
    for (int a = 0; a < 3; ++a) {
        lo[a] = std::min(lo[a], p[a]);
        hi[a] = std::max(hi[a], p[a]);
    }
}

// Ray parameter where the ray enters the box, or +inf when it misses it before maxT
float enterBox(const float* lo, const float* hi, const float* origin, const float* invDir, float maxT)
{
    float tNear = 0.0f;
    float tFar = maxT;
    for (int a = 0; a < 3; ++a) {
        float t0 = (lo[a] - origin[a]) * invDir[a];
        float t1 = (hi[a] - origin[a]) * invDir[a];
        if (t0 > t1)
            std::swap(t0, t1);
        // NaN (origin on a slab plane of a ray parallel to it) keeps the previous bound
        tNear = t0 > tNear ? t0 : tNear;
        tFar = t1 < tFar ? t1 : tFar;
        if (tNear > tFar)
            return std::numeric_limits<float>::infinity();
    }
    return tNear;
}
} // namespace

//=============================================================================================================

const BrainSurface::Bvh& BrainSurface::bvh() const
{
    if (m_bvh && m_bvh->geometryGeneration == m_geometryGeneration && m_bvh->indexGeneration == m_indexGeneration)
        return *m_bvh;

    auto tree = std::make_shared<Bvh>();
    tree->geometryGeneration = m_geometryGeneration;
    tree->indexGeneration = m_indexGeneration;
    const int triangleCount = int(m_indexData.size() / 3);

    // Triangle boxes in one contiguous array that the build partitions in place
    struct Item
    {
        float lo[3];
        int triangle;
        float hi[3];
        float pad;
    };
    std::vector<Item> items(static_cast<size_t>(triangleCount));
    for (int t = 0; t < triangleCount; ++t) {
        Item& item = items[size_t(t)];
        item.triangle = t;
        for (int a = 0; a < 3; ++a) {
            item.lo[a] = std::numeric_limits<float>::max();
            item.hi[a] = std::numeric_limits<float>::lowest();
        }
        for (int k = 0; k < 3; ++k)
            growBox(item.lo, item.hi, m_vertexData[m_indexData[3 * t + k]].pos);
    }
    tree->nodes.reserve(size_t(std::max(1, 2 * triangleCount / kBvhLeafSize + 1)));

    // Top-down build: split each node at the median box centre along its widest centre axis
    struct Task
    {
        int node;
        int begin;
        int end;
    };
    std::vector<Task> tasks;
    tree->nodes.push_back({});
    tasks.push_back({0, 0, triangleCount});
    while (!tasks.empty()) {
        const Task task = tasks.back();
        tasks.pop_back();
        Bvh::Node node{};
        float cLo[3];
        float cHi[3];
        for (int a = 0; a < 3; ++a) {
            node.min[a] = cLo[a] = std::numeric_limits<float>::max();
            node.max[a] = cHi[a] = std::numeric_limits<float>::lowest();
        }
        for (int i = task.begin; i < task.end; ++i) {
            const Item& item = items[size_t(i)];
            for (int a = 0; a < 3; ++a) {
                node.min[a] = std::min(node.min[a], item.lo[a]);
                node.max[a] = std::max(node.max[a], item.hi[a]);
                const float c = item.lo[a] + item.hi[a];
                cLo[a] = std::min(cLo[a], c);
                cHi[a] = std::max(cHi[a], c);
            }
        }
        for (int a = 0; a < 3; ++a) {
            node.min[a] -= kBvhPad;
            node.max[a] += kBvhPad;
        }

        const int axis = (cHi[1] - cLo[1] > cHi[0] - cLo[0]) ? ((cHi[2] - cLo[2] > cHi[1] - cLo[1]) ? 2 : 1)
                                                             : ((cHi[2] - cLo[2] > cHi[0] - cLo[0]) ? 2 : 0);
        if (task.end - task.begin <= kBvhLeafSize || cHi[axis] <= cLo[axis]) {
            node.first = task.begin;
            node.count = task.end - task.begin;
            tree->nodes[size_t(task.node)] = node;
            continue;
        }
        const int mid = (task.begin + task.end) / 2;
        std::nth_element(items.begin() + task.begin, items.begin() + mid, items.begin() + task.end,
                         [axis](const Item& l, const Item& r) { return l.lo[axis] + l.hi[axis] < r.lo[axis] + r.hi[axis]; });
        const int left = int(tree->nodes.size());
        const int right = left + 1;
        tree->nodes.resize(tree->nodes.size() + 2);
        node.first = left;
        node.count = 0;
        node.right = right;
        tree->nodes[size_t(task.node)] = node;
        tasks.push_back({right, mid, task.end});
        tasks.push_back({left, task.begin, mid});
    }

    tree->triangles.resize(size_t(triangleCount));
    for (int i = 0; i < triangleCount; ++i)
        tree->triangles[size_t(i)] = items[size_t(i)].triangle;

    m_bvh = std::move(tree);
    return *m_bvh;
}

//=============================================================================================================

bool BrainSurface::intersects(const QVector3D& rayOrigin, const QVector3D& rayDir, float& dist, int& vertexIdx) const
{
    vertexIdx = -1;
    if (m_vertexData.isEmpty() || m_indexData.size() < 3)
        return false;

    const Bvh& tree = bvh();
    const float origin[3] = {rayOrigin.x(), rayOrigin.y(), rayOrigin.z()};
    const float invDir[3] = {1.0f / rayDir.x(), 1.0f / rayDir.y(), 1.0f / rayDir.z()};
    const double originX = rayOrigin.x(), originY = rayOrigin.y(), originZ = rayOrigin.z();
    const double dirX = rayDir.x(), dirY = rayDir.y(), dirZ = rayDir.z();

    double closestDist = std::numeric_limits<double>::max();
    int closestTriangle = -1;

    // Möller–Trumbore in double precision on the triangles of the leaves the ray enters, nearest
    // first. Median splits keep the depth near log2(triangles / leaf size), far below the stack size.
    int stack[64];
    int top = 0;
    stack[top++] = 0;
    while (top > 0) {
        const Bvh::Node& node = tree.nodes[size_t(stack[--top])];
        const float limit = closestDist < double(std::numeric_limits<float>::max()) ? float(closestDist) : std::numeric_limits<float>::max();
        if (enterBox(node.min, node.max, origin, invDir, limit) == std::numeric_limits<float>::infinity())
            continue;
        if (node.count == 0) {
            const Bvh::Node& l = tree.nodes[size_t(node.first)];
            const Bvh::Node& r = tree.nodes[size_t(node.right)];
            const float tl = enterBox(l.min, l.max, origin, invDir, limit);
            const float tr = enterBox(r.min, r.max, origin, invDir, limit);
            constexpr float miss = std::numeric_limits<float>::infinity();
            // Push the farther child first so the nearer one is visited next
            const bool leftFirst = tl <= tr;
            const int nearChild = leftFirst ? node.first : node.right;
            const int farChild = leftFirst ? node.right : node.first;
            if ((leftFirst ? tr : tl) != miss)
                stack[top++] = farChild;
            if ((leftFirst ? tl : tr) != miss)
                stack[top++] = nearChild;
            continue;
        }

        for (int i = node.first; i < node.first + node.count; ++i) {
            const int t = tree.triangles[size_t(i)];
            const QVector3D& v0q = m_vertexData[m_indexData[3 * t]].pos;
            const QVector3D& v1q = m_vertexData[m_indexData[3 * t + 1]].pos;
            const QVector3D& v2q = m_vertexData[m_indexData[3 * t + 2]].pos;

            const double v0x = v0q.x(), v0y = v0q.y(), v0z = v0q.z();
            const double edge1x = v1q.x() - v0x, edge1y = v1q.y() - v0y, edge1z = v1q.z() - v0z;
            const double edge2x = v2q.x() - v0x, edge2y = v2q.y() - v0y, edge2z = v2q.z() - v0z;

            const double hx = dirY * edge2z - dirZ * edge2y;
            const double hy = dirZ * edge2x - dirX * edge2z;
            const double hz = dirX * edge2y - dirY * edge2x;

            const double a = edge1x * hx + edge1y * hy + edge1z * hz;
            if (std::abs(a) < 1e-18)
                continue; // Parallel to the triangle

            const double f = 1.0 / a;
            const double sx = originX - v0x, sy = originY - v0y, sz = originZ - v0z;
            const double u = f * (sx * hx + sy * hy + sz * hz);
            if (u < -1e-7 || u > 1.0000001)
                continue;

            const double qx = sy * edge1z - sz * edge1y;
            const double qy = sz * edge1x - sx * edge1z;
            const double qz = sx * edge1y - sy * edge1x;

            const double v = f * (dirX * qx + dirY * qy + dirZ * qz);
            if (v < -1e-7 || u + v > 1.0000001)
                continue;

            const double hitT = f * (edge2x * qx + edge2y * qy + edge2z * qz);
            if (hitT > 1e-7 && hitT < closestDist) {
                closestDist = hitT;
                closestTriangle = t;
            }
        }
    }

    if (closestTriangle < 0)
        return false;

    // The vertex of the hit triangle nearest to the hit point, used for region lookup
    const QVector3D hit = rayOrigin + float(closestDist) * rayDir;
    float best = std::numeric_limits<float>::max();
    for (int k = 0; k < 3; ++k) {
        const int v = int(m_indexData[3 * closestTriangle + k]);
        const float d = (m_vertexData[v].pos - hit).lengthSquared();
        if (d < best) {
            best = d;
            vertexIdx = v;
        }
    }
    dist = float(closestDist);
    return true;
}

//=============================================================================================================

QString BrainSurface::getAnnotationLabel(int vertexIdx) const
{
    if (!m_hasAnnotation || vertexIdx < 0 || vertexIdx >= m_vertexLabelIds.size()) {
        return "";
    }
    const int labelId = m_vertexLabelIds[vertexIdx];
    if (labelId == -1)
        return "Unknown";

    const FSLIB::FsColortable& ct = m_annotation.getColortable();
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
    if (!m_hasAnnotation || vertexIdx < 0 || vertexIdx >= m_vertexLabelIds.size())
        return -1;
    return m_vertexLabelIds[vertexIdx];
}

void BrainSurface::setSelectedRegion(int regionId)
{
    m_selectedRegionId = regionId;
}

void BrainSurface::setSelected(bool selected)
{
    m_selected = selected;
}

void BrainSurface::setSelectedVertexRange(int start, int count)
{
    m_selectedVertexStart = start;
    m_selectedVertexCount = count;
}

std::optional<uint32_t> BrainSurface::selectedLabel() const
{
    if (m_selectedVertexStart >= 0 && m_selectedVertexCount > 0 && m_selectedVertexStart < m_annotationColors.size())
        return m_annotationColors[m_selectedVertexStart];
    if (m_selected && m_selectedRegionId != -1 && m_hasAnnotation) {
        const FSLIB::FsColortable& ct = m_annotation.getColortable();
        for (int c = 0; c < ct.numEntries; ++c) {
            if (ct.table(c, 4) == m_selectedRegionId)
                return packABGR(ct.table(c, 0), ct.table(c, 1), ct.table(c, 2));
        }
    }
    return std::nullopt;
}

} // namespace DISP3DLIB
