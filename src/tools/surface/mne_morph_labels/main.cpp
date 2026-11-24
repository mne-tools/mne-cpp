//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2002
 * @brief    Morph label files between subjects using sphere-registered surfaces.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fs/fs_surface.h>
#include <mne/fs/fs_label.h>
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

#include <cmath>
#include <cstdlib>
#include <format>
#include <fstream>
#include <limits>
#include <print>
#include <set>
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
 * For each vertex on the source sphere, find the nearest vertex on the destination sphere.
 * This is the core of the morphing: vertices are matched by proximity on the sphere registration.
 */
static VectorXi buildNearestMap(const MatrixX3f& srcSphere, const MatrixX3f& dstSphere)
{
    int nSrc = srcSphere.rows();
    int nDst = dstSphere.rows();
    VectorXi mapping(nSrc);

    for (int s = 0; s < nSrc; ++s) {
        float minDist = std::numeric_limits<float>::max();
        int nearest = 0;
        for (int d = 0; d < nDst; ++d) {
            float dist = (srcSphere.row(s) - dstSphere.row(d)).squaredNorm();
            if (dist < minDist) {
                minDist = dist;
                nearest = d;
            }
        }
        mapping(s) = nearest;
    }
    return mapping;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    const char* subjectsDirEnv = std::getenv("SUBJECTS_DIR");

    CommandLineParser parser("Morph label files between subjects using sphere registration.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"from"}, "Source subject name.", "subject", {}});
    parser.add_option({{"to"}, "Destination subject name.", "subject", {}});
    parser.add_option({{"subjects_dir"}, "Subjects directory.", "dir", subjectsDirEnv ? subjectsDirEnv : ""});
    parser.add_option({{"hemi"}, "Hemisphere: lh or rh.", "hemi", {}});
    parser.add_option({{"label"}, "Input label file.", "file", {}});
    parser.add_option({{"out"}, "Output morphed label file.", "file", {}});
    parser.process(argc, argv, "mne_morph_labels", PROGRAM_VERSION);

    const std::string fromSubject = parser.value("from");
    const std::string toSubject = parser.value("to");
    const std::string subjectsDir = parser.value("subjects_dir");
    const std::string hemi = parser.value("hemi");
    const std::string labelFile = parser.value("label");
    const std::string outFile = parser.value("out");

    if (fromSubject.empty() || toSubject.empty()) {
        Log::critical("--from and --to are required.");
        std::print("{}", parser.help_text("mne_morph_labels"));
        return 1;
    }
    if (hemi.empty()) {
        Log::critical("--hemi is required.");
        std::print("{}", parser.help_text("mne_morph_labels"));
        return 1;
    }
    if (labelFile.empty()) {
        Log::critical("--label is required.");
        std::print("{}", parser.help_text("mne_morph_labels"));
        return 1;
    }
    if (outFile.empty()) {
        Log::critical("--out is required.");
        std::print("{}", parser.help_text("mne_morph_labels"));
        return 1;
    }
    if (subjectsDir.empty()) {
        Log::critical("$SUBJECTS_DIR not set.");
        return 1;
    }

    // Load sphere-registered surfaces
    const std::string srcSpherePath = std::format("{}/{}/surf/{}.sphere.reg", subjectsDir, fromSubject, hemi);
    const std::string dstSpherePath = std::format("{}/{}/surf/{}.sphere.reg", subjectsDir, toSubject, hemi);
    const std::string dstWhitePath = std::format("{}/{}/surf/{}.white", subjectsDir, toSubject, hemi);

    FsSurface srcSphere, dstSphere, dstWhite;
    if (!FsSurface::read(path_from_utf8(srcSpherePath), srcSphere)) {
        Log::critical("Cannot read source sphere: {}", srcSpherePath);
        return 1;
    }
    if (!FsSurface::read(path_from_utf8(dstSpherePath), dstSphere)) {
        Log::critical("Cannot read dest sphere: {}", dstSpherePath);
        return 1;
    }
    if (!FsSurface::read(path_from_utf8(dstWhitePath), dstWhite)) {
        Log::critical("Cannot read dest white surface: {}", dstWhitePath);
        return 1;
    }

    Log::info("Source sphere: {} vertices", static_cast<int>(srcSphere.rr().rows()));
    Log::info("Dest sphere:   {} vertices", static_cast<int>(dstSphere.rr().rows()));

    // Build nearest-neighbor mapping on sphere
    Log::info("Building nearest-neighbor mapping on sphere...");
    VectorXi mapping = buildNearestMap(srcSphere.rr(), dstSphere.rr());

    // Read input label
    FsLabel label;
    if (!FsLabel::read(path_from_utf8(labelFile), label)) {
        Log::critical("Cannot read label: {}", labelFile);
        return 1;
    }
    Log::info("Input label: {} vertices", static_cast<int>(label.vertices.size()));

    // Morph label vertices
    std::set<int> morphedSet;
    for (int i = 0; i < label.vertices.size(); ++i) {
        int srcVert = label.vertices(i);
        if (srcVert >= 0 && srcVert < mapping.size()) {
            morphedSet.insert(mapping(srcVert));
        }
    }

    // Write morphed label
    std::ofstream out(path_from_utf8(outFile));
    if (!out) {
        Log::critical("Cannot write: {}", outFile);
        return 1;
    }
    out << "#Morphed from " << fromSubject << " to " << toSubject << "\n";
    out << morphedSet.size() << "\n";
    for (int v : morphedSet) {
        if (v < dstWhite.rr().rows()) {
            out << std::format("{}  {:8.4f}  {:8.4f}  {:8.4f}  {:10.6f}\n",
                               v,
                               mne::tie_away_from_zero(dstWhite.rr()(v, 0), 4), mne::tie_away_from_zero(dstWhite.rr()(v, 1), 4), mne::tie_away_from_zero(dstWhite.rr()(v, 2), 4),
                               0.0);
        }
    }
    out.close();

    Log::info("Morphed label: {} vertices -> {}", static_cast<int>(morphedSet.size()), outFile);
    return 0;
}
