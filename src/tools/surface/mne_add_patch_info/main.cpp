//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Add patch information to a source space using Dijkstra geodesic distances.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_source_spaces.h>
#include <mne/mne_source_space.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_constants.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <queue>
#include <vector>
#include <set>
#include <cmath>
#include <limits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
/**
 * Build adjacency list from surface triangulation.
 * Returns a vector of vectors of (neighbor_index, edge_length) pairs.
 */
static std::vector<std::vector<std::pair<int, float>>> buildAdjacency(
    const MatrixX3f& rr, const MatrixX3i& tris)
{
    int nVert = rr.rows();
    std::vector<std::vector<std::pair<int, float>>> adj(nVert);

    for (int t = 0; t < tris.rows(); ++t) {
        int v[3] = { tris(t, 0), tris(t, 1), tris(t, 2) };
        for (int e = 0; e < 3; ++e) {
            int a = v[e], b = v[(e + 1) % 3];
            float d = (rr.row(a) - rr.row(b)).norm();
            adj[a].push_back(std::make_pair(b, d));
            adj[b].push_back(std::make_pair(a, d));
        }
    }

    // Remove duplicate edges
    for (int i = 0; i < nVert; ++i) {
        std::sort(adj[i].begin(), adj[i].end());
        adj[i].erase(std::unique(adj[i].begin(), adj[i].end()), adj[i].end());
    }

    return adj;
}

//=============================================================================================================
/**
 * Run Dijkstra from a single source vertex, returning the nearest source vertex
 * (from sourceVerts) for each vertex on the surface.
 * This is a multi-source Dijkstra: all source vertices start with distance 0.
 */
static void multiSourceDijkstra(
    const std::vector<std::vector<std::pair<int, float>>>& adj,
    const VectorXi& sourceVerts,
    int nVert,
    VectorXi& nearest,
    VectorXf& dist)
{
    nearest = VectorXi::Constant(nVert, -1);
    dist = VectorXf::Constant(nVert, std::numeric_limits<float>::max());

    // Priority queue: (distance, vertex)
    typedef std::pair<float, int> PQEntry;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> pq;

    for (int i = 0; i < sourceVerts.size(); ++i) {
        int v = sourceVerts(i);
        if (v >= 0 && v < nVert) {
            dist(v) = 0.0f;
            nearest(v) = v;
            pq.push(std::make_pair(0.0f, v));
        }
    }

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (d > dist(u))
            continue;

        for (const auto& [v, w] : adj[u]) {
            float newDist = d + w;
            if (newDist < dist(v)) {
                dist(v) = newDist;
                nearest(v) = nearest(u);
                pq.push(std::make_pair(newDist, v));
            }
        }
    }
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_add_patch_info");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Add patch information to a source space.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption srcOpt("src", "Input source space FIFF file.", "file");
    parser.addOption(srcOpt);

    QCommandLineOption outOpt("out", "Output source space FIFF file with patch info.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString srcFile = parser.value(srcOpt);
    QString outFile = parser.value(outOpt);

    if (srcFile.isEmpty()) { qCritical("--src is required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }

    // Read source space
    QFile file(srcFile);
    file.open(QIODevice::ReadOnly);
    FiffStream::SPtr stream(new FiffStream(&file));
    MNESourceSpaces src;
    if (!MNESourceSpaces::readFromStream(stream, true, src) || src.size() == 0) {
        qCritical("Cannot read source space: %s", qPrintable(srcFile));
        return 1;
    }

    printf("Read source space: %d hemispheres\n", (int)src.size());

    // Process each hemisphere
    for (int h = 0; h < src.size(); ++h) {
        MNESourceSpace& sp = src[h];
        printf("\nHemisphere %d: %d vertices, %d active, %d triangles\n",
               h + 1, sp.np, sp.nuse, sp.ntri);

        if (sp.ntri == 0 || sp.np == 0) {
            printf("  Skipping (no triangulation)\n");
            continue;
        }

        // Build adjacency
        auto adj = buildAdjacency(sp.rr, sp.itris);

        // Collect active (in-use) vertices
        QList<int> activeList;
        for (int i = 0; i < sp.np; ++i) {
            if (sp.inuse[i])
                activeList.append(i);
        }

        VectorXi sourceVerts(activeList.size());
        for (int i = 0; i < activeList.size(); ++i)
            sourceVerts(i) = activeList[i];

        printf("  Running multi-source Dijkstra with %d source vertices...\n", (int)sourceVerts.size());

        VectorXi nearest;
        VectorXf dist;
        multiSourceDijkstra(adj, sourceVerts, sp.np, nearest, dist);

        // Count patch sizes
        QMap<int, int> patchSizes;
        for (int i = 0; i < sp.np; ++i) {
            if (nearest(i) >= 0)
                patchSizes[nearest(i)]++;
        }

        // Store patch info via setNearestData
        VectorXd distD = dist.cast<double>();
        sp.setNearestData(nearest, distD);

        int minPatch = sp.np, maxPatch = 0;
        double avgPatch = 0;
        for (auto it = patchSizes.begin(); it != patchSizes.end(); ++it) {
            if (it.value() < minPatch) minPatch = it.value();
            if (it.value() > maxPatch) maxPatch = it.value();
            avgPatch += it.value();
        }
        if (!patchSizes.isEmpty())
            avgPatch /= patchSizes.size();

        printf("  Patch info: %d patches, size range %d..%d (avg %.1f)\n",
               (int)patchSizes.size(), minPatch, maxPatch, avgPatch);
    }

    // Write output
    printf("\nWriting source space with patch info to: %s\n", qPrintable(outFile));

    QFile outF(outFile);
    outF.open(QIODevice::WriteOnly);
    FiffStream::SPtr outStream(new FiffStream(&outF));
    src.writeToStream(outStream.data());
    outF.close();

    printf("Done.\n");
    return 0;
}
