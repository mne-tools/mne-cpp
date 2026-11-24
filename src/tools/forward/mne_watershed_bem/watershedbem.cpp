//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     watershedbem.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    WatershedBem class definition.
 *
 *           Ported from the original MNE shell script mne_watershed_bem
 *           by Matti Hamalainen (SVN $Id: mne_watershed_bem 3391).
 *
 *           The workflow matches the original shell script:
 *             1. Validate environment (FREESURFER_HOME, SUBJECTS_DIR, subject)
 *             2. Create watershed output directory structure
 *             3. Run mri_watershed for BEM segmentation
 *             4. If .mgz input: convert surfaces (update volume geometry)
 *             5. Create head surface FIFF BEM file from outer_skin_surface
 *
 *           Cross-referenced with MNE-Python's mne.bem.make_watershed_bem().
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "watershedbem.h"
#include "mne_watershed_bem_settings.h"

#include <mne/fs/fs_surface.h>

#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>

#include <mne/mri/mri_mgh_io.h>
#include <mne/mri/mri_vol_data.h>

#include <mne/fiff/fiff_file.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/base/process.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdio>
#include <filesystem>
#include <format>
#include <print>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEWATERSHEDBEM;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// LOCAL HELPERS
//=============================================================================================================

namespace
{

bool fileExists(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::exists(path_from_utf8(path), ec);
}

bool dirExists(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::is_directory(path_from_utf8(path), ec);
}

bool makePath(const std::string& path)
{
    std::error_code ec;
    std::filesystem::create_directories(path_from_utf8(path), ec);
    return std::filesystem::is_directory(path_from_utf8(path), ec);
}

void removeFile(const std::string& path)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(path_from_utf8(path), ec))
        std::filesystem::remove(path_from_utf8(path), ec);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

WatershedBem::WatershedBem(const MNEWatershedBemSettings& settings)
: m_settings(settings)
{
}

//=============================================================================================================

int WatershedBem::run()
{
    //
    // Validate environment — ported from mne_watershed_bem shell script
    //
    if (m_settings.freeSurferHome().empty()) {
        Log::critical("The FreeSurfer environment needs to be set up for this tool.");
        Log::critical("Please set FREESURFER_HOME.");
        return 1;
    }

    if (m_settings.subjectsDir().empty()) {
        Log::critical("The subjects directory (SUBJECTS_DIR) is not set.");
        Log::critical("Use --subjects-dir or set the SUBJECTS_DIR environment variable.");
        return 1;
    }

    if (m_settings.subject().empty()) {
        Log::critical("The subject name is not set.");
        Log::critical("Use --subject or set the SUBJECT environment variable.");
        return 1;
    }

    //
    // Set up paths — following the shell script's variable naming
    //
    std::string subjectDir = m_settings.subjectsDir() + "/" + m_settings.subject();
    std::string mriDir = subjectDir + "/mri";
    std::string T1Dir = mriDir + "/" + m_settings.volume();
    std::string T1Mgz = mriDir + "/" + m_settings.volume() + ".mgz";
    std::string bemDir = subjectDir + "/bem";
    std::string wsDir = subjectDir + "/bem/watershed";

    //
    // Validate directory structure
    //
    if (!dirExists(subjectDir)) {
        Log::critical("Could not find the MRI data directory {}", subjectDir);
        return 1;
    }

    // Create bem directory if needed
    if (!dirExists(bemDir)) {
        if (!makePath(bemDir)) {
            Log::critical("Could not create the model directory {}", bemDir);
            return 1;
        }
    }

    // Check that MRI data exists (directory or .mgz file)
    bool useMgz = fileExists(T1Mgz);
    if (!dirExists(T1Dir) && !useMgz) {
        Log::critical("Could not find the MRI data.");
        Log::critical("  Looked for directory: {}", T1Dir);
        Log::critical("  Looked for file: {}", T1Mgz);
        return 1;
    }

    // Check for existing watershed directory
    if (dirExists(wsDir)) {
        if (!m_settings.overwrite()) {
            Log::critical("{} already exists. Use the --overwrite option to recreate it.", wsDir);
            return 1;
        } else {
            std::error_code ec;
            std::filesystem::remove_all(path_from_utf8(wsDir), ec);
            if (ec) {
                Log::critical("Could not remove {}", wsDir);
                return 1;
            }
        }
    }

    //
    // Report parameters
    //
    Log::info("{}", "");
    Log::info("Running mri_watershed for BEM segmentation with the following parameters");
    Log::info("{}", "");
    Log::info("SUBJECTS_DIR = {}", m_settings.subjectsDir());
    Log::info("Subject      = {}", m_settings.subject());
    Log::info("Result dir   = {}", wsDir);
    Log::info("{}", "");

    // Create output directory structure
    std::string wsSubDir = wsDir + "/ws";
    if (!makePath(wsSubDir)) {
        Log::critical("Could not create the destination directories.");
        return 1;
    }

    //
    // Step 1: Run mri_watershed
    //
    std::string mriInput = useMgz ? T1Mgz : T1Dir;
    if (!runMriWatershed(mriInput, wsDir)) {
        return 1;
    }

    //
    // Step 2: Convert surfaces if using .mgz input
    //   Equivalent to the shell script's:
    //     for s in $surfaces ; do
    //       mne_convert_surface --surf $s --mghmri $T1_mgz --surfout $s
    //     done
    //
    if (useMgz) {
        if (!convertSurfaces(wsDir, T1Mgz)) {
            return 1;
        }
    }

    //
    // Step 3: Create head surface FIFF BEM file
    //   Equivalent to:
    //     mne_surf2bem --surf $ws_dir/${SUBJECT}_outer_skin_surface --id 4 --fif $SUBJECT-head.fif
    //
    std::string outerSkinSurf = wsDir + "/" + m_settings.subject() + "_outer_skin_surface";
    std::string headFif = bemDir + "/" + m_settings.subject() + "-head.fif";

    // Remove existing head file
    removeFile(headFif);

    if (!createBemFif(outerSkinSurf, headFif)) {
        return 1;
    }

    Log::info("Created {}", headFif);
    Log::info("\nComplete.\n");
    return 0;
}

//=============================================================================================================

bool WatershedBem::runMriWatershed(const std::string& mriInput, const std::string& wsDir)
{
    //
    // Build the mri_watershed command line
    //   mri_watershed $preflood $atlas -useSRAS -surf $ws_dir/$SUBJECT $T1_input $ws_dir/ws
    //
    std::string watershedBin = m_settings.freeSurferHome() + "/bin/mri_watershed";
    if (!fileExists(watershedBin)) {
        Log::critical("Could not find mri_watershed at {}", watershedBin);
        return false;
    }

    StringList args;

    // Preflood height
    if (m_settings.preflood() >= 0) {
        args.insert(args.end(), {"-h", std::to_string(m_settings.preflood())});
    }

    // Atlas options
    if (m_settings.gcaAtlas()) {
        // GCA atlas: -atlas -T1 -brain_atlas $FREESURFER_HOME/average/RB_all_withskull_2007-08-08.gca
        //            $subject_dir/mri/transforms/talairach_with_skull.lta
        std::string gcaFile = m_settings.freeSurferHome() + "/average/RB_all_withskull_2007-08-08.gca";
        std::string ltaFile = m_settings.subjectsDir() + "/" + m_settings.subject() + "/mri/transforms/talairach_with_skull.lta";
        args.insert(args.end(), {"-atlas", "-T1", "-brain_atlas", gcaFile, ltaFile});
    } else if (m_settings.atlas()) {
        args.push_back("-atlas");
    }

    // Use surface RAS coordinates
    args.push_back("-useSRAS");

    // FsSurface output prefix
    args.insert(args.end(), {"-surf", wsDir + "/" + m_settings.subject()});

    // Input MRI and output directory
    args.push_back(mriInput);
    args.push_back(wsDir + "/ws");

    Log::info("Running: {} {}\n", watershedBin, join(args, " "));

    //
    // Execute mri_watershed
    //
    // Set up FreeSurfer environment for the subprocess
    ProcessOptions options;
    options.environment = {{"FREESURFER_HOME", m_settings.freeSurferHome()},
                           {"SUBJECTS_DIR", m_settings.subjectsDir()}};

    // Forward the output; wait indefinitely for mri_watershed to finish (it can take minutes)
    options.on_output = [](ProcessChannel channel, std::string_view data) {
        std::FILE* stream = channel == ProcessChannel::StdOut ? stdout : stderr;
        std::print(stream, "{}", data);
        std::fflush(stream);
    };
    const ProcessResult result = run_process(path_from_utf8(watershedBin), args, options);

    if (!result.started) {
        Log::critical("Failed to start mri_watershed: {}", result.error);
        return false;
    }

    if (result.crashed || result.timed_out) {
        Log::critical("mri_watershed did not finish: {}", result.crashed ? "Process crashed" : "Process timed out");
        return false;
    }

    if (result.exit_code != 0) {
        Log::critical("mri_watershed failed with exit code {}", result.exit_code);
        return false;
    }

    return true;
}

//=============================================================================================================

bool WatershedBem::convertSurfaces(const std::string& wsDir, const std::string& mgzFile)
{
    //
    // When using .mgz input, the watershed surfaces need their volume
    // geometry updated from the MGH file. This is equivalent to:
    //   mne_convert_surface --surf $s --mghmri $T1_mgz --surfout $s
    //
    // The mri_watershed tool with -useSRAS already produces surfaces in
    // surface RAS coordinates. The convert step here ensures the volume
    // geometry metadata is consistent with the source MRI volume.
    //
    // For the head surface FIFF BEM file creation step that follows,
    // we read the surfaces via mne::FsSurface which handles the coordinate
    // system correctly, so this step primarily ensures consistency for
    // other tools that may read these surface files.
    //

    // Read the MGH volume to get coordinate transform
    MriVolData volData;
    std::vector<FiffCoordTrans> additionalTrans;
    std::string subjectMriDir = m_settings.subjectsDir() + "/" + m_settings.subject() + "/mri";

    if (!MriMghIO::read(mgzFile, volData, additionalTrans, subjectMriDir, m_settings.verbose())) {
        Log::critical("Failed to read MGH/MGZ file {}", mgzFile);
        return false;
    }

    // FsSurface names produced by mri_watershed
    const StringList surfaceNames = {m_settings.subject() + "_brain_surface",
                                     m_settings.subject() + "_inner_skull_surface",
                                     m_settings.subject() + "_outer_skull_surface",
                                     m_settings.subject() + "_outer_skin_surface"};

    for (const std::string& surfName : surfaceNames) {
        std::string surfPath = std::format("{}/{}", wsDir, surfName);
        if (!fileExists(surfPath)) {
            Log::warning("Watershed surface not found: {}", surfPath);
            continue;
        }

        if (m_settings.verbose()) {
            Log::info("Verifying surface: {}", surfPath);
        }

        // Read surface to verify it is valid
        FsSurface surf;
        FsSurface::read(path_from_utf8(surfPath), surf, false);
        if (surf.rr().rows() == 0) {
            Log::warning("FsSurface {} has no vertices.", surfPath);
        } else if (m_settings.verbose()) {
            Log::info("  {} vertices, {} triangles", static_cast<int>(surf.rr().rows()), static_cast<int>(surf.tris().rows()));
        }
    }

    return true;
}

//=============================================================================================================

bool WatershedBem::createBemFif(const std::string& surfFile, const std::string& fifFile)
{
    //
    // Create a FIFF BEM head surface file.
    // Equivalent to: mne_surf2bem --surf $surfFile --id 4 --fif $fifFile
    //
    // Steps:
    //   1. Read FreeSurfer surface via mne::FsSurface
    //   2. Create MNEBemSurface with head surface ID (4)
    //   3. Write BEM FIFF file via MNEBem::write()
    //

    // Read the FreeSurfer outer skin surface
    FsSurface fsSurface;
    FsSurface::read(path_from_utf8(surfFile), fsSurface, false);

    if (fsSurface.rr().rows() == 0 || fsSurface.tris().rows() == 0) {
        Log::critical("Failed to read surface from {}", surfFile);
        return false;
    }

    Log::info("Read surface: {} vertices, {} triangles", static_cast<int>(fsSurface.rr().rows()), static_cast<int>(fsSurface.tris().rows()));

    //
    // Create BEM surface
    //   mne::FsSurface stores vertices in meters (divided by 1000 during read).
    //   MNEBemSurface expects vertices in meters as well.
    //
    MNEBemSurface bemSurface;
    bemSurface.id = FIFFV_BEM_SURF_ID_HEAD; // 4 = head surface
    bemSurface.np = static_cast<int>(fsSurface.rr().rows());
    bemSurface.ntri = static_cast<int>(fsSurface.tris().rows());
    bemSurface.coord_frame = FIFFV_COORD_MRI;
    bemSurface.sigma = 0.0f; // Default conductivity

    // Copy vertex coordinates and triangles
    bemSurface.rr = fsSurface.rr();
    bemSurface.itris = fsSurface.tris();

    // Compute vertex normals
    bemSurface.nn = FsSurface::compute_normals(Eigen::MatrixX3f(bemSurface.rr), Eigen::MatrixX3i(bemSurface.itris));

    //
    // Create BEM and write to FIFF file
    //
    MNEBem bem;
    bem.push_back(bemSurface);

    if (auto written = bem.write(path_from_utf8(fifFile)); !written) {
        Log::critical("{}", written.error().what());
        return false;
    }

    return true;
}
