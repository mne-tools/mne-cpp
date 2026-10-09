//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fs_label_utils.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of @ref FSLIB::FsLabelUtils: grow / split labels on the surface mesh and convert between source-estimate matrices and labels.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_label_utils.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QHash>
#include <QQueue>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QList<QSet<int>> FsLabelUtils::buildAdjacency(const MatrixX3i& tris, int nVerts)
{
    QList<QSet<int>> adj(nVerts);

    for (int t = 0; t < tris.rows(); ++t) {
        int v0 = tris(t, 0);
        int v1 = tris(t, 1);
        int v2 = tris(t, 2);

        if (v0 >= 0 && v0 < nVerts && v1 >= 0 && v1 < nVerts && v2 >= 0 && v2 < nVerts) {
            adj[v0].insert(v1);
            adj[v0].insert(v2);
            adj[v1].insert(v0);
            adj[v1].insert(v2);
            adj[v2].insert(v0);
            adj[v2].insert(v1);
        }
    }

    return adj;
}

//=============================================================================================================

FsLabel FsLabelUtils::growLabel(const FsLabel& label,
                                const FsSurface& surface,
                                int nSteps)
{
    if (label.isEmpty() || surface.isEmpty() || nSteps <= 0)
        return label;

    const int nVerts = static_cast<int>(surface.rr().rows());
    QList<QSet<int>> adj = buildAdjacency(surface.tris(), nVerts);

    // Start with seed vertices
    QSet<int> current;
    for (int i = 0; i < label.vertices.size(); ++i)
        current.insert(label.vertices[i]);

    QSet<int> allVerts = current;

    // BFS expansion
    for (int step = 0; step < nSteps; ++step) {
        QSet<int> frontier;
        for (int v : current) {
            if (v >= 0 && v < nVerts) {
                for (int neighbor : adj[v]) {
                    if (!allVerts.contains(neighbor))
                        frontier.insert(neighbor);
                }
            }
        }
        allVerts.unite(frontier);
        current = frontier;

        if (frontier.isEmpty())
            break;
    }

    // Build result label
    QList<int> sortedVerts(allVerts.begin(), allVerts.end());
    std::sort(sortedVerts.begin(), sortedVerts.end());

    FsLabel result;
    result.hemi = label.hemi;
    result.name = label.name + "_grown";
    result.vertices.resize(sortedVerts.size());
    result.pos.resize(sortedVerts.size(), 3);
    result.values = VectorXd::Ones(sortedVerts.size());

    for (int i = 0; i < sortedVerts.size(); ++i) {
        result.vertices[i] = sortedVerts[i];
        if (sortedVerts[i] < nVerts)
            result.pos.row(i) = surface.rr().row(sortedVerts[i]);
    }

    return result;
}

//=============================================================================================================

QList<FsLabel> FsLabelUtils::splitLabel(const FsLabel& label,
                                        const FsSurface& surface)
{
    QList<FsLabel> components;

    if (label.isEmpty())
        return components;

    // Label row of each vertex: the parts keep the label's positions and values
    QHash<int, int> rowOf;
    for (int i = 0; i < label.vertices.size(); ++i)
        rowOf.insert(label.vertices[i], i);

    // Connected components over the surface edges; without a surface every vertex stands alone
    const int nVerts = surface.isEmpty() ? 0 : static_cast<int>(surface.rr().rows());
    const QList<QSet<int>> adj = surface.isEmpty() ? QList<QSet<int>>() : buildAdjacency(surface.tris(), nVerts);
    QSet<int> visited;
    QList<QList<int>> parts;
    for (int i = 0; i < label.vertices.size(); ++i) {
        const int seed = label.vertices[i];
        if (visited.contains(seed))
            continue;
        QQueue<int> queue;
        queue.enqueue(seed);
        visited.insert(seed);
        QList<int> part;
        while (!queue.isEmpty()) {
            const int v = queue.dequeue();
            part.append(v);
            if (v >= 0 && v < nVerts) {
                for (int neighbor : adj[v]) {
                    if (rowOf.contains(neighbor) && !visited.contains(neighbor)) {
                        visited.insert(neighbor);
                        queue.enqueue(neighbor);
                    }
                }
            }
        }
        std::sort(part.begin(), part.end());
        parts.append(part);
    }

    // As mne Label.split("contiguous"): largest part first, named <name>_div<i>[-lh|-rh]
    std::stable_sort(parts.begin(), parts.end(), [](const QList<int>& a, const QList<int>& b) { return a.size() > b.size(); });
    const bool hemiSuffix = label.name.endsWith(QStringLiteral("lh")) || label.name.endsWith(QStringLiteral("rh"));
    const QString base = hemiSuffix ? label.name.left(label.name.size() - 3) : label.name;
    const QString ext = hemiSuffix ? label.name.right(3) : QString();

    for (int p = 0; p < parts.size(); ++p) {
        const QList<int>& part = parts[p];
        FsLabel comp;
        comp.hemi = label.hemi;
        comp.name = QStringLiteral("%1_div%2%3").arg(base).arg(p + 1).arg(ext);
        comp.vertices.resize(part.size());
        comp.pos.resize(part.size(), 3);
        comp.values.resize(part.size());
        for (int j = 0; j < part.size(); ++j) {
            const int row = rowOf.value(part[j]);
            comp.vertices[j] = part[j];
            comp.pos.row(j) = label.pos.row(row);
            comp.values[j] = row < label.values.size() ? label.values[row] : 1.0;
        }
        components.append(comp);
    }

    return components;
}

//=============================================================================================================

QList<FsLabel> FsLabelUtils::stcToLabel(const MatrixXd& stcData,
                                        const VectorXi& vertices,
                                        const FsSurface& surface,
                                        double dThreshold,
                                        int iHemi)
{
    QList<FsLabel> labels;

    if (stcData.size() == 0 || vertices.size() == 0 || surface.isEmpty())
        return labels;

    // Find vertices above threshold (max absolute value across time)
    VectorXd maxAbs = stcData.cwiseAbs().rowwise().maxCoeff();

    QSet<int> aboveThresh;
    for (int i = 0; i < vertices.size(); ++i) {
        if (maxAbs[i] > dThreshold)
            aboveThresh.insert(vertices[i]);
    }

    if (aboveThresh.isEmpty())
        return labels;

    // Build a label from above-threshold vertices and split into components
    FsLabel fullLabel;
    fullLabel.hemi = iHemi;
    fullLabel.name = "stc_label";

    QList<int> sortedVerts(aboveThresh.begin(), aboveThresh.end());
    std::sort(sortedVerts.begin(), sortedVerts.end());

    const int nSurfVerts = static_cast<int>(surface.rr().rows());
    fullLabel.vertices.resize(sortedVerts.size());
    fullLabel.pos.resize(sortedVerts.size(), 3);
    fullLabel.values.resize(sortedVerts.size());

    for (int i = 0; i < sortedVerts.size(); ++i) {
        fullLabel.vertices[i] = sortedVerts[i];
        if (sortedVerts[i] < nSurfVerts)
            fullLabel.pos.row(i) = surface.rr().row(sortedVerts[i]);

        // Find the vertex's row in the STC
        for (int j = 0; j < vertices.size(); ++j) {
            if (vertices[j] == sortedVerts[i]) {
                fullLabel.values[i] = maxAbs[j];
                break;
            }
        }
    }

    // Split into connected components
    labels = splitLabel(fullLabel, surface);

    return labels;
}

//=============================================================================================================

MatrixXd FsLabelUtils::labelsToStc(const QList<FsLabel>& labels,
                                   const VectorXi& stcVertices,
                                   int nTimes)
{
    const int nVerts = static_cast<int>(stcVertices.size());
    MatrixXd mask = MatrixXd::Zero(nVerts, nTimes);

    // Build vertex-to-row map
    QMap<int, int> vertToRow;
    for (int i = 0; i < nVerts; ++i)
        vertToRow.insert(stcVertices[i], i);

    for (const auto& label : labels) {
        for (int i = 0; i < label.vertices.size(); ++i) {
            auto it = vertToRow.find(label.vertices[i]);
            if (it != vertToRow.end()) {
                mask.row(it.value()).setOnes();
            }
        }
    }

    return mask;
}
