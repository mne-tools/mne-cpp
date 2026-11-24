//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2004
 * @brief    Convert FreeSurfer annotation files to individual label files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fs/fs_annotation.h>
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
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <string>
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

static bool writeLabel(const std::string& filename, const std::string& comment,
                       const VectorXi& vertices, const MatrixX3f& positions,
                       const VectorXd& values)
{
    std::ofstream out(path_from_utf8(filename));
    if (!out) {
        Log::critical("Cannot write label file: {}", filename);
        return false;
    }
    out << "#" << comment << "\n";
    out << vertices.size() << "\n";
    for (int i = 0; i < vertices.size(); i++) {
        out << std::format("{}  {:8.4f}  {:8.4f}  {:8.4f}  {:10.6f}\n",
                           vertices(i),
                           mne::tie_away_from_zero(positions(i, 0), 4), mne::tie_away_from_zero(positions(i, 1), 4), mne::tie_away_from_zero(positions(i, 2), 4),
                           mne::tie_away_from_zero(values(i), 6));
    }
    return true;
}

//=============================================================================================================

static bool processHemisphere(const std::string& subjectsDir, const std::string& subject,
                              const std::string& hemi, const std::string& annotName,
                              const std::string& surfName, const std::string& outDir)
{
    // Read surface for vertex coordinates
    const std::string surfPath = std::format("{}/{}/surf/{}.{}", subjectsDir, subject, hemi, surfName);
    FsSurface surface;
    if (!FsSurface::read(path_from_utf8(surfPath), surface)) {
        Log::critical("Cannot read surface: {}", surfPath);
        return false;
    }
    Log::info("Read surface: {} ({} vertices)", surfPath, static_cast<long long>(surface.rr().rows()));

    // Read annotation
    const std::string annotPath = std::format("{}/{}/label/{}.{}.annot", subjectsDir, subject, hemi, annotName);
    FsAnnotation annot;
    if (!FsAnnotation::read(path_from_utf8(annotPath), annot)) {
        Log::critical("Cannot read annotation: {}", annotPath);
        return false;
    }
    Log::info("Read annotation: {}", annotPath);

    // Convert to labels using the library method
    std::vector<FsLabel> labels;
    std::vector<Eigen::RowVector4i> labelRgbas;
    annot.toLabels(surface, labels, labelRgbas);

    Log::info("Found {} labels in {} hemisphere", static_cast<long long>(labels.size()), hemi);

    // Write each label to a file
    int written = 0;
    for (int i = 0; i < std::ssize(labels); i++) {
        const FsLabel& label = labels[i];
        std::string labelName = label.name;
        if (labelName.empty())
            labelName = std::format("label_{}", i);

        const std::string filename = std::format("{}/{}-{}.label", outDir, labelName, hemi);

        VectorXi verts = label.vertices;
        MatrixX3f pos(verts.size(), 3);
        VectorXd values = VectorXd::Zero(verts.size());

        for (int v = 0; v < verts.size(); v++) {
            int vi = verts(v);
            if (vi < surface.rr().rows()) {
                pos(v, 0) = surface.rr()(vi, 0);
                pos(v, 1) = surface.rr()(vi, 1);
                pos(v, 2) = surface.rr()(vi, 2);
            }
        }

        const std::string comment = std::format(" {} annotation {} {}", annotName, hemi, labelName);
        if (writeLabel(filename, comment, verts, pos, values)) {
            written++;
        }
    }

    Log::info("Wrote {} label files for {} hemisphere", written, hemi);
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    const char* subjectsDirEnv = std::getenv("SUBJECTS_DIR");

    CommandLineParser parser("Convert FreeSurfer annotation file to individual label files.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"subject"}, "Subject name.", "name", {}});
    parser.add_option({{"subjects_dir"}, "Subjects directory.", "dir", subjectsDirEnv ? subjectsDirEnv : ""});
    parser.add_option({{"annot"}, "Annotation name (e.g., aparc, aparc.a2009s).", "name", "aparc"});
    parser.add_option({{"hemi"}, "Hemisphere: lh or rh (default: both).", "hemi", {}});
    parser.add_option({{"outdir"}, "Output directory for label files.", "dir", {}});
    parser.add_option({{"surf"}, "Surface to use for coordinates.", "name", "white"});
    parser.process(argc, argv, "mne_annot2labels", PROGRAM_VERSION);

    const std::string subject = parser.value("subject");
    const std::string subjectsDir = parser.value("subjects_dir");
    const std::string annotName = parser.value("annot");
    const std::string hemi = parser.value("hemi");
    std::string outDir = parser.value("outdir");
    const std::string surfName = parser.value("surf");

    if (subject.empty()) {
        Log::critical("--subject is required.");
        return 1;
    }
    if (subjectsDir.empty()) {
        Log::critical("--subjects_dir or $SUBJECTS_DIR is required.");
        return 1;
    }

    // Default output dir
    if (outDir.empty())
        outDir = std::format("{}/{}/label", subjectsDir, subject);

    std::error_code ec;
    std::filesystem::create_directories(path_from_utf8(outDir), ec);

    bool ok = true;
    if (hemi.empty() || hemi == "lh")
        ok = processHemisphere(subjectsDir, subject, "lh", annotName, surfName, outDir) && ok;
    if (hemi.empty() || hemi == "rh")
        ok = processHemisphere(subjectsDir, subject, "rh", annotName, surfName, outDir) && ok;

    return ok ? 0 : 1;
}
