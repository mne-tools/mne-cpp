//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2002
 * @brief    Add patch information to a source space using Dijkstra geodesic distances.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_source_spaces.h>
#include <mne/mne/mne_source_space.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <map>
#include <queue>
#include <vector>
#include <set>
#include <cmath>
#include <limits>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
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
        int v[3] = {tris(t, 0), tris(t, 1), tris(t, 2)};
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

int main(int argc, char* argv[])
{
    CommandLineParser parser("Add patch information to a source space.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"src"}, "Input source space FIFF file.", "file", {}});
    parser.add_option({{"out"}, "Output source space FIFF file with patch info.", "file", {}});
    parser.process(argc, argv, "mne_add_patch_info", PROGRAM_VERSION);

    const std::string srcFile = parser.value("src");
    const std::string outFile = parser.value("out");

    if (srcFile.empty()) {
        Log::critical("--src is required.");
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        return 1;
    }

    // Read source space
    auto read = MNESourceSpaces::read(path_from_utf8(srcFile));
    if (!read) {
        Log::critical("{}", read.error().what());
        return 1;
    }
    MNESourceSpaces& src = *read;

    Log::info("Read source space: {} hemispheres", static_cast<int>(src.size()));

    // Process each hemisphere
    for (int h = 0; h < src.size(); ++h) {
        MNESourceSpace& sp = src[h];
        Log::info("\nHemisphere {}: {} vertices, {} active, {} triangles", h + 1, sp.np, sp.nuse, sp.ntri);

        if (sp.ntri == 0 || sp.np == 0) {
            Log::info("  Skipping (no triangulation)");
            continue;
        }

        // Build adjacency
        auto adj = buildAdjacency(sp.rr, sp.itris);

        // Collect active (in-use) vertices
        std::vector<int> activeList;
        for (int i = 0; i < sp.np; ++i) {
            if (sp.inuse[i])
                activeList.push_back(i);
        }

        VectorXi sourceVerts(std::ssize(activeList));
        for (int i = 0; i < std::ssize(activeList); ++i)
            sourceVerts(i) = activeList[static_cast<std::size_t>(i)];

        Log::info("  Running multi-source Dijkstra with {} source vertices...", static_cast<int>(sourceVerts.size()));

        VectorXi nearest;
        VectorXf dist;
        multiSourceDijkstra(adj, sourceVerts, sp.np, nearest, dist);

        // Count patch sizes
        std::map<int, int> patchSizes;
        for (int i = 0; i < sp.np; ++i) {
            if (nearest(i) >= 0)
                patchSizes[nearest(i)]++;
        }

        // Store patch info via setNearestData
        VectorXd distD = dist.cast<double>();
        sp.setNearestData(nearest, distD);

        int minPatch = sp.np, maxPatch = 0;
        double avgPatch = 0;
        for (const auto& [vertex, size] : patchSizes) {
            if (size < minPatch)
                minPatch = size;
            if (size > maxPatch)
                maxPatch = size;
            avgPatch += size;
        }
        if (!patchSizes.empty())
            avgPatch /= static_cast<double>(patchSizes.size());

        Log::info("  Patch info: {} patches, size range {}..{} (avg {:.1f})", static_cast<int>(patchSizes.size()), minPatch, maxPatch, avgPatch);
    }

    // Write output
    Log::info("\nWriting source space with patch info to: {}", outFile);

    auto outStream = FiffStream::start_file(path_from_utf8(outFile));
    if (!outStream) {
        Log::critical("Cannot open output file: {}", outFile);
        return 1;
    }
    src.write_to_stream(*outStream);
    outStream->end_file();

    Log::info("Done.");
    return 0;
}
