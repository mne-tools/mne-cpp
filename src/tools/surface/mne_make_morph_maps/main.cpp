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
 * @brief    Compute morphing maps between two subjects using sphere-registered surfaces.
 *
 * Writes $SUBJECTS_DIR/morph-maps/<from>-<to>-morph.fif with the maps of both
 * hemispheres in both directions, like MNE-C mne_make_morph_maps and
 * mne.read_morph_map (see mne::MNEMorphMap::compute).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fs/fs_surface.h>
#include <mne/mne/mne_morph_map.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char* argv[])
{
    const char* subjectsDirEnv = std::getenv("SUBJECTS_DIR");

    CommandLineParser parser("Compute the morphing maps between two subjects from their ?h.sphere.reg surfaces.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"from"}, "Source subject name.", "subject", {}});
    parser.add_option({{"to"}, "Destination subject name.", "subject", {}});
    parser.add_option({{"subjects_dir"}, "Subjects directory.", "dir", subjectsDirEnv ? subjectsDirEnv : ""});
    parser.add_option({{"out"}, "Output morph map FIFF file (default $SUBJECTS_DIR/morph-maps/<from>-<to>-morph.fif).", "file", {}});
    parser.process(argc, argv, "mne_make_morph_maps", PROGRAM_VERSION);

    const std::string fromSubject = parser.value("from");
    const std::string toSubject = parser.value("to");
    const std::string subjectsDir = parser.value("subjects_dir");
    std::string outFile = parser.value("out");

    if (fromSubject.empty() || toSubject.empty()) {
        Log::critical("--from and --to are required.");
        return 1;
    }
    if (subjectsDir.empty()) {
        Log::critical("$SUBJECTS_DIR not set.");
        return 1;
    }
    if (outFile.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(path_from_utf8(subjectsDir + "/morph-maps"), ec);
        outFile = std::format("{}/morph-maps/{}-{}-morph.fif", subjectsDir, fromSubject, toSubject);
    }

    std::vector<MNEMorphMap> maps;
    for (int hemi = 0; hemi < 2; ++hemi) {
        const std::string name = std::string(hemi == 0 ? "lh" : "rh") + ".sphere.reg";
        FsSurface fromSphere;
        FsSurface toSphere;
        if (!FsSurface::read(path_from_utf8(std::format("{}/{}/surf/{}", subjectsDir, fromSubject, name)), fromSphere, false) ||
            !FsSurface::read(path_from_utf8(std::format("{}/{}/surf/{}", subjectsDir, toSubject, name)), toSphere, false)) {
            Log::critical("Cannot read the {} surfaces.", name);
            return 1;
        }
        MNEMorphMap fromTo = MNEMorphMap::compute(fromSphere.rr(), fromSphere.tris(), toSphere.rr());
        fromTo.hemi = hemi;
        fromTo.from_subj = fromSubject;
        fromTo.to_subj = toSubject;
        MNEMorphMap toFrom = MNEMorphMap::compute(toSphere.rr(), toSphere.tris(), fromSphere.rr());
        toFrom.hemi = hemi;
        toFrom.from_subj = toSubject;
        toFrom.to_subj = fromSubject;
        Log::info("{}: {} -> {} and {} -> {} vertices", name, fromTo.map->cols(), fromTo.map->rows(), toFrom.map->cols(), toFrom.map->rows());
        maps.push_back(std::move(fromTo));
        maps.push_back(std::move(toFrom));
    }
    if (!MNEMorphMap::write(path_from_utf8(outFile), {&maps[0], &maps[2], &maps[1], &maps[3]})) {
        Log::critical("Cannot write {}", outFile);
        return 1;
    }
    Log::info("Written morph maps to: {}", outFile);
    return 0;
}
