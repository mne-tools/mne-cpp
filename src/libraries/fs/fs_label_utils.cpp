//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fs_label_utils.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of @ref mne::FsLabelUtils: grow / split labels on the surface mesh and convert between source-estimate matrices and labels.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_label_utils.h"

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <numeric>
#include <unordered_map>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

FsLabel makeLabel(const std::vector<int>& verts, int hemi, std::string name)
{
    FsLabel label;
    label.hemi = hemi;
    label.name = std::move(name);
    const auto n = static_cast<Index>(verts.size());
    label.vertices = Map<const VectorXi>(verts.data(), n);
    label.pos = MatrixX3f::Zero(n, 3);
    label.values = VectorXd::Ones(n);
    return label;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::vector<std::vector<int>> FsLabelUtils::buildAdjacency(const MatrixX3i& tris, int nVerts)
{
    std::vector<std::vector<int>> adj(static_cast<std::size_t>(std::max(nVerts, 0)));
    for (Index t = 0; t < tris.rows(); ++t) {
        const int v[3] = {tris(t, 0), tris(t, 1), tris(t, 2)};
        if (std::ranges::all_of(v, [nVerts](int x) { return x >= 0 && x < nVerts; })) {
            for (int a = 0; a < 3; ++a) {
                auto& neighbours = adj[static_cast<std::size_t>(v[a])];
                neighbours.push_back(v[(a + 1) % 3]);
                neighbours.push_back(v[(a + 2) % 3]);
            }
        }
    }
    for (auto& neighbours : adj) {
        std::ranges::sort(neighbours);
        neighbours.erase(std::unique(neighbours.begin(), neighbours.end()), neighbours.end());
    }
    return adj;
}

//=============================================================================================================

FsLabel FsLabelUtils::growLabel(const FsLabel& label,
                                const FsSurface& surface,
                                int nSteps)
{
    if (label.empty() || surface.empty() || nSteps <= 0)
        return label;

    const int nVerts = static_cast<int>(surface.rr().rows());
    const auto adj = buildAdjacency(surface.tris(), nVerts);

    // Breadth-first by rings; out-of-range seed vertices are kept but not expanded
    std::vector<char> inLabel(static_cast<std::size_t>(nVerts), 0);
    std::vector<int> all(label.vertices.data(), label.vertices.data() + label.vertices.size());
    std::vector<int> current;
    for (const int v : all) {
        if (v >= 0 && v < nVerts && !inLabel[static_cast<std::size_t>(v)]) {
            inLabel[static_cast<std::size_t>(v)] = 1;
            current.push_back(v);
        }
    }
    for (int step = 0; step < nSteps && !current.empty(); ++step) {
        std::vector<int> frontier;
        for (const int v : current) {
            for (const int neighbour : adj[static_cast<std::size_t>(v)]) {
                if (!inLabel[static_cast<std::size_t>(neighbour)]) {
                    inLabel[static_cast<std::size_t>(neighbour)] = 1;
                    frontier.push_back(neighbour);
                }
            }
        }
        all.insert(all.end(), frontier.begin(), frontier.end());
        current = std::move(frontier);
    }
    std::ranges::sort(all);
    all.erase(std::unique(all.begin(), all.end()), all.end());

    FsLabel result = makeLabel(all, label.hemi, label.name + "_grown");
    for (Index i = 0; i < result.vertices.size(); ++i) {
        if (result.vertices[i] >= 0 && result.vertices[i] < nVerts)
            result.pos.row(i) = surface.rr().row(result.vertices[i]);
    }
    return result;
}

//=============================================================================================================

FsLabel FsLabelUtils::growCircularLabel(const FsSurface& surface,
                                        std::span<const int> seeds,
                                        double extentMm,
                                        int hemi,
                                        const std::string& name)
{
    // Adapted from mne.label._verts_within_dist / _grow_labels (MNE-Python, BSD-3-Clause).
    const MatrixX3f& rr = surface.rr();
    const int nVerts = static_cast<int>(rr.rows());
    const auto adj = buildAdjacency(surface.tris(), nVerts);

    // NaN marks a vertex not reached yet; neighbours are visited in ascending order, as in mne's CSR graph
    std::vector<double> dist(static_cast<std::size_t>(std::max(nVerts, 0)), std::numeric_limits<double>::quiet_NaN());
    std::vector<int> reached;
    std::vector<int> addedLast;
    for (const int seed : seeds) {
        if (seed >= 0 && seed < nVerts && std::isnan(dist[static_cast<std::size_t>(seed)])) {
            dist[static_cast<std::size_t>(seed)] = 0.0;
            addedLast.push_back(seed);
        }
    }
    reached = addedLast;
    while (!addedLast.empty()) {
        std::vector<int> added;
        for (const int i : addedLast) {
            const double vDist = dist[static_cast<std::size_t>(i)];
            for (const int j : adj[static_cast<std::size_t>(i)]) {
                const double nDist = vDist + 1000.0 * (rr.row(j) - rr.row(i)).cast<double>().norm();
                double& known = dist[static_cast<std::size_t>(j)];
                if (!std::isnan(known)) {
                    known = std::min(known, nDist);
                } else if (nDist <= extentMm) {
                    known = nDist;
                    added.push_back(j);
                }
            }
        }
        reached.insert(reached.end(), added.begin(), added.end());
        addedLast = std::move(added);
    }
    std::ranges::sort(reached);

    const char* suffix = hemi == 0 ? "lh" : "rh";
    FsLabel label = makeLabel(reached, hemi, name.ends_with(suffix) ? name : std::format("{}-{}", name, suffix));
    for (Index k = 0; k < label.vertices.size(); ++k) {
        label.pos.row(k) = rr.row(label.vertices[k]);
        label.values[k] = std::trunc(dist[static_cast<std::size_t>(label.vertices[k])]);
    }
    return label;
}

//=============================================================================================================

std::vector<FsLabel> FsLabelUtils::splitLabel(const FsLabel& label,
                                              const FsSurface& surface)
{
    std::vector<FsLabel> components;
    if (label.empty())
        return components;

    // Label row of each vertex: the parts keep the label's positions and values
    std::unordered_map<int, Index> rowOf;
    rowOf.reserve(static_cast<std::size_t>(label.vertices.size()));
    for (Index i = 0; i < label.vertices.size(); ++i)
        rowOf.insert_or_assign(label.vertices[i], i);

    // Connected components over the surface edges; without a surface every vertex stands alone
    const int nVerts = surface.empty() ? 0 : static_cast<int>(surface.rr().rows());
    const auto adj = surface.empty() ? std::vector<std::vector<int>>() : buildAdjacency(surface.tris(), nVerts);
    std::vector<char> visited(static_cast<std::size_t>(label.vertices.size()), 0);
    std::vector<std::vector<int>> parts;
    for (Index i = 0; i < label.vertices.size(); ++i) {
        const int seed = label.vertices[i];
        char& seen = visited[static_cast<std::size_t>(rowOf[seed])];
        if (seen)
            continue;
        seen = 1;
        std::vector<int> part{seed};
        for (std::size_t head = 0; head < part.size(); ++head) {
            const int v = part[head];
            if (v < 0 || v >= nVerts)
                continue;
            for (const int neighbour : adj[static_cast<std::size_t>(v)]) {
                const auto it = rowOf.find(neighbour);
                if (it != rowOf.end() && !visited[static_cast<std::size_t>(it->second)]) {
                    visited[static_cast<std::size_t>(it->second)] = 1;
                    part.push_back(neighbour);
                }
            }
        }
        std::ranges::sort(part);
        parts.push_back(std::move(part));
    }

    // As mne Label.split("contiguous"): largest part first, named <name>_div<i>[-lh|-rh]
    std::ranges::stable_sort(parts, std::ranges::greater{}, &std::vector<int>::size);
    const bool hemiSuffix = label.name.ends_with("lh") || label.name.ends_with("rh");
    const std::string_view base = std::string_view(label.name).substr(0, hemiSuffix ? label.name.size() - 3 : label.name.size());
    const std::string_view ext = hemiSuffix ? std::string_view(label.name).substr(label.name.size() - 3) : std::string_view();

    components.reserve(parts.size());
    for (std::size_t p = 0; p < parts.size(); ++p) {
        FsLabel comp = makeLabel(parts[p], label.hemi, std::format("{}_div{}{}", base, p + 1, ext));
        for (Index j = 0; j < comp.vertices.size(); ++j) {
            const Index row = rowOf[comp.vertices[j]];
            comp.pos.row(j) = label.pos.row(row);
            comp.values[j] = row < label.values.size() ? label.values[row] : 1.0;
        }
        components.push_back(std::move(comp));
    }

    return components;
}

//=============================================================================================================

std::vector<FsLabel> FsLabelUtils::stcToLabel(const MatrixXd& stcData,
                                              const VectorXi& vertices,
                                              const FsSurface& surface,
                                              double dThreshold,
                                              int iHemi)
{
    if (stcData.size() == 0 || vertices.size() == 0 || surface.empty())
        return {};

    // Vertices whose maximum absolute value over time exceeds the threshold, with their first STC row
    const VectorXd maxAbs = stcData.cwiseAbs().rowwise().maxCoeff();
    std::unordered_map<int, Index> firstRow;
    for (Index i = 0; i < vertices.size(); ++i) {
        if (maxAbs[i] > dThreshold)
            firstRow.try_emplace(vertices[i], i);
    }
    if (firstRow.empty())
        return {};

    std::vector<int> sortedVerts;
    sortedVerts.reserve(firstRow.size());
    for (const auto& entry : firstRow)
        sortedVerts.push_back(entry.first);
    std::ranges::sort(sortedVerts);

    const int nSurfVerts = static_cast<int>(surface.rr().rows());
    FsLabel fullLabel = makeLabel(sortedVerts, iHemi, "stc_label");
    for (Index i = 0; i < fullLabel.vertices.size(); ++i) {
        const int v = fullLabel.vertices[i];
        if (v >= 0 && v < nSurfVerts)
            fullLabel.pos.row(i) = surface.rr().row(v);
        fullLabel.values[i] = maxAbs[firstRow[v]];
    }

    // Split into connected components
    return splitLabel(fullLabel, surface);
}

//=============================================================================================================

MatrixXd FsLabelUtils::labelsToStc(std::span<const FsLabel> labels,
                                   const VectorXi& stcVertices,
                                   int nTimes)
{
    const Index nVerts = stcVertices.size();
    MatrixXd mask = MatrixXd::Zero(nVerts, nTimes);

    std::unordered_map<int, Index> vertToRow;
    vertToRow.reserve(static_cast<std::size_t>(nVerts));
    for (Index i = 0; i < nVerts; ++i)
        vertToRow.insert_or_assign(stcVertices[i], i);

    for (const FsLabel& label : labels) {
        for (Index i = 0; i < label.vertices.size(); ++i) {
            const auto it = vertToRow.find(label.vertices[i]);
            if (it != vertToRow.end())
                mask.row(it->second).setOnes();
        }
    }

    return mask;
}
