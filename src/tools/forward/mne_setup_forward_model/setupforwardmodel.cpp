//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     setupforwardmodel.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2006
 * @brief    SetupForwardModel class definition.
 *
 *           Ported from the original MNE shell script mne_setup_forward_model
 *           by Matti Hamalainen (SVN $Id: mne_setup_forward_model 3282
 *           2011-02-02 14:28:16Z gramfort $).
 *
 *           The original script orchestrated three external programs:
 *             1. mne_surf2bem — create BEM geometry FIFF file
 *             2. mne_list_bem — export .pnt and .surf files
 *             3. mne_prepare_bem_model — compute BEM solution matrix
 *
 *           This C++ port uses mne-cpp library classes directly for all
 *           three steps, eliminating the need for external tools.
 *
 *           Cross-referenced with MNE-Python's mne.make_bem_model() and
 *           mne.make_bem_solution().
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "setupforwardmodel.h"
#include "mne_setup_forward_model_settings.h"

#include <mne/fs/fs_surface.h>

#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>

#include <mne/fiff/fiff_file.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_stream.h>

#include <mne/fwd/fwd_bem_model.h>

#include <mne/mne/mne_surface.h>
#include <mne/mne/mne_triangle.h>

#include <mne/fiff/fiff_byte_swap.h>
#include <mne/base/byte_stream.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstdint>
#include <format>
#include <fstream>
#include <string>
#include <string_view>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESETUPFORWARDMODEL;
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

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SetupForwardModel::SetupForwardModel(const MNESetupForwardModelSettings& settings)
: m_settings(settings)
{
}

//=============================================================================================================

int SetupForwardModel::run()
{
    //
    // Validate settings
    //
    if (m_settings.subjectsDir().empty()) {
        Log::critical("The environment variable SUBJECTS_DIR should be set.");
        Log::critical("Use --subjects-dir or set the SUBJECTS_DIR environment variable.");
        return 1;
    }
    if (m_settings.subject().empty()) {
        Log::critical("Subject not specified.");
        Log::critical("Use --subject or set the SUBJECT environment variable.");
        return 1;
    }

    //
    // Derive paths
    //
    std::string subjectDir = m_settings.subjectsDir() + "/" + m_settings.subject();
    std::string bemDir = subjectDir + "/bem";
    std::string ext = m_settings.useSurfFormat() ? "surf" : "tri";

    if (!dirExists(subjectDir)) {
        Log::critical("Could not find the MRI data directory {}", subjectDir);
        return 1;
    }
    if (!dirExists(bemDir)) {
        Log::critical("Could not find the BEM directory {}", bemDir);
        return 1;
    }

    //
    // Locate surface files
    //
    std::string innerSkullFile;
    if (!locateSurface(bemDir, "inner_skull", ext, innerSkullFile)) {
        Log::critical("Could not find the inner skull triangulation");
        return 1;
    }

    std::string outerSkullFile, outerSkinFile;
    if (!m_settings.homogeneous()) {
        if (!locateSurface(bemDir, "outer_skull", ext, outerSkullFile)) {
            Log::critical("Could not find the outer skull triangulation {}", outerSkullFile);
            return 1;
        }
        if (!locateSurface(bemDir, "outer_skin", ext, outerSkinFile)) {
            Log::critical("Could not find the scalp triangulation {}", outerSkinFile);
            return 1;
        }
    }

    //
    // Step 1: Read surfaces and create BEM geometry
    //
    Log::info("{}", "");
    Log::info("Setting up the BEM with the following parameters:");
    Log::info("{}", "");
    Log::info("SUBJECTS_DIR       = {}", m_settings.subjectsDir());
    Log::info("Subject            = {}", m_settings.subject());

    // Read inner skull
    MNEBemSurface innerSkull;
    if (m_settings.useSurfFormat()) {
        if (!readFreeSurferSurf(innerSkullFile, FIFFV_BEM_SURF_ID_BRAIN,
                                m_settings.brainConductivity(),
                                m_settings.innerShift(), innerSkull)) {
            return 1;
        }
    } else {
        if (!readAsciiTriFile(innerSkullFile, FIFFV_BEM_SURF_ID_BRAIN,
                              m_settings.brainConductivity(),
                              m_settings.innerShift(), innerSkull)) {
            return 1;
        }
    }
    Log::info("Inner skull        = {} ({} triangles)", innerSkullFile, innerSkull.ntri);

    MNEBemSurface outerSkull, outerSkin;
    if (!m_settings.homogeneous()) {
        // Read outer skull
        if (m_settings.useSurfFormat()) {
            if (!readFreeSurferSurf(outerSkullFile, FIFFV_BEM_SURF_ID_SKULL,
                                    m_settings.skullConductivity(),
                                    m_settings.outerShift(), outerSkull)) {
                return 1;
            }
        } else {
            if (!readAsciiTriFile(outerSkullFile, FIFFV_BEM_SURF_ID_SKULL,
                                  m_settings.skullConductivity(),
                                  m_settings.outerShift(), outerSkull)) {
                return 1;
            }
        }
        Log::info("Outer skull        = {} ({} triangles)", outerSkullFile, outerSkull.ntri);

        // Read outer skin (scalp)
        if (m_settings.useSurfFormat()) {
            if (!readFreeSurferSurf(outerSkinFile, FIFFV_BEM_SURF_ID_HEAD,
                                    m_settings.scalpConductivity(),
                                    m_settings.scalpShift(), outerSkin)) {
                return 1;
            }
        } else {
            if (!readAsciiTriFile(outerSkinFile, FIFFV_BEM_SURF_ID_HEAD,
                                  m_settings.scalpConductivity(),
                                  m_settings.scalpShift(), outerSkin)) {
                return 1;
            }
        }
        Log::info("Scalp              = {} ({} triangles)", outerSkinFile, outerSkin.ntri);
    }

    Log::info("brain conductivity = {:g} S/m", m_settings.brainConductivity());
    if (!m_settings.homogeneous()) {
        Log::info("skull conductivity = {:g} S/m", m_settings.skullConductivity());
        Log::info("scalp conductivity = {:g} S/m", m_settings.scalpConductivity());
    }

    //
    // Figure out the model name
    //
    std::string modelFile;
    if (m_settings.modelName().empty()) {
        if (m_settings.homogeneous()) {
            modelFile = std::format("{}/{}-{}-bem.fif", bemDir, m_settings.subject(), innerSkull.ntri);
        } else {
            modelFile = std::format("{}/{}-{}-{}-{}-bem.fif", bemDir, m_settings.subject(), innerSkull.ntri, outerSkull.ntri, outerSkin.ntri);
        }
    } else {
        modelFile = bemDir + "/" + m_settings.modelName() + "-bem.fif";
    }

    Log::info("Resulting BEM      = {}\n", modelFile);

    //
    // Check if file already exists
    //
    if (fileExists(modelFile) && !m_settings.overwrite()) {
        Log::critical("Output file {} already exists.", modelFile);
        Log::critical("Use --overwrite to overwrite.");
        return 1;
    }

    //
    // >> 1. Creating the BEM geometry file
    //
    Log::info(">> 1. Creating the BEM geometry file...");

    MNEBem bem;
    if (m_settings.homogeneous()) {
        bem.push_back(innerSkull);
    } else {
        // Order: head (4), skull (3), brain (1) — standard BEM nesting
        bem.push_back(outerSkin);
        bem.push_back(outerSkull);
        bem.push_back(innerSkull);
    }

    if (auto written = bem.write(path_from_utf8(modelFile)); !written) {
        Log::critical("{}", written.error().what());
        return 1;
    }
    Log::info("BEM geometry file written to {}\n", modelFile);

    //
    // >> 2. Creating ascii pnt files and surf files
    //
    Log::info(">> 2. Creating ascii pnt files and surf files...");

    if (m_settings.homogeneous()) {
        std::string pntFile = std::format("{}/{}-inner_skull-{}.pnt", bemDir, m_settings.subject(), innerSkull.ntri);
        std::string surfFile = std::format("{}/{}-inner_skull-{}.surf", bemDir, m_settings.subject(), innerSkull.ntri);

        if (!exportPntFile(innerSkull, pntFile))
            return 1;
        if (!exportSurfFile(innerSkull, surfFile))
            return 1;
    } else {
        // Outer skin (scalp)
        {
            std::string pntFile = std::format("{}/{}-outer_skin-{}.pnt", bemDir, m_settings.subject(), outerSkin.ntri);
            std::string surfFile = std::format("{}/{}-outer_skin-{}.surf", bemDir, m_settings.subject(), outerSkin.ntri);

            if (!exportPntFile(outerSkin, pntFile))
                return 1;
            if (!exportSurfFile(outerSkin, surfFile))
                return 1;
        }

        // Outer skull
        {
            std::string pntFile = std::format("{}/{}-outer_skull-{}.pnt", bemDir, m_settings.subject(), outerSkull.ntri);
            std::string surfFile = std::format("{}/{}-outer_skull-{}.surf", bemDir, m_settings.subject(), outerSkull.ntri);

            if (!exportPntFile(outerSkull, pntFile))
                return 1;
            if (!exportSurfFile(outerSkull, surfFile))
                return 1;
        }

        // Inner skull
        {
            std::string pntFile = std::format("{}/{}-inner_skull-{}.pnt", bemDir, m_settings.subject(), innerSkull.ntri);
            std::string surfFile = std::format("{}/{}-inner_skull-{}.surf", bemDir, m_settings.subject(), innerSkull.ntri);

            if (!exportPntFile(innerSkull, pntFile))
                return 1;
            if (!exportSurfFile(innerSkull, surfFile))
                return 1;
        }
    }
    Log::info("{}", "");

    //
    // >> 3. Calculating BEM geometry data (solution)
    //
    if (!m_settings.noSolution()) {
        Log::info(">> 3. Calculating BEM geometry data (this takes several minutes)...\n");

        std::string solFile = modelFile;
        for (std::size_t pos = solFile.find(".fif"); pos != std::string::npos; pos = solFile.find(".fif", pos + 8))
            solFile.replace(pos, 4, "-sol.fif");

        if (!prepareBemSolution(modelFile, solFile)) {
            Log::critical("Model preparation failed.");
            return 1;
        } else {
            Log::info("\nThe model {} is now ready for use", solFile);
        }
    }

    Log::info("\nComplete.");
    return 0;
}

//=============================================================================================================

bool SetupForwardModel::locateSurface(const std::string& bemDir, const std::string& name,
                                      const std::string& ext, std::string& path) const
{
    // Try: <bem_dir>/<name>.<ext>
    path = bemDir + "/" + name + "." + ext;
    if (fileExists(path)) {
        return true;
    }

    // Try: <bem_dir>/<subject>-<name>.<ext>
    std::string alt = bemDir + "/" + m_settings.subject() + "-" + name + "." + ext;
    if (fileExists(alt)) {
        path = alt;
        return true;
    }

    // Not found
    return false;
}

//=============================================================================================================

bool SetupForwardModel::readAsciiTriFile(const std::string& fileName, int id, float sigma,
                                         float shift, MNEBemSurface& surf) const
{
    //
    // Read ASCII triangle file
    //
    // Format:
    //   nvert
    //   x1 y1 z1
    //   x2 y2 z2
    //   ...
    //   ntri
    //   v1 v2 v3   (1-based vertex indices)
    //   ...
    //
    std::ifstream in(path_from_utf8(fileName));
    if (!in) {
        Log::critical("Could not open ASCII triangle file: {}", fileName);
        return false;
    }

    // Read number of vertices
    int nvert = 0;
    in >> nvert;
    if (nvert <= 0) {
        Log::critical("Invalid vertex count in {}", fileName);
        return false;
    }

    // Read vertices
    MatrixX3f rr(nvert, 3);
    for (int k = 0; k < nvert; ++k) {
        // Read as double and round to float, as QTextStream did
        double x = 0.0, y = 0.0, z = 0.0;
        in >> x >> y >> z;
        if (!in) {
            Log::critical("Error reading vertex {} from {}", k, fileName);
            return false;
        }
        rr(k, 0) = static_cast<float>(x);
        rr(k, 1) = static_cast<float>(y);
        rr(k, 2) = static_cast<float>(z);
    }

    // Read number of triangles
    int ntri = 0;
    in >> ntri;
    if (ntri <= 0) {
        Log::critical("Invalid triangle count in {}", fileName);
        return false;
    }

    // Read triangles (1-based in file, convert to 0-based)
    MatrixX3i tris(ntri, 3);
    for (int k = 0; k < ntri; ++k) {
        int v1 = 0, v2 = 0, v3 = 0;
        in >> v1 >> v2 >> v3;
        if (!in) {
            Log::critical("Error reading triangle {} from {}", k, fileName);
            return false;
        }
        if (m_settings.swap()) {
            tris(k, 0) = v1 - 1;
            tris(k, 1) = v3 - 1; // Swapped
            tris(k, 2) = v2 - 1; // Swapped
        } else {
            tris(k, 0) = v1 - 1;
            tris(k, 1) = v2 - 1;
            tris(k, 2) = v3 - 1;
        }
    }
    in.close();

    //
    // Convert units: default is mm, --meters means already in meters
    //
    if (!m_settings.meters()) {
        rr /= 1000.0f;
    }

    //
    // Populate BEM surface
    //
    surf.np = nvert;
    surf.ntri = ntri;
    surf.coord_frame = FIFFV_COORD_MRI;
    surf.rr = rr;
    surf.itris = tris;
    surf.id = id;
    surf.sigma = sigma;

    // Compute vertex normals
    surf.nn = FsSurface::compute_normals(Eigen::MatrixX3f(surf.rr), Eigen::MatrixX3i(surf.itris));

    // Shift vertices if requested
    if (shift != 0.0f) {
        shiftVertices(surf, shift / 1000.0f); // Convert mm to meters
    }

    return true;
}

//=============================================================================================================

bool SetupForwardModel::readFreeSurferSurf(const std::string& fileName, int id, float sigma,
                                           float shift, MNEBemSurface& surf) const
{
    auto geometry = FsSurface::read_geometry(path_from_utf8(fileName));
    if (!geometry) {
        Log::critical("{}", geometry.error().what());
        return false;
    }

    //
    // Populate BEM surface
    //
    surf.np = static_cast<int>(geometry->rr.rows());
    surf.ntri = static_cast<int>(geometry->tris.rows());
    surf.coord_frame = FIFFV_COORD_MRI;
    surf.rr = geometry->rr;
    surf.itris = geometry->tris;
    surf.id = id;
    surf.sigma = sigma;

    // Compute vertex normals
    surf.nn = FsSurface::compute_normals(Eigen::MatrixX3f(surf.rr), Eigen::MatrixX3i(surf.itris));

    // Shift vertices if requested
    if (shift != 0.0f) {
        shiftVertices(surf, shift / 1000.0f); // Convert mm to meters
    }

    return true;
}

//=============================================================================================================

void SetupForwardModel::shiftVertices(MNEBemSurface& surf, float shift) const
{
    for (int k = 0; k < surf.np; ++k) {
        surf.rr(k, 0) += shift * surf.nn(k, 0);
        surf.rr(k, 1) += shift * surf.nn(k, 1);
        surf.rr(k, 2) += shift * surf.nn(k, 2);
    }

    // Recompute normals after shifting
    surf.nn = FsSurface::compute_normals(Eigen::MatrixX3f(surf.rr), Eigen::MatrixX3i(surf.itris));

    Log::info("FsSurface vertices shifted by {:6.1f} mm.", 1000.0f * shift);
}

//=============================================================================================================

bool SetupForwardModel::exportPntFile(const MNEBemSurface& surf,
                                      const std::string& fileName) const
{
    //
    // Write ASCII .pnt file (mne_list_bem equivalent)
    //
    // Format: nvert on first line, then one "x y z" per line in mm
    //
    std::ofstream out(path_from_utf8(fileName));
    if (!out) {
        Log::critical("Could not open {} for writing", fileName);
        return false;
    }

    out << surf.np << "\n";
    for (int k = 0; k < surf.np; ++k) {
        // Convert from meters to mm for output
        out << std::format("{:.4f} {:.4f} {:.4f}\n",
                           static_cast<double>(surf.rr(k, 0)) * 1000.0,
                           static_cast<double>(surf.rr(k, 1)) * 1000.0,
                           static_cast<double>(surf.rr(k, 2)) * 1000.0);
    }
    out.close();
    Log::info("Written {}", fileName);
    return true;
}

//=============================================================================================================

bool SetupForwardModel::exportSurfFile(const MNEBemSurface& surf,
                                       const std::string& fileName) const
{
    //
    // Write FreeSurfer binary surface file (mne_list_bem --surf equivalent)
    //
    auto file = FileByteStream::open(path_from_utf8(fileName), FileByteStream::Mode::Write);
    if (!file) {
        Log::critical("Could not open {} for writing", fileName);
        return false;
    }

    // Magic number for triangle surface: 0xFFFFFE
    const std::array<std::uint8_t, 3> magic = {0xFF, 0xFF, 0xFE};
    file->write(magic.data(), magic.size());

    // Two lines of text (comment + newline)
    const std::string_view comment = "created by mne_setup_forward_model\n\n";
    file->write(comment.data(), comment.size());

    // Number of vertices and faces
    file->write_big_endian(static_cast<std::int32_t>(surf.np));
    file->write_big_endian(static_cast<std::int32_t>(surf.ntri));

    // Vertex coordinates (as float32 in mm — FreeSurfer convention)
    for (int k = 0; k < surf.np; ++k) {
        file->write_big_endian(surf.rr(k, 0) * 1000.0f); // meters -> mm
        file->write_big_endian(surf.rr(k, 1) * 1000.0f);
        file->write_big_endian(surf.rr(k, 2) * 1000.0f);
    }

    // Triangle vertex indices (as int32, 0-based)
    for (int k = 0; k < surf.ntri; ++k) {
        file->write_big_endian(static_cast<std::int32_t>(surf.itris(k, 0)));
        file->write_big_endian(static_cast<std::int32_t>(surf.itris(k, 1)));
        file->write_big_endian(static_cast<std::int32_t>(surf.itris(k, 2)));
    }

    file->close();
    Log::info("Written {}", fileName);
    return true;
}

//=============================================================================================================

bool SetupForwardModel::prepareBemSolution(const std::string& bemFile,
                                           const std::string& solFile) const
{
    //
    // Compute the BEM solution matrix using FwdBemModel
    // (mne_prepare_bem_model --bem $model --sol $model_sol --method linear)
    //
    // Steps:
    //   1. Load BEM surfaces from the geometry file
    //   2. Compute the linear collocation solution
    //   3. Save the BEM model with the solution to a new FIFF file
    //
    std::unique_ptr<FwdBemModel> bemModel;

    if (m_settings.homogeneous()) {
        bemModel = FwdBemModel::fwd_bem_load_homog_surface(path_from_utf8(bemFile));
    } else {
        bemModel = FwdBemModel::fwd_bem_load_three_layer_surfaces(path_from_utf8(bemFile));
    }

    if (!bemModel) {
        Log::critical("Could not load BEM surfaces from {}", bemFile);
        return false;
    }

    Log::info("Computing the linear collocation solution...");

    int result = bemModel->fwd_bem_compute_solution(FWD_BEM_LINEAR_COLL);
    if (result != 0) {
        Log::critical("BEM solution computation failed.");
        return false;
    }

    Log::info("Solution computed.");

    Log::info("Saving...");
    if (bemModel->fwd_bem_save_model(path_from_utf8(solFile)) != 0) {
        Log::critical("Could not write {}", solFile);
        return false;
    }
    Log::info("Saved the result to {}", solFile);


    return true;
}
