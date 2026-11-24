//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Smooth source estimates on a cortical surface using iterative Laplacian averaging.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_source_spaces.h>
#include <mne/mne/mne_source_space.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fs/fs_surface.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <fstream>
#include <print>
#include <set>
#include <string>
#include <utility>
#include <vector>

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
 * Build an adjacency-based smoothing operator from surface triangulation.
 * Returns a sparse matrix S where S(i,j) = 1/N_i for each neighbor j of i,
 * and S(i,i) = 0 (pure averaging of neighbors).
 */
static SparseMatrix<double> buildSmoothingOperator(const MatrixX3i& tris, int nVert)
{
    // Count neighbors for each vertex
    VectorXi degree = VectorXi::Zero(nVert);
    std::vector<std::set<int>> neighbors(nVert);

    for (int t = 0; t < tris.rows(); ++t) {
        int v0 = tris(t, 0), v1 = tris(t, 1), v2 = tris(t, 2);
        neighbors[v0].insert(v1);
        neighbors[v0].insert(v2);
        neighbors[v1].insert(v0);
        neighbors[v1].insert(v2);
        neighbors[v2].insert(v0);
        neighbors[v2].insert(v1);
    }

    // Build sparse triplets
    typedef Triplet<double> T;
    std::vector<T> triplets;
    triplets.reserve(static_cast<std::size_t>(nVert) * 7); // ~6 neighbors average on icosphere

    for (int i = 0; i < nVert; ++i) {
        int deg = static_cast<int>(neighbors[i].size());
        if (deg > 0) {
            double w = 1.0 / deg;
            for (int j : neighbors[i])
                triplets.push_back(T(i, j, w));
        } else {
            // Isolated vertex: keep itself
            triplets.push_back(T(i, i, 1.0));
        }
    }

    SparseMatrix<double> S(nVert, nVert);
    S.setFromTriplets(triplets.begin(), triplets.end());
    return S;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Smooth source estimate data on a cortical surface.");
    parser.add_help_option();
    parser.add_version_option();

    parser.add_option({{"src"}, "Source space FIFF file (for surface connectivity).", "file", {}});
    parser.add_option({{"surf"}, "FreeSurfer surface file (alternative to --src).", "file", {}});
    parser.add_option({{"stc"}, "Input STC file (text format: vertex value [per time]).", "file", {}});
    parser.add_option({{"out"}, "Output smoothed STC file.", "file", {}});
    parser.add_option({{"smooth"}, "Number of smoothing iterations.", "n", "5"});

    parser.process(argc, argv, "mne_smooth", PROGRAM_VERSION);

    const std::string srcFile = parser.value("src");
    const std::string surfFile = parser.value("surf");
    const std::string stcFile = parser.value("stc");
    const std::string outFile = parser.value("out");
    const int nSmooth = to_int(parser.value("smooth")).value_or(0);

    if (srcFile.empty() && surfFile.empty()) {
        Log::critical("Either --src or --surf is required.");
        std::print("{}", parser.help_text("mne_smooth"));
        return 1;
    }
    if (stcFile.empty()) {
        Log::critical("--stc is required.");
        std::print("{}", parser.help_text("mne_smooth"));
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        std::print("{}", parser.help_text("mne_smooth"));
        return 1;
    }

    // Get surface triangulation
    MatrixX3i tris;
    int nVert = 0;

    if (!surfFile.empty()) {
        FsSurface surface;
        if (!FsSurface::read(path_from_utf8(surfFile), surface)) {
            Log::critical("Cannot read surface: {}", surfFile);
            return 1;
        }
        tris = surface.tris();
        nVert = static_cast<int>(surface.rr().rows());
        Log::info("Read surface: {} vertices, {} triangles", nVert, static_cast<int>(tris.rows()));
    } else {
        // Read source space
        auto read = MNESourceSpaces::read(path_from_utf8(srcFile));
        if (!read) {
            Log::critical("{}", read.error().what());
            return 1;
        }
        const MNESourceSpaces& srcSpaces = *read;
        // Use first hemisphere's triangulation
        tris = srcSpaces[0].itris;
        nVert = srcSpaces[0].np;
        Log::info("Read source space: {} vertices, {} triangles", nVert, static_cast<int>(tris.rows()));
    }

    // Build smoothing operator
    SparseMatrix<double> S = buildSmoothingOperator(tris, nVert);
    Log::info("Built smoothing operator: {} x {}", static_cast<int>(S.rows()), static_cast<int>(S.cols()));

    // Read STC data (text format: one value per line, or vertex value pairs)
    std::ifstream stcIn(path_from_utf8(stcFile));
    if (!stcIn) {
        Log::critical("Cannot open STC file: {}", stcFile);
        return 1;
    }

    std::vector<std::pair<int, double>> dataList;
    std::string rawLine;
    while (std::getline(stcIn, rawLine)) {
        const std::string_view line = trimmed(rawLine);
        if (line.empty() || line.starts_with('#'))
            continue;
        const std::vector<std::string_view> parts = split_whitespace(line);
        if (parts.size() >= 2) {
            dataList.emplace_back(to_int(parts[0]).value_or(0), to_double(parts[1]).value_or(0.0));
        } else if (parts.size() == 1) {
            dataList.emplace_back(static_cast<int>(dataList.size()), to_double(parts[0]).value_or(0.0));
        }
    }
    stcIn.close();

    // Build data vector
    VectorXd data = VectorXd::Zero(nVert);
    for (const auto& pair : dataList) {
        if (pair.first >= 0 && pair.first < nVert)
            data(pair.first) = pair.second;
    }
    Log::info("Read {} data values", static_cast<int>(dataList.size()));

    // Apply iterative smoothing
    VectorXd smoothed = data;
    for (int iter = 0; iter < nSmooth; ++iter) {
        smoothed = S * smoothed;
    }
    Log::info("Applied {} iterations of Laplacian smoothing", nSmooth);

    // Write output
    std::ofstream out(path_from_utf8(outFile));
    if (!out) {
        Log::critical("Cannot write output file: {}", outFile);
        return 1;
    }
    out << "# Smoothed source estimate (" << nSmooth << " iterations)\n";
    for (int i = 0; i < nVert; ++i) {
        if (smoothed(i) != 0.0)
            out << std::format("{} {:.10g}\n", i, smoothed(i));
    }
    out.close();
    Log::info("Written smoothed data to: {}", outFile);

    return 0;
}
