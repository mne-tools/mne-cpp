//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_morph_map.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref MNELIB::MNEMorphMap (MNE-C mne_morph_maps.c).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_morph_map.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

constexpr int kHemiKind[2] = {FIFFV_MNE_SURF_LEFT_HEMI, FIFFV_MNE_SURF_RIGHT_HEMI};

//=============================================================================================================
/**
 * Exact nearest-neighbour search on the unit sphere with a uniform cell grid.
 */
class SphereGrid
{
public:
    explicit SphereGrid(const MatrixX3d& points)
    : m_points(points)
    , m_n(std::max(1, static_cast<int>(std::cbrt(static_cast<double>(points.rows()) / 2.0))))
    , m_cells(static_cast<std::size_t>(m_n) * m_n * m_n)
    {
        for (Index k = 0; k < points.rows(); ++k) {
            m_cells[cellIndex(cellOf(points.row(k)))].push_back(static_cast<int>(k));
        }
    }

    int nearest(const RowVector3d& r) const
    {
        const Vector3i c = cellOf(r);
        const double size = 2.0 / m_n;
        int best = -1;
        double bestDist = std::numeric_limits<double>::max();
        // Grow the searched cube until no cell outside it can hold a closer point.
        for (int ring = 0; ring <= m_n; ++ring) {
            for (int i = c.x() - ring; i <= c.x() + ring; ++i) {
                for (int j = c.y() - ring; j <= c.y() + ring; ++j) {
                    for (int k = c.z() - ring; k <= c.z() + ring; ++k) {
                        const bool onShell = std::max({std::abs(i - c.x()), std::abs(j - c.y()), std::abs(k - c.z())}) == ring;
                        if (!onShell || i < 0 || j < 0 || k < 0 || i >= m_n || j >= m_n || k >= m_n) {
                            continue;
                        }
                        for (int p : m_cells[cellIndex(Vector3i(i, j, k))]) {
                            const double d = (m_points.row(p) - r).squaredNorm();
                            if (d < bestDist) {
                                bestDist = d;
                                best = p;
                            }
                        }
                    }
                }
            }
            if (best >= 0 && std::sqrt(bestDist) <= ring * size) {
                break;
            }
        }
        return best;
    }

private:
    Vector3i cellOf(const RowVector3d& r) const
    {
        Vector3i c;
        for (int d = 0; d < 3; ++d) {
            c[d] = std::clamp(static_cast<int>((r[d] + 1.0) / 2.0 * m_n), 0, m_n - 1);
        }
        return c;
    }

    std::size_t cellIndex(const Vector3i& c) const
    {
        return (static_cast<std::size_t>(c.x()) * m_n + c.y()) * m_n + c.z();
    }

    const MatrixX3d& m_points;
    int m_n;
    std::vector<std::vector<int>> m_cells;
};

//=============================================================================================================
/**
 * Projection of a point onto the plane of one triangle: barycentric (p, q), signed plane distance and the
 * edge metric a = |r12|^2, b = |r13|^2, c = r12.r13 (MNE-C mne_triangle_coords).
 */
struct TriangleProjection
{
    double p;
    double q;
    double dist;
    double a;
    double b;
    double c;

    TriangleProjection(const RowVector3d& r, const RowVector3d& r1, const RowVector3d& r2, const RowVector3d& r3)
    {
        const RowVector3d r12 = r2 - r1;
        const RowVector3d r13 = r3 - r1;
        const RowVector3d d = r - r1;
        a = r12.squaredNorm();
        b = r13.squaredNorm();
        c = r12.dot(r13);
        double det = a * b - c * c;
        if (det == 0.0) {
            det = 1.0;
        }
        const double v1 = d.dot(r12);
        const double v2 = d.dot(r13);
        p = (b * v1 - c * v2) / det;
        q = (a * v2 - c * v1) / det;
        dist = d.dot(r12.cross(r13).normalized());
    }

    bool inside() const
    {
        return p >= 0.0 && q >= 0.0 && p <= 1.0 && q <= 1.0 && p + q < 1.0;
    }

    /** Distance to the edge point (p0, q0), measured in the triangle metric plus the plane offset. */
    double edgeDistance(double p0, double q0) const
    {
        const double dp = p - p0;
        const double dq = q - q0;
        return std::sqrt(dp * dp * a + dq * dq * b + dp * dq * c + dist * dist);
    }

    /** Nearest point on the three triangle sides (MNE-C nearest_triangle_point edge search). */
    double nearestEdge(double& pOut, double& qOut) const
    {
        const double sides[3][2] = {
            {std::clamp(p + 0.5 * q * c / a, 0.0, 1.0), 0.0},
            {0.0, std::clamp(0.5 * ((2.0 * a - c) * (1.0 - p) + (2.0 * b - c) * q) / (a + b - c), 0.0, 1.0)},
            {0.0, std::clamp(q + 0.5 * p * c / b, 0.0, 1.0)},
        };
        double best = std::numeric_limits<double>::max();
        for (int side = 0; side < 3; ++side) {
            const double p0 = side == 1 ? 1.0 - sides[1][1] : sides[side][0];
            const double q0 = sides[side][1];
            const double d = edgeDistance(p0, q0);
            if (d < best) {
                best = d;
                pOut = p0;
                qOut = q0;
            }
        }
        return best;
    }
};

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEMorphMap::MNEMorphMap(const MNEMorphMap& other)
: map(other.map ? std::make_unique<FiffSparseMatrix>(*other.map) : nullptr)
, best(other.best)
, hemi(other.hemi)
, from_subj(other.from_subj)
, to_subj(other.to_subj)
{
}

//=============================================================================================================

MNEMorphMap MNEMorphMap::compute(const MatrixX3f& fromRr, const MatrixX3i& fromTris, const MatrixX3f& toRr)
{
    const MatrixX3d from = fromRr.cast<double>().rowwise().normalized();
    const MatrixX3d to = toRr.cast<double>().rowwise().normalized();

    std::vector<std::vector<int>> trisOfVertex(from.rows());
    for (Index t = 0; t < fromTris.rows(); ++t) {
        for (int v = 0; v < 3; ++v) {
            trisOfVertex[fromTris(t, v)].push_back(static_cast<int>(t));
        }
    }

    MNEMorphMap result;
    result.best.resize(to.rows());
    const SphereGrid grid(from);
    std::vector<Triplet<float>> weights;
    weights.reserve(3 * to.rows());
    for (Index j = 0; j < to.rows(); ++j) {
        const RowVector3d r = to.row(j);
        const int nearest = grid.nearest(r);
        result.best[j] = nearest;
        // A triangle around the nearest vertex that contains the projected point wins (smallest plane
        // distance); only if none does, the nearest point on their sides is used.
        double bestDist = std::numeric_limits<double>::max();
        int bestTri = -1;
        double bestP = 0.0;
        double bestQ = 0.0;
        std::vector<std::pair<int, TriangleProjection>> outside;
        for (int t : trisOfVertex[nearest]) {
            const TriangleProjection proj(r, from.row(fromTris(t, 0)), from.row(fromTris(t, 1)), from.row(fromTris(t, 2)));
            if (!proj.inside()) {
                outside.emplace_back(t, proj);
            } else if (std::abs(proj.dist) < bestDist) {
                bestDist = std::abs(proj.dist);
                bestTri = t;
                bestP = proj.p;
                bestQ = proj.q;
            }
        }
        if (bestTri < 0) {
            for (const auto& [t, proj] : outside) {
                double p = 0.0;
                double q = 0.0;
                const double dist = proj.nearestEdge(p, q);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestTri = t;
                    bestP = p;
                    bestQ = q;
                }
            }
        }
        const int row = static_cast<int>(j);
        weights.emplace_back(row, fromTris(bestTri, 0), static_cast<float>(1.0 - bestP - bestQ));
        weights.emplace_back(row, fromTris(bestTri, 1), static_cast<float>(bestP));
        weights.emplace_back(row, fromTris(bestTri, 2), static_cast<float>(bestQ));
    }
    SparseMatrix<float> matrix(to.rows(), from.rows());
    matrix.setFromTriplets(weights.begin(), weights.end());
    result.map = std::make_unique<FiffSparseMatrix>(std::move(matrix));
    return result;
}

//=============================================================================================================

std::optional<MNEMorphMap> MNEMorphMap::read(const QString& path, const QString& fromSubj, const QString& toSubj, int hemi)
{
    QFile file(path);
    FiffStream::SPtr stream(new FiffStream(&file));
    if (hemi < 0 || hemi > 1 || !stream->open()) {
        return std::nullopt;
    }
    FiffTag::UPtr tag;
    for (const FiffDirNode::SPtr& node : stream->dirtree()->dir_tree_find(FIFFB_MNE_MORPH_MAP)) {
        if (!node->find_tag(stream, FIFF_MNE_HEMI, tag) || *tag->toInt() != kHemiKind[hemi]) {
            continue;
        }
        if (!node->find_tag(stream, FIFF_MNE_MORPH_MAP_FROM, tag) || tag->toString() != fromSubj) {
            continue;
        }
        if (!node->find_tag(stream, FIFF_MNE_MORPH_MAP_TO, tag) || tag->toString() != toSubj) {
            continue;
        }
        if (!node->find_tag(stream, FIFF_MNE_MORPH_MAP, tag)) {
            break;
        }
        MNEMorphMap result;
        result.map = FiffSparseMatrix::fiff_get_float_sparse_matrix(tag);
        if (!result.map) {
            break;
        }
        result.hemi = hemi;
        result.from_subj = fromSubj;
        result.to_subj = toSubj;
        return result;
    }
    qWarning("MNEMorphMap::read - %s hemisphere morph map from %s to %s not found in %s",
             hemi == 0 ? "left" : "right", qPrintable(fromSubj), qPrintable(toSubj), qPrintable(path));
    return std::nullopt;
}

//=============================================================================================================

bool MNEMorphMap::write(const QString& path, const QList<const MNEMorphMap*>& maps)
{
    QFile file(path);
    FiffStream::SPtr stream = FiffStream::start_file(file);
    if (!stream) {
        return false;
    }
    for (const MNEMorphMap* morph : maps) {
        if (!morph || !morph->map || morph->hemi < 0 || morph->hemi > 1) {
            qWarning("MNEMorphMap::write - Incomplete morph map");
            return false;
        }
        stream->start_block(FIFFB_MNE_MORPH_MAP);
        stream->write_string(FIFF_MNE_MORPH_MAP_FROM, morph->from_subj);
        stream->write_string(FIFF_MNE_MORPH_MAP_TO, morph->to_subj);
        stream->write_int(FIFF_MNE_HEMI, &kHemiKind[morph->hemi]);
        stream->write_float_sparse_rcs(FIFF_MNE_MORPH_MAP, morph->map->eigen());
        stream->end_block(FIFFB_MNE_MORPH_MAP);
    }
    stream->end_file();
    return true;
}

//=============================================================================================================

SparseMatrix<double> MNEMorphMap::toEigen() const
{
    return map ? map->eigen().cast<double>() : SparseMatrix<double>();
}
