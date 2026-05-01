//=============================================================================================================
/**
 * @file     fs_label_utils.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    FsLabelUtils class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_label_utils.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
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

    // If surface is empty, treat each vertex as its own component
    if (surface.isEmpty()) {
        for (int i = 0; i < label.vertices.size(); ++i) {
            FsLabel comp;
            comp.hemi = label.hemi;
            comp.name = QString("%1_part%2").arg(label.name).arg(i);
            comp.vertices.resize(1);
            comp.vertices[0] = label.vertices[i];
            comp.pos = label.pos.row(i);
            comp.values = VectorXd::Ones(1);
            if (i < label.values.size())
                comp.values[0] = label.values[i];
            components.append(comp);
        }
        return components;
    }

    const int nVerts = static_cast<int>(surface.rr().rows());
    QList<QSet<int>> adj = buildAdjacency(surface.tris(), nVerts);

    // Build set of label vertices for fast lookup
    QSet<int> labelSet;
    for (int i = 0; i < label.vertices.size(); ++i)
        labelSet.insert(label.vertices[i]);

    QSet<int> visited;
    int compIdx = 0;

    for (int i = 0; i < label.vertices.size(); ++i) {
        int seed = label.vertices[i];
        if (visited.contains(seed))
            continue;

        // BFS from seed, restricted to label vertices
        QQueue<int> queue;
        queue.enqueue(seed);
        visited.insert(seed);
        QList<int> component;

        while (!queue.isEmpty()) {
            int v = queue.dequeue();
            component.append(v);

            if (v >= 0 && v < nVerts) {
                for (int neighbor : adj[v]) {
                    if (labelSet.contains(neighbor) && !visited.contains(neighbor)) {
                        visited.insert(neighbor);
                        queue.enqueue(neighbor);
                    }
                }
            }
        }

        std::sort(component.begin(), component.end());

        FsLabel comp;
        comp.hemi = label.hemi;
        comp.name = QString("%1_part%2").arg(label.name).arg(compIdx++);
        comp.vertices.resize(component.size());
        comp.pos.resize(component.size(), 3);
        comp.values = VectorXd::Ones(component.size());

        for (int j = 0; j < component.size(); ++j) {
            comp.vertices[j] = component[j];
            if (component[j] < nVerts)
                comp.pos.row(j) = surface.rr().row(component[j]);
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
